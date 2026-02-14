#pragma once

#include <fstream>
#include <mutex>
#include <string>

namespace core
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warn,
        Error,
    };

    class Logger
    {
    public:
        static Logger& Instance();

        void Log(LogLevel level, const char* className, const char* methodName, const std::string& message);

    private:
        Logger();
        ~Logger();

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        std::string BuildTimestamp() const;

    private:
        mutable std::mutex mutex_{};
        std::ofstream file_{};
    };
}

#define LOG_DEBUG(className, methodName, message) ::core::Logger::Instance().Log(::core::LogLevel::Debug, (className), (methodName), (message))
#define LOG_INFO(className, methodName, message) ::core::Logger::Instance().Log(::core::LogLevel::Info, (className), (methodName), (message))
#define LOG_WARN(className, methodName, message) ::core::Logger::Instance().Log(::core::LogLevel::Warn, (className), (methodName), (message))
#define LOG_ERROR(className, methodName, message) ::core::Logger::Instance().Log(::core::LogLevel::Error, (className), (methodName), (message))

#define LOG_METHOD(className, methodName) LOG_DEBUG((className), (methodName), "enter")
