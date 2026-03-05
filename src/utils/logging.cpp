#include "utils/logging.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace linkora::utils
{
    namespace
    {
        std::mutex gLogMutex;

        const char *LevelToText(LogLevel level)
        {
            switch (level)
            {
            case LogLevel::Info:
                return "INFO";
            case LogLevel::Warn:
                return "WARN";
            case LogLevel::Error:
                return "ERROR";
            case LogLevel::Debug:
                return "DEBUG";
            }
            return "UNKNOWN";
        }
    } // namespace

    void Log(LogLevel level, const std::string &message)
    {
        std::lock_guard<std::mutex> guard(gLogMutex);
        const auto now = std::chrono::system_clock::now();
        const std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::cout << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                  << " [" << LevelToText(level) << "] "
                  << message << '\n';
    }

} // namespace linkora::utils
