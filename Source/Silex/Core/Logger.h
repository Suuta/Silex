
#pragma once

#include "Core/CoreType.h"
#include "Core/Assert.h"

//==========================================
// C++17内で std::format の代用として fmt を使用
//==========================================
//#include <fmt/format.h>

#include <format>
#include <string>



namespace Silex
{
    enum class LogLevel
    {
        Fatal,
        Error,
        Warning,
        Info,
        Trace,
        Debug,

        Count,
    };

    class Logger
    {
    public:

        static void Init();
        static void Shutdown();
        static void SetLogLevel(LogLevel level);

        static void Log(LogLevel level, const std::string& message);

    private:

        static inline LogLevel logFilter = LogLevel::Debug;
    };
}


#if 0

#define SL_LOG_FATAL(...) Silex::Logger::Log(Silex::LogLevel::Fatal,   fmt::format(__VA_ARGS__))
#define SL_LOG_ERROR(...) Silex::Logger::Log(Silex::LogLevel::Error,   fmt::format(__VA_ARGS__))
#define SL_LOG_WARN(...)  Silex::Logger::Log(Silex::LogLevel::Warning, fmt::format(__VA_ARGS__))
#define SL_LOG_INFO(...)  Silex::Logger::Log(Silex::LogLevel::Info,    fmt::format(__VA_ARGS__))
#define SL_LOG_TRACE(...) Silex::Logger::Log(Silex::LogLevel::Trace,   fmt::format(__VA_ARGS__))
#define SL_LOG_DEBUG(...) Silex::Logger::Log(Silex::LogLevel::Debug,   fmt::format(__VA_ARGS__))

#define SL_LOG(level, ...) { Silex::Logger::Log(level, fmt::format(__VA_ARGS__)); }

#else

#define SL_LOG_FATAL(...) Silex::Logger::Log(Silex::LogLevel::Fatal,   std::format(__VA_ARGS__))
#define SL_LOG_ERROR(...) Silex::Logger::Log(Silex::LogLevel::Error,   std::format(__VA_ARGS__))
#define SL_LOG_WARN(...)  Silex::Logger::Log(Silex::LogLevel::Warning, std::format(__VA_ARGS__))
#define SL_LOG_INFO(...)  Silex::Logger::Log(Silex::LogLevel::Info,    std::format(__VA_ARGS__))
#define SL_LOG_TRACE(...) Silex::Logger::Log(Silex::LogLevel::Trace,   std::format(__VA_ARGS__))
#define SL_LOG_DEBUG(...) Silex::Logger::Log(Silex::LogLevel::Debug,   std::format(__VA_ARGS__))

#define SL_LOG(level, ...) { Silex::Logger::Log(level, std::format(__VA_ARGS__)); }

#endif
