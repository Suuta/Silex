#pragma once

#include "Core/Core.h"
#include <string>


namespace Silex
{
    struct PlatformContext
    {
        void*  outputHandle;
        void*  errorHandle;
        void*  instance;
        uint32 defaultConsoleCP;

        uint32 osVersionMajor;
        uint32 osVersionMinor;
        uint32 osBuildNumber;
    };


    class OS
    {
    public:

        static void Init();
        static void Shutdown();

        static float GetTime();

        static void  Sleep(uint32 milliseconds);

        static void  SetConsoleAttribute(uint16 color);
        static void  OutputConsole(uint8 color, const std::string& message);
        static void  OutputDebugConsole(const std::string& message);

        static void* MemoryZero(void* block, uint64 size);
        static void* MemoryCopy(void* dest, const void* source, uint64 size);
        static void* MemorySet(void* dest, int32 value, uint64 size);

        static std::string OpenFile(const char* filter = "All\0*.*\0");
        static std::string SaveFile(const char* filter = "All\0*.*\0", const char* extention = nullptr);

        static void             SetContext(const PlatformContext& pc) { context = pc; }
        static PlatformContext& GetContext()                          { return context;    }

    private:

        static inline PlatformContext context;
    };



#if 1
    namespace Windows
    {
        // S_OK             操作に成功しました
        // E_ABORT          操作は中止されました
        // E_ACCESSDENIED   一般的なアクセス拒否エラーが発生しました
        // E_FAIL           不特定のエラー
        // E_HANDLE         無効なハンドル
        // E_INVALIDARG     1つ以上の引数が無効です
        // E_NOINTERFACE    そのようなインターフェイスはサポートされていません
        // E_NOTIMPL        未実装
        // E_OUTOFMEMORY    必要なメモリの割り当てに失敗しました
        // E_POINTER        無効なポインター
        // E_UNEXPECTED     予期しないエラー

        // HRESULT の値
        //https://learn.microsoft.com/en-us/windows/win32/seccrypto/common-hresult-values
        inline const char* HRESULTToString(HRESULT result)
        {
            switch (result)
            {
                case S_OK           : return "S_OK";
                case E_ABORT        : return "E_ABORT";
                case E_ACCESSDENIED : return "E_ACCESSDENIED";
                case E_FAIL         : return "E_FAIL";
                case E_HANDLE       : return "E_HANDLE";
                case E_INVALIDARG   : return "E_INVALIDARG";
                case E_NOINTERFACE  : return "E_NOINTERFACE";
                case E_NOTIMPL      : return "E_NOTIMPL";
                case E_OUTOFMEMORY  : return "E_OUTOFMEMORY";
                case E_POINTER      : return "E_POINTER";
                case E_UNEXPECTED   : return "E_UNEXPECTED";

                default: break;
            }

            SL_ASSERT(false);
            return nullptr;
        }

        inline void Win32CheckResult(HRESULT result)
        {
            if (result != S_OK)
            {
                SL_LOG_ERROR("HRESULT: '{0}' - {1}:{2}", Windows::HRESULTToString(result), __FILE__, __LINE__);
                SL_ASSERT(false);
            }
        }
    }

#define CHECK_HRESULT(func) \
    {\
        HRESULT _result = func;\
        Silex::Windows::Win32CheckResult(_result);\
    }

#endif
}
