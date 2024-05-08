
#include "PCH.h"

#include "Core/OS.h"
#include "Engine/Engine.h"

#include <GLFW/glfw3.h>
#include <commdlg.h>
#include <codecvt>


namespace Silex
{
    std::wstring ToUTF16(const std::string& utf8)
    {
        if (utf8.empty())
            return std::wstring();

        int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
        std::wstring utf16(size, 0);

        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &utf16[0], size);

        return utf16;
    }

    std::string ToUTF8(const std::wstring& utf16)
    {
        if (utf16.empty())
            return std::string();

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.size(), NULL, 0, NULL, NULL);
        std::string utf8(size_needed, 0);

        WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.size(), &utf8[0], size_needed, NULL, NULL);

        return utf8;
    }


#if SL_PLATFORM_WINDOWS

    void OS::Init()
    {
#if SL_DEBUG
        // コンソールのエンコードを UTF8 にする
        context.defaultConsoleCP = GetConsoleOutputCP();
        SetConsoleOutputCP(65001);
#endif
        //CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        glfwInit();

        // コンソールの標準入出力ハンドル取得
        context.outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        context.instance     = GetModuleHandleW(NULL);


        // Windows OS バージョンを取得
        const auto hModule = LoadLibraryW(TEXT("ntdll.dll"));
        if (hModule)
        {
            const auto address = GetProcAddress(hModule, "RtlGetVersion");
            if (address)
            {
                using RtlGetVersionType = NTSTATUS(WINAPI*)(OSVERSIONINFOEXW*);
                const auto RtlGetVersion = reinterpret_cast<RtlGetVersionType>(address);

                OSVERSIONINFOEXW os = { sizeof(os) };
                if (SUCCEEDED(RtlGetVersion(&os)))
                {
                    context.osBuildNumber  = os.dwBuildNumber;
                    context.osVersionMinor = os.dwMinorVersion;
                    context.osVersionMajor = os.dwMajorVersion;
                }
            }

            FreeLibrary(hModule);
        }
    }


    void OS::Shutdown()
    {
#if SL_DEBUG
        SetConsoleCP(context.defaultConsoleCP);
        SetConsoleOutputCP(context.defaultConsoleCP);
#endif
        glfwTerminate();
        //CoUninitialize();
    }

    float OS::GetTime()
    {
        return (float)glfwGetTime();
    }

    void OS::Sleep(uint32 milliseconds)
    {
        ::Sleep(milliseconds);
    }

    void OS::SetConsoleAttribute(uint16 color)
    {
        SetConsoleTextAttribute(context.outputHandle, color);
    }

    std::string OS::OpenFile(const char* filter)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };

        OS::MemoryZero(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize  = sizeof(OPENFILENAME);
        ofn.hwndOwner    = Engine::Get()->GetWindow()->GetWindowHandle();
        ofn.lpstrFile    = szFile;
        ofn.nMaxFile     = sizeof(szFile);
        ofn.lpstrFilter  = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return {};
    }

    std::string OS::SaveFile(const char* filter, const char* extention)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };

        OS::MemoryZero(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize  = sizeof(OPENFILENAME);
        ofn.hwndOwner    = Engine::Get()->GetWindow()->GetWindowHandle();
        ofn.lpstrFile    = szFile;
        ofn.nMaxFile     = sizeof(szFile);
        ofn.lpstrFilter  = filter;
        ofn.lpstrDefExt  = extention;
        ofn.nFilterIndex = 1;
        ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return {};
    }

    // コンソールカラー
    // https://blog.csdn.net/Fdog_/article/details/103764196
    void OS::OutputConsole(uint8 color, const std::string& message)
    {
        static uint8 levels[6] = { 0xcf, 0x0c, 0x06, 0x02, 0x08, 0x03 };
        SetConsoleTextAttribute(context.outputHandle, levels[color]);
    
        // コンソール出力
        WriteConsole(context.outputHandle, message.c_str(), (uint32)message.size(), nullptr, nullptr);
    }

    void OS::OutputDebugConsole(const std::string& message)
    {
        OutputDebugString(ToUTF16(message).c_str());
    }

    void* OS::MemoryZero(void* block, uint64 size)
    {
        return memset(block, 0, size);
    }

    void* OS::MemorySet(void* dest, int32 value, uint64 size)
    {
        return memset(dest, value, size);
    }

    void* OS::MemoryCopy(void* dest, const void* source, uint64 size)
    {
        return memcpy(dest, source, size);
    }

#endif

}