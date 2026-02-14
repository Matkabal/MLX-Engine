#include "core/Logger.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace core
{
    Logger& Logger::Instance()
    {
        static Logger logger;
        return logger;
    }

    Logger::Logger()
    {
        std::error_code ec;
        std::filesystem::create_directories("log", ec);
        file_.open("log/engine.log", std::ios::out | std::ios::app);
    }

    Logger::~Logger()
    {
        if (file_.is_open())
        {
            file_.flush();
            file_.close();
        }
    }

    std::string Logger::BuildTimestamp() const
    {
        using clock = std::chrono::system_clock;
        const auto now = clock::now();
        const std::time_t t = clock::to_time_t(now);

        std::tm tmNow{};
#if defined(_WIN32)
        localtime_s(&tmNow, &t);
#else
        localtime_r(&t, &tmNow);
#endif

        std::ostringstream out;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        out << std::put_time(&tmNow, "%Y-%m-%d %H:%M:%S")
            << "."
            << std::setfill('0') << std::setw(3) << ms.count();
        return out.str();
    }

    void Logger::Log(LogLevel level, const char* className, const char* methodName, const std::string& message)
    {
        const char* levelText = "INFO";
        switch (level)
        {
        case LogLevel::Debug:
            levelText = "DEBUG";
            break;
        case LogLevel::Info:
            levelText = "INFO";
            break;
        case LogLevel::Warn:
            levelText = "WARN";
            break;
        case LogLevel::Error:
            levelText = "ERROR";
            break;
        }

        const std::string timestamp = BuildTimestamp();
        std::ostringstream line;
        line << timestamp
             << " [" << levelText << "]"
             << " [thread:" << std::this_thread::get_id() << "]"
             << " [" << (className ? className : "unknown") << "::" << (methodName ? methodName : "unknown") << "] "
             << message;

        const std::string text = line.str();

        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << text << std::endl;
        if (file_.is_open())
        {
            file_ << text << '\n';
            file_.flush();
        }
    }
}
