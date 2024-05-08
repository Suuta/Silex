
#include "PCH.h"

#include "Core/OS.h"
#include "Editor/ConsoleLogger.h"


namespace Silex
{
    static std::mutex s_LogMutex;


    void Logger::Init()
    {
        OS::SetConsoleAttribute(8);
    }

    void Logger::Shutdown()
    {
        OS::SetConsoleAttribute(8);
    }

    void Logger::SetLogLevel(LogLevel level)
    {
        logFilter = level;
    }

    void Logger::Log(LogLevel level, const std::string& msg)
    {
        // ログレベルのフィルタリング
        if (level <= logFilter)
        {
#if SL_DEBUG
            switch (level)
            {
                case LogLevel::Fatal   : OS::OutputDebugConsole("[FATAL] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[FATAL] " + msg + "\n"); break;
                case LogLevel::Error   : OS::OutputDebugConsole("[ERROR] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[ERROR] " + msg + "\n"); break;
                case LogLevel::Warning : OS::OutputDebugConsole("[WARN ] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[WARN ] " + msg + "\n"); break;
                case LogLevel::Info    : OS::OutputDebugConsole("[INFO ] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[INFO ] " + msg + "\n"); break;
                case LogLevel::Trace   : OS::OutputDebugConsole("[TRACE] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[TRACE] " + msg + "\n"); break;
                case LogLevel::Debug   : OS::OutputDebugConsole("[DEBUG] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[DEBUG] " + msg + "\n"); break;

                default: break;
            }
#else
            switch (level)
            {
                case LogLevel::Fatal   : ConsoleLogger::Get().Log(level, "[FATAL] " + msg + "\n"); break;
                case LogLevel::Error   : ConsoleLogger::Get().Log(level, "[ERROR] " + msg + "\n"); break;
                case LogLevel::Warning : ConsoleLogger::Get().Log(level, "[WARN ] " + msg + "\n"); break;
                case LogLevel::Info    : ConsoleLogger::Get().Log(level, "[INFO ] " + msg + "\n"); break;
                case LogLevel::Trace   : ConsoleLogger::Get().Log(level, "[TRACE] " + msg + "\n"); break;
                case LogLevel::Debug   : ConsoleLogger::Get().Log(level, "[DEBUG] " + msg + "\n"); break;

                default: break;
            }
#endif
        }
    }
}
