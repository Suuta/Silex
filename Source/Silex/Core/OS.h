#pragma once

#include "Core/Core.h"


namespace Silex
{
    class OS
    {
    public:

        OS()
        {
            instance = this;
        }

        virtual ~OS()
        {
            instance = nullptr;
        }

        static OS* Get()
        {
            return instance;
        }

        virtual void Initialize() = 0;
        virtual void Finalize()   = 0;

        // 時間
        virtual uint64 GetTickSeconds()       = 0;
        virtual void   Sleep(uint32 millisec) = 0;

        // ファイル
        virtual std::string OpenFile(const char* filter = "All\0*.*\0")                                  = 0;
        virtual std::string SaveFile(const char* filter = "All\0*.*\0", const char* extention = nullptr) = 0;

        // コンソール
        virtual void SetConsoleAttribute(uint16 color)                      = 0;
        virtual void OutputConsole(uint8 color, const std::string& message) = 0;
        virtual void OutputDebugConsole(const std::string& message)         = 0;

        virtual void Run() = 0;

    protected:

        static inline OS* instance;
    };
}
