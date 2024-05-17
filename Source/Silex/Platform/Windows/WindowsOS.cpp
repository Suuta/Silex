
#include "PCH.h"
#include "Core/Engine.h"
#include "Platform/Windows/WindowsOS.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Rendering/Vulkan/Windows/WindowsVulkanContext.h"

#include <GLFW/glfw3.h>
#include <dwmapi.h>


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


    // Windows Window
    static Window* CreateWindowsWindow(const WindowCreateInfo& createInfo)
    {
        return Memory::Allocate<WindowsWindow>(createInfo);
    }

    // Windows VulkanContext
    static RenderingContext* CreateVulkanRenderContext()
    {
        return Memory::Allocate<WindowsVulkanContext>();
    }



    WindowsOS::WindowsOS()
    {
        CHECK_HRESULT(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    }

    WindowsOS::~WindowsOS()
    {
        ::CoUninitialize();
    }

    void WindowsOS::Run()
    {
        while (true)
        {
            Window::Get()->PumpMessage();
            if (!Engine::Get()->MainLoop())
            {
                break;
            }
        }
    }

    void WindowsOS::Initialize()
    {
#if SL_DEBUG
        // メモリリークトラッカ有効化
        SL_TRACK_CRT_MEMORY_LEAK();

        // コンソールのエンコードを UTF8 にする
        defaultConsoleCP = GetConsoleOutputCP();
        SetConsoleOutputCP(65001);

        // コンソールの標準入出力ハンドル取得
        outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

        // Windows OS バージョンを取得
        CheckOSVersion();

        // クロックカウンター初期化
        ::timeBeginPeriod(1);
        ::QueryPerformanceFrequency((LARGE_INTEGER*)&tickPerSecond);
        ::QueryPerformanceCounter((LARGE_INTEGER*)&startTickCount);

        // glfw 初期化
        ::glfwInit();

        // 各プラットフォーム生成関数を登録
        Window::RegisterCreateFunction(&CreateWindowsWindow);
        RenderingContext::ResisterCreateFunction(&CreateVulkanRenderContext);
    }

    void WindowsOS::Finalize()
    {
#if SL_DEBUG
        SetConsoleOutputCP(defaultConsoleCP);
#endif

        ::glfwTerminate();
        ::timeEndPeriod(1);
    }

    uint64 WindowsOS::GetTickSeconds()
    {
        uint64 tick;
        ::QueryPerformanceCounter((LARGE_INTEGER*)&tick);

        tick -= startTickCount;

        uint64 μsec     = 1'000'000;
        uint64 seconds  = (tick / tickPerSecond) * μsec;                 // 整数部
        uint64 decimal  = (tick % tickPerSecond) * μsec / tickPerSecond; // 小数点部

        return seconds + decimal;
    }

    void WindowsOS::Sleep(uint32 millisec)
    {
        ::Sleep(millisec);
    }

    std::string WindowsOS::OpenFile(const char* filter)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };

        std::memset(&ofn, 0, sizeof(OPENFILENAME));
        ofn.lStructSize  = sizeof(OPENFILENAME);
        ofn.hwndOwner    = (HWND)Window::Get()->GetWindowHandle();
        ofn.lpstrFile    = szFile;
        ofn.nMaxFile     = sizeof(szFile);
        ofn.lpstrFilter  = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (::GetOpenFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return {};
    }

    std::string WindowsOS::SaveFile(const char* filter, const char* extention)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };

        std::memset(&ofn, 0, sizeof(OPENFILENAME));
        ofn.lStructSize  = sizeof(OPENFILENAME);
        ofn.hwndOwner    = (HWND)Window::Get()->GetWindowHandle();
        ofn.lpstrFile    = szFile;
        ofn.nMaxFile     = sizeof(szFile);
        ofn.lpstrFilter  = filter;
        ofn.lpstrDefExt  = extention;
        ofn.nFilterIndex = 1;
        ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (::GetSaveFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return {};
    }

    void WindowsOS::SetConsoleAttribute(uint16 color)
    {
#if SL_DEBUG
        ::SetConsoleTextAttribute(outputHandle, color);
#endif
    }

    void WindowsOS::OutputConsole(uint8 color, const std::string& message)
    {
#if SL_DEBUG
        // コンソールカラー
        // https://blog.csdn.net/Fdog_/article/details/103764196
        static uint8 levels[6] = { 0xcf, 0x0c, 0x06, 0x02, 0x08, 0x03 };
        ::SetConsoleTextAttribute(outputHandle, levels[color]);

        // コンソール出力
        ::WriteConsoleW(outputHandle, message.c_str(), (uint32)message.size(), nullptr, nullptr);
#endif
    }

    void WindowsOS::OutputDebugConsole(const std::string& message)
    {
        // VisualStudio コンソール出力
        ::OutputDebugStringW(ToUTF16(message).c_str());
    }

    // Windows 11 以降　ウィンドウ角丸のスタイルの設定
    HRESULT WindowsOS::TrySetWindowCornerStyle(HWND hWnd, bool tryRound)
    {
        HRESULT hr = E_FAIL;

        //----------------------------------------------------------
        // NOTE:
        // Windows 11 (OS build: 22000) 以降をサポート
        // SDK は 10.0.22000.0 以降 (DWM_WINDOW_CORNER_PREFERENCE)
        //----------------------------------------------------------
        if (osBuildNumber >= 22000)
        {
#if 0
            const DWM_WINDOW_CORNER_PREFERENCE corner = tryRound ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
            hr = DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(DWM_WINDOW_CORNER_PREFERENCE));
#endif
            const int32 corner = tryRound ? 2 : 1;
            hr = DwmSetWindowAttribute(hWnd, 33, &corner, sizeof(int32));
        }

        return hr;
    }

    void WindowsOS::CheckOSVersion()
    {
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
                    osBuildNumber  = os.dwBuildNumber;
                    osVersionMinor = os.dwMinorVersion;
                    osVersionMajor = os.dwMajorVersion;
                }
            }

            FreeLibrary(hModule);
        }
    }
}
