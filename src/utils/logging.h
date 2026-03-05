#pragma once

#include <string>

namespace linkora::utils
{

    enum class LogLevel
    {
        Info,
        Warn,
        Error,
        Debug
    };

    void Log(LogLevel level, const std::string &message);

} // namespace linkora::utils
