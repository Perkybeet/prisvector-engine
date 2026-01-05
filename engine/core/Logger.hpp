#pragma once

#include <spdlog/spdlog.h>
#include <memory>

namespace Engine {

class Logger {
public:
    static void Init();

    static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

} // namespace Engine

// Core logging macros
#define LOG_CORE_TRACE(...)    ::Engine::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_CORE_DEBUG(...)    ::Engine::Logger::GetCoreLogger()->debug(__VA_ARGS__)
#define LOG_CORE_INFO(...)     ::Engine::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_CORE_WARN(...)     ::Engine::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_CORE_ERROR(...)    ::Engine::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define LOG_CORE_CRITICAL(...) ::Engine::Logger::GetCoreLogger()->critical(__VA_ARGS__)

// Client logging macros
#define LOG_TRACE(...)    ::Engine::Logger::GetClientLogger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::Engine::Logger::GetClientLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...)     ::Engine::Logger::GetClientLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)     ::Engine::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::Engine::Logger::GetClientLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::Engine::Logger::GetClientLogger()->critical(__VA_ARGS__)
