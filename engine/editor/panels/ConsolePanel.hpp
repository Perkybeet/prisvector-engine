#pragma once

#include "Panel.hpp"
#include <imgui.h>
#include <spdlog/sinks/base_sink.h>
#include <mutex>

namespace Editor {

struct LogEntry {
    spdlog::level::level_enum Level;
    Engine::String Message;
    Engine::String Timestamp;
};

class ConsolePanel : public Panel {
public:
    ConsolePanel();
    ~ConsolePanel();

    void OnInit(EditorContext& context) override;
    void OnShutdown() override;
    void OnImGuiRender() override;

    void AddLog(spdlog::level::level_enum level, const Engine::String& message);
    void Clear();

private:
    ImVec4 GetLogColor(spdlog::level::level_enum level) const;

    Engine::Vector<LogEntry> m_Logs;
    bool m_AutoScroll = true;
    bool m_ShowTimestamps = false;

    // Filters
    bool m_ShowTrace = true;
    bool m_ShowDebug = true;
    bool m_ShowInfo = true;
    bool m_ShowWarn = true;
    bool m_ShowError = true;
    bool m_ShowCritical = true;

    char m_FilterBuffer[256] = "";

    std::mutex m_Mutex;
};

// Custom spdlog sink that forwards to ConsolePanel
template<typename Mutex>
class ConsolePanelSink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit ConsolePanelSink(ConsolePanel* panel) : m_Panel(panel) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        m_Panel->AddLog(msg.level, fmt::to_string(formatted));
    }

    void flush_() override {}

private:
    ConsolePanel* m_Panel;
};

using ConsolePanelSink_mt = ConsolePanelSink<std::mutex>;

} // namespace Editor
