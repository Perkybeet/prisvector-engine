#include "FileWatcher.hpp"
#include "core/Logger.hpp"
#include <algorithm>

namespace Engine {

FileWatcher::FileWatcher() = default;

FileWatcher::~FileWatcher() {
    Stop();
}

void FileWatcher::Watch(const fs::path& path, bool recursive) {
    std::lock_guard<std::mutex> lock(m_DirMutex);

    // Check if already watching
    for (const auto& dir : m_WatchedDirs) {
        if (dir.Path == path) {
            LOG_CORE_WARN("Already watching directory: {}", path.string());
            return;
        }
    }

    if (!fs::exists(path)) {
        LOG_CORE_ERROR("Cannot watch non-existent path: {}", path.string());
        return;
    }

    WatchedDirectory watchedDir;
    watchedDir.Path = path;
    watchedDir.Recursive = recursive;

    // Initialize timestamps for existing files
    if (fs::is_directory(path)) {
        auto iterator = recursive ?
            fs::recursive_directory_iterator(path) :
            fs::recursive_directory_iterator(path, fs::directory_options::none);

        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file() && PassesFilter(entry.path())) {
                    watchedDir.FileTimestamps[entry.path()] = entry.last_write_time();
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.is_regular_file() && PassesFilter(entry.path())) {
                    watchedDir.FileTimestamps[entry.path()] = entry.last_write_time();
                }
            }
        }
    } else if (fs::is_regular_file(path)) {
        watchedDir.FileTimestamps[path] = fs::last_write_time(path);
    }

    m_WatchedDirs.push_back(std::move(watchedDir));
    LOG_CORE_INFO("Now watching: {} (recursive: {})", path.string(), recursive);
}

void FileWatcher::Unwatch(const fs::path& path) {
    std::lock_guard<std::mutex> lock(m_DirMutex);

    auto it = std::find_if(m_WatchedDirs.begin(), m_WatchedDirs.end(),
        [&path](const WatchedDirectory& dir) { return dir.Path == path; });

    if (it != m_WatchedDirs.end()) {
        m_WatchedDirs.erase(it);
        LOG_CORE_INFO("Stopped watching: {}", path.string());
    }
}

void FileWatcher::AddFilter(const String& extension) {
    // Ensure extension starts with '.'
    String ext = extension;
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    m_Filters.push_back(ext);
}

void FileWatcher::ClearFilters() {
    m_Filters.clear();
}

void FileWatcher::OnFileChanged(FileCallback callback) {
    m_Callbacks.push_back(std::move(callback));
}

void FileWatcher::Start() {
    if (m_Running.load()) return;

    m_Running.store(true);
    m_WatcherThread = std::thread(&FileWatcher::WatcherThread, this);
    LOG_CORE_INFO("FileWatcher started");
}

void FileWatcher::Stop() {
    if (!m_Running.load()) return;

    m_Running.store(false);
    if (m_WatcherThread.joinable()) {
        m_WatcherThread.join();
    }
    LOG_CORE_INFO("FileWatcher stopped");
}

u32 FileWatcher::Poll() {
    Vector<FileEvent> events;

    {
        std::lock_guard<std::mutex> lock(m_EventMutex);
        events = std::move(m_PendingEvents);
        m_PendingEvents.clear();
    }

    for (const auto& event : events) {
        for (const auto& callback : m_Callbacks) {
            callback(event);
        }
    }

    return static_cast<u32>(events.size());
}

void FileWatcher::WatcherThread() {
    while (m_Running.load()) {
        {
            std::lock_guard<std::mutex> lock(m_DirMutex);

            for (auto& watchedDir : m_WatchedDirs) {
                CheckDirectory(watchedDir.Path, watchedDir.Recursive);
            }
        }

        // Sleep for poll interval
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<i64>(m_PollInterval * 1000.0f))
        );
    }
}

void FileWatcher::CheckDirectory(const fs::path& dir, bool recursive) {
    if (!fs::exists(dir)) return;

    // Find the watched directory entry
    auto watchedIt = std::find_if(m_WatchedDirs.begin(), m_WatchedDirs.end(),
        [&dir](const WatchedDirectory& wd) { return wd.Path == dir; });

    if (watchedIt == m_WatchedDirs.end()) return;

    auto& timestamps = watchedIt->FileTimestamps;
    HashMap<fs::path, bool> currentFiles;

    // Check all current files
    auto checkFile = [&](const fs::directory_entry& entry) {
        if (!entry.is_regular_file()) return;
        if (!PassesFilter(entry.path())) return;

        const auto& path = entry.path();
        currentFiles[path] = true;

        auto lastWriteTime = entry.last_write_time();
        auto it = timestamps.find(path);

        if (it == timestamps.end()) {
            // New file
            timestamps[path] = lastWriteTime;
            QueueEvent({path, FileAction::Added, std::chrono::system_clock::now()});
        } else if (it->second != lastWriteTime) {
            // Modified file
            it->second = lastWriteTime;
            QueueEvent({path, FileAction::Modified, std::chrono::system_clock::now()});
        }
    };

    try {
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                checkFile(entry);
            }
        } else {
            for (const auto& entry : fs::directory_iterator(dir)) {
                checkFile(entry);
            }
        }
    } catch (const fs::filesystem_error& e) {
        LOG_CORE_ERROR("Filesystem error while watching {}: {}", dir.string(), e.what());
        return;
    }

    // Check for removed files
    Vector<fs::path> removedFiles;
    for (const auto& [path, time] : timestamps) {
        if (currentFiles.find(path) == currentFiles.end()) {
            removedFiles.push_back(path);
            QueueEvent({path, FileAction::Removed, std::chrono::system_clock::now()});
        }
    }

    for (const auto& path : removedFiles) {
        timestamps.erase(path);
    }
}

bool FileWatcher::PassesFilter(const fs::path& path) const {
    if (m_Filters.empty()) return true;

    String ext = path.extension().string();
    for (const auto& filter : m_Filters) {
        if (ext == filter) return true;
    }
    return false;
}

void FileWatcher::QueueEvent(const FileEvent& event) {
    // Debouncing - check if we've seen this file recently
    auto now = std::chrono::steady_clock::now();
    String pathStr = event.Path.string();

    auto it = m_LastEventTime.find(pathStr);
    if (it != m_LastEventTime.end()) {
        auto elapsed = std::chrono::duration<f32>(now - it->second).count();
        if (elapsed < m_DebounceTime) {
            return;  // Skip this event (too soon after last one)
        }
    }
    m_LastEventTime[pathStr] = now;

    std::lock_guard<std::mutex> lock(m_EventMutex);
    m_PendingEvents.push_back(event);
}

} // namespace Engine
