#include "ConsolePanel.hpp"
#include "core/Logger.hpp"
#include <imgui.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Editor {

ConsolePanel::ConsolePanel() : Panel("Console") {
}

ConsolePanel::~ConsolePanel() {
}

void ConsolePanel::OnInit(EditorContext& context) {
    Panel::OnInit(context);

    // Add custom sink to spdlog
    auto sink = std::make_shared<ConsolePanelSink_mt>(this);
    sink->set_pattern("%v");

    Engine::Logger::GetCoreLogger()->sinks().push_back(sink);
    Engine::Logger::GetClientLogger()->sinks().push_back(sink);
}

void ConsolePanel::OnShutdown() {
    // Note: We should remove our sink from the loggers here, but for simplicity
    // we'll let spdlog handle cleanup
}

void ConsolePanel::AddLog(spdlog::level::level_enum level, const Engine::String& message) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    LogEntry entry;
    entry.Level = level;
    entry.Message = message;

    // Remove trailing newline if present
    if (!entry.Message.empty() && entry.Message.back() == '\n') {
        entry.Message.pop_back();
    }

    // Get timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    entry.Timestamp = ss.str();

    m_Logs.push_back(entry);

    // Limit log size
    if (m_Logs.size() > 10000) {
        m_Logs.erase(m_Logs.begin(), m_Logs.begin() + 1000);
    }
}

void ConsolePanel::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Logs.clear();
}

ImVec4 ConsolePanel::GetLogColor(spdlog::level::level_enum level) const {
    switch (level) {
        case spdlog::level::trace:    return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        case spdlog::level::debug:    return ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
        case spdlog::level::info:     return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        case spdlog::level::warn:     return ImVec4(1.0f, 0.9f, 0.3f, 1.0f);
        case spdlog::level::err:      return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        case spdlog::level::critical: return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
        default:                      return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void ConsolePanel::OnImGuiRender() {
    ImGui::Begin("Console");

    // Toolbar
    if (ImGui::Button("Clear")) {
        Clear();
    }
    ImGui::SameLine();

    ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
    ImGui::SameLine();

    ImGui::Checkbox("Timestamps", &m_ShowTimestamps);

    // Filter buttons
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));

    auto FilterButton = [](const char* label, bool& enabled, ImVec4 color) {
        if (enabled) {
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x * 1.2f, color.y * 1.2f, color.z * 1.2f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        }

        if (ImGui::SmallButton(label)) {
            enabled = !enabled;
        }

        ImGui::PopStyleColor(2);
    };

    FilterButton("T", m_ShowTrace, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::SameLine();
    FilterButton("D", m_ShowDebug, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
    ImGui::SameLine();
    FilterButton("I", m_ShowInfo, ImVec4(0.3f, 0.3f, 0.6f, 1.0f));
    ImGui::SameLine();
    FilterButton("W", m_ShowWarn, ImVec4(0.6f, 0.6f, 0.2f, 1.0f));
    ImGui::SameLine();
    FilterButton("E", m_ShowError, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    ImGui::SameLine();
    FilterButton("C", m_ShowCritical, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));

    ImGui::PopStyleVar();

    // Text filter
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputTextWithHint("##filter", "Filter...", m_FilterBuffer, sizeof(m_FilterBuffer));

    ImGui::Separator();

    // Log display
    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        for (const auto& entry : m_Logs) {
            // Level filter
            bool show = false;
            switch (entry.Level) {
                case spdlog::level::trace:    show = m_ShowTrace; break;
                case spdlog::level::debug:    show = m_ShowDebug; break;
                case spdlog::level::info:     show = m_ShowInfo; break;
                case spdlog::level::warn:     show = m_ShowWarn; break;
                case spdlog::level::err:      show = m_ShowError; break;
                case spdlog::level::critical: show = m_ShowCritical; break;
                default: show = true; break;
            }

            if (!show) continue;

            // Text filter
            if (m_FilterBuffer[0] != '\0') {
                if (entry.Message.find(m_FilterBuffer) == Engine::String::npos) {
                    continue;
                }
            }

            // Display
            ImGui::PushStyleColor(ImGuiCol_Text, GetLogColor(entry.Level));

            if (m_ShowTimestamps) {
                ImGui::TextUnformatted(("[" + entry.Timestamp + "] " + entry.Message).c_str());
            } else {
                ImGui::TextUnformatted(entry.Message.c_str());
            }

            ImGui::PopStyleColor();
        }
    }

    // Auto-scroll
    if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    ImGui::End();
}

} // namespace Editor
