#pragma once

#include "core/Types.hpp"
#include <filesystem>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

namespace Engine {

namespace fs = std::filesystem;

// Type of file change detected
enum class FileAction : u8 {
    Added,
    Modified,
    Removed
};

// Event data for file changes
struct FileEvent {
    fs::path Path;
    FileAction Action;
    std::chrono::system_clock::time_point Timestamp;
};

// Callback type for file change notifications
using FileCallback = std::function<void(const FileEvent&)>;

// Cross-platform file watcher using std::filesystem
class FileWatcher {
public:
    FileWatcher();
    ~FileWatcher();

    // Add directory to watch (recursive by default)
    void Watch(const fs::path& path, bool recursive = true);

    // Remove directory from watch list
    void Unwatch(const fs::path& path);

    // Add file extension filter (e.g., ".glsl", ".lua")
    // If filters are set, only matching files trigger events
    void AddFilter(const String& extension);
    void ClearFilters();

    // Register callback for file changes
    void OnFileChanged(FileCallback callback);

    // Start/stop the watcher thread
    void Start();
    void Stop();

    // Poll for changes (call from main thread to process events)
    // Returns number of events processed
    u32 Poll();

    // Configuration
    void SetPollInterval(f32 seconds) { m_PollInterval = seconds; }
    void SetDebounceTime(f32 seconds) { m_DebounceTime = seconds; }

    // Status
    bool IsRunning() const { return m_Running.load(); }
    usize GetWatchedDirectoryCount() const { return m_WatchedDirs.size(); }

private:
    void WatcherThread();
    bool PassesFilter(const fs::path& path) const;
    void QueueEvent(const FileEvent& event);
    void CheckDirectory(const fs::path& dir, bool recursive);

private:
    struct WatchedDirectory {
        fs::path Path;
        bool Recursive;
        HashMap<fs::path, fs::file_time_type> FileTimestamps;
    };

    Vector<WatchedDirectory> m_WatchedDirs;
    Vector<String> m_Filters;
    Vector<FileCallback> m_Callbacks;

    // Event queue (thread-safe)
    Vector<FileEvent> m_PendingEvents;
    std::mutex m_EventMutex;

    // Debouncing - prevent multiple events for same file in short time
    HashMap<String, std::chrono::steady_clock::time_point> m_LastEventTime;
    f32 m_DebounceTime = 0.1f;  // 100ms default

    // Thread control
    std::thread m_WatcherThread;
    std::atomic<bool> m_Running{false};
    f32 m_PollInterval = 0.5f;  // 500ms default

    std::mutex m_DirMutex;
};

} // namespace Engine
