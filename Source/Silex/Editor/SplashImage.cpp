//===============================================================================
// UnrealEngine 5.2 の　FSplashScreen を 参照
// Engine/Source/Runtime/ApplicationCore/Public/Windows/WindowsPlatformSplash.h
//===============================================================================

#include "PCH.h"

#include "Editor/SplashImage.h"
#include "Engine/Engine.h"
#include "Core/OS.h"

#include <strsafe.h>
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")

#pragma warning(disable: 4267)
#pragma warning(disable: 4244)


namespace Silex
{
    static HINSTANCE hInstance;

    // バックグラウンド
    static HANDLE       s_Thread   = NULL;
    static HBITMAP      s_Bitmap   = NULL; // Unrealと同サイズの 720 x 370 を使用
    static HWND         s_Wnd      = NULL;
    static HWND         s_GuardWnd = NULL;
    static std::wstring s_AppName;
    static std::wstring s_Text[SplashTextType::NumTextTypes];
    static RECT         s_TextRects[SplashTextType::NumTextTypes];
    static HFONT        s_TextFont                  = NULL;
    static HFONT        s_TitleFont                 = NULL;
    static bool         s_AllowFading  = false;
    static float        s_FadeDuration = 0.5f;

    // プログレスバー
    static int          s_Progress       = 0;
    static HBITMAP      s_ProgressBitmap = NULL;
    static UINT_PTR     s_TimerID        = NULL;
    static std::mutex   s_Mutex;

#if 1
    // DrawStateW の コールバック
    // WM_PAINT と同時に呼ばれる？と推測されるので、毎フレーム更新の場合は
    // 意図的にウィンドウに変化を与える必要があるかもしれない
    BOOL CALLBACK DrawStateProc(HDC hdc, LPARAM lData, WPARAM wData, int cx, int cy)
    {
        HBITMAP hBitmap = (HBITMAP)lData;
        HDC hdcMem = CreateCompatibleDC(hdc);
        SelectObject(hdcMem, hBitmap);

        BitBlt(hdc, 0, 0, cx, cy, hdcMem, 0, 0, SRCCOPY);

        DeleteDC(hdcMem);

        return TRUE;
    }
#endif


    LRESULT CALLBACK SplashScreenWindowProc(HWND hWnd, uint32 message, WPARAM wParam, LPARAM lParam)
    {
        HDC hdc;
        PAINTSTRUCT ps;

        switch (message)
        {
            case WM_CREATE:
            {
                // elapsed == (ms)
                SetTimer(hWnd, s_TimerID, 33, NULL);
            }

            case WM_TIMER:
            {
                
            }

            case WM_PAINT:
            {
                hdc = BeginPaint(hWnd, &ps);

                // バックグラウンド
                DrawState(hdc, NULL, NULL, (LPARAM)s_Bitmap, NULL, 0, 0, 0, 0, DST_BITMAP);

                // プログレスバー
                DrawState(hdc, NULL, NULL, (LPARAM)s_ProgressBitmap, NULL, 0, 270, 1 + s_Progress, 275, DST_BITMAP);

                // 文字列
                for (int32 CurTypeIndex = 0; CurTypeIndex < SplashTextType::NumTextTypes; ++CurTypeIndex)
                {
                    const std::wstring& SplashText = s_Text[CurTypeIndex];
                    const RECT& TextRect           = s_TextRects[CurTypeIndex];

                    if (!SplashText.empty())
                    {
                        COLORREF title = RGB(255, 255, 255);
                        COLORREF text  = RGB(160, 160, 160);
                        COLORREF black = RGB(  0,   0,   0);

                        SelectObject(hdc, CurTypeIndex == SplashTextType::AppName? s_TitleFont : s_TextFont);
                        SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

                        SetBkColor(hdc, black);
                        SetBkMode(hdc, TRANSPARENT);

                        RECT ClientRect;
                        GetClientRect(hWnd, &ClientRect);

                        SetTextColor(hdc, CurTypeIndex == SplashTextType::AppName? title : text);

                        TextOut(hdc, TextRect.left, TextRect.top, SplashText.c_str(), SplashText.length());
                    }
                }

                EndPaint(hWnd, &ps);

                break;
            }

            case WM_DESTROY:
            {
                KillTimer(hWnd, s_TimerID);
                PostQuitMessage(0);
                break;
            }

            default: return DefWindowProc(hWnd, message, wParam, lParam);
        }

        return 0;
    }

    HBITMAP LoadBitmapFile(const std::wstring& filePath)
    {
        HRESULT hr = CoInitialize(NULL);

        IWICImagingFactory* Factory = NULL;

        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&Factory)
        );

        IWICBitmapDecoder* Decoder = NULL;

        hr = Factory->CreateDecoderFromFilename(
            filePath.c_str(),
            NULL,                         
            GENERIC_READ,                 
            WICDecodeMetadataCacheOnDemand,
            &Decoder                      
        );

        IWICBitmapFrameDecode* Frame = NULL;

        if (SUCCEEDED(hr))
        {
            hr = Decoder->GetFrame(0, &Frame);
        }

        IWICBitmapSource* OriginalBitmapSource = NULL;
        if (SUCCEEDED(hr))
        {
            hr = Frame->QueryInterface(IID_IWICBitmapSource, reinterpret_cast<void**>(&OriginalBitmapSource));
        }

        IWICBitmapSource* ToRenderBitmapSource = NULL;

        if (SUCCEEDED(hr))
        {
            IWICFormatConverter* Converter = NULL;
            hr = Factory->CreateFormatConverter(&Converter);

            if (SUCCEEDED(hr))
            {
                hr = Converter->Initialize(
                    Frame,                      
                    GUID_WICPixelFormat32bppBGR,
                    WICBitmapDitherTypeNone,    
                    NULL,                        
                    0.f,                        
                    WICBitmapPaletteTypeCustom  
                );

                if (SUCCEEDED(hr))
                {
                    hr = Converter->QueryInterface(IID_PPV_ARGS(&ToRenderBitmapSource));
                }
            }

            Converter->Release();
        }

        HBITMAP hDIBBitmap = 0;
        if (SUCCEEDED(hr))
        {
            UINT width = 0;
            UINT height = 0;

            void* ImageBits = NULL;

            WICPixelFormatGUID pixelFormat;
            hr = ToRenderBitmapSource->GetPixelFormat(&pixelFormat);

            if (SUCCEEDED(hr))
            {
                hr = (pixelFormat == GUID_WICPixelFormat32bppBGR) ? S_OK : E_FAIL;
            }

            if (SUCCEEDED(hr))
            {
                hr = ToRenderBitmapSource->GetSize(&width, &height);
            }

            if (SUCCEEDED(hr))
            {
                BITMAPINFO bminfo;
                ZeroMemory(&bminfo, sizeof(bminfo));
                bminfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
                bminfo.bmiHeader.biWidth       = width;
                bminfo.bmiHeader.biHeight      = -(LONG)height;
                bminfo.bmiHeader.biPlanes      = 1;
                bminfo.bmiHeader.biBitCount    = 32;
                bminfo.bmiHeader.biCompression = BI_RGB;

                HDC hdcScreen = GetDC(NULL);
                hr = hdcScreen ? S_OK : E_FAIL;

                if (SUCCEEDED(hr))
                {
                    if (hDIBBitmap)
                    {
                        DeleteObject(hDIBBitmap);
                    }

                    hDIBBitmap = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &ImageBits, NULL, 0);

                    ReleaseDC(NULL, hdcScreen);

                    hr = hDIBBitmap ? S_OK : E_FAIL;
                }
            }

            UINT cbStride = 0;
            if (SUCCEEDED(hr))
            {
                hr = UIntMult(width, sizeof(DWORD), &cbStride);
            }

            UINT cbImage = 0;
            if (SUCCEEDED(hr))
            {
                hr = UIntMult(cbStride, height, &cbImage);
            }

            if (SUCCEEDED(hr) && ToRenderBitmapSource)
            {
                hr = ToRenderBitmapSource->CopyPixels(
                    NULL,
                    cbStride,
                    cbImage,
                    reinterpret_cast<BYTE*>(ImageBits));
            }

            if (FAILED(hr) && hDIBBitmap)
            {
                DeleteObject(hDIBBitmap);
                hDIBBitmap = NULL;
            }
        }

        if (OriginalBitmapSource)
        {
            OriginalBitmapSource->Release();
        }

        if (ToRenderBitmapSource)
        {
            ToRenderBitmapSource->Release();
        }

        if (Decoder)
        {
            Decoder->Release();
        }

        if (Frame)
        {
            Frame->Release();
        }

        if (Factory)
        {
            Factory->Release();
        }

        return hDIBBitmap;
    }

    uint32 WINAPI StartSplashScreenThread(LPVOID unused)
    {
        WNDCLASS wc;
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = (WNDPROC)SplashScreenWindowProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
        wc.hCursor       = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = TEXT("SplashScreenClass");

        BITMAP bm;
        memset(&bm, 0, sizeof(bm));

        {
            if (!RegisterClass(&wc))
            {
                return 0;
            }

            s_Bitmap         = LoadBitmapFile(L"Assets/Editor/Splash.png");
            s_ProgressBitmap = LoadBitmapFile(L"Assets/Editor/Progress.png");

            if (s_Bitmap)
            {
                GetObjectW(s_Bitmap, sizeof(bm), &bm);
                const int32 WindowWidth = bm.bmWidth;
                const int32 WindowHeight = bm.bmHeight;
                int32 ScreenPosX = (GetSystemMetrics(SM_CXSCREEN) - WindowWidth) / 2;
                int32 ScreenPosY = (GetSystemMetrics(SM_CYSCREEN) - WindowHeight) / 2;

                uint32 dwWindowStyle = WS_EX_APPWINDOW;
                if (s_AllowFading)
                {
                    dwWindowStyle |= WS_EX_LAYERED;
                }

                s_Wnd = CreateWindowExW(
                    dwWindowStyle,
                    wc.lpszClassName,
                    TEXT("SplashScreen"),
                    WS_POPUP,
                    ScreenPosX,
                    ScreenPosY,
                    WindowWidth,
                    WindowHeight,
                    (HWND)NULL,
                    (HMENU)NULL,
                    (HINSTANCE)hInstance,
                    (LPVOID)NULL);
            }
        }

        if (s_Wnd)
        {
            assert(s_Bitmap);

            if (s_AllowFading)
            {
                // フェード用に透明にする
                SetLayeredWindowAttributes(s_Wnd, 0, 0, LWA_ALPHA);
            }

            {
                AddFontResourceW(L"Assets/Fonts/NotoSansJP-Regular.ttf");

                {
                    LOGFONT MyFont;
                    memset(&MyFont, 0, sizeof(MyFont));
                    wcscpy(MyFont.lfFaceName, L"NotoSansJP-Regular");

                    MyFont.lfHeight  = 15;
                    MyFont.lfQuality = CLEARTYPE_QUALITY;

                    s_TextFont = CreateFontIndirect(&MyFont);
                }

                {
                    LOGFONT MyFont;
                    memset(&MyFont, 0, sizeof(MyFont));
                    wcscpy(MyFont.lfFaceName, L"NotoSansJP-Regular");

                    MyFont.lfHeight  = 25;
                    MyFont.lfWeight  = FW_BOLD;
                    MyFont.lfQuality = CLEARTYPE_QUALITY;

                    s_TitleFont = CreateFontIndirect(&MyFont);
                }
            }

            // アプリ名
            s_TextRects[SplashTextType::AppName].top           = bm.bmHeight - 80;
            s_TextRects[SplashTextType::AppName].bottom        = bm.bmHeight - 60;
            s_TextRects[SplashTextType::AppName].left          = 100;
            s_TextRects[SplashTextType::AppName].right         = bm.bmWidth - 20;

            // バージョン情報
            s_TextRects[SplashTextType::VersionInfo].top       = bm.bmHeight - 50;
            s_TextRects[SplashTextType::VersionInfo].bottom    = bm.bmHeight - 30;
            s_TextRects[SplashTextType::VersionInfo].left      = 100;
            s_TextRects[SplashTextType::VersionInfo].right     = bm.bmWidth - 20;

            // プログレステキスト
            s_TextRects[SplashTextType::StartupProgress].top    = bm.bmHeight - 30;
            s_TextRects[SplashTextType::StartupProgress].bottom = bm.bmHeight - 10;
            s_TextRects[SplashTextType::StartupProgress].left   = 100;
            s_TextRects[SplashTextType::StartupProgress].right  = bm.bmWidth - 20;

            // コピーライト表示
            s_TextRects[SplashTextType::CopyrightInfo].top      = bm.bmHeight - 25;
            s_TextRects[SplashTextType::CopyrightInfo].bottom   = bm.bmHeight - 15;
            s_TextRects[SplashTextType::CopyrightInfo].left     = bm.bmWidth  - 100;
            s_TextRects[SplashTextType::CopyrightInfo].right    = bm.bmWidth  - 20;


            SetWindowText(s_Wnd, s_AppName.c_str());
            ShowWindow(s_Wnd, SW_SHOW);
            UpdateWindow(s_Wnd);

            const float fadeStartTime = OS::GetTime();
            byte currentOpacityByte = 0;


            MSG message;
            bool bIsSplashFinished = false;
            while (!bIsSplashFinished)
            {
                if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);

                    if (message.message == WM_QUIT)
                    {
                        bIsSplashFinished = true;
                    }
                }

                // フェード
                if (s_AllowFading && currentOpacityByte < 255)
                {
                    const float timeSinceFadeStart = OS::GetTime() - fadeStartTime;
                    const float FadeAmount         = std::clamp(timeSinceFadeStart / s_FadeDuration, 0.0f, 2.0f);
                    const int32 newOpacityByte     = (int32)(255 * FadeAmount);
                    if (newOpacityByte != currentOpacityByte)
                    {
                        currentOpacityByte = newOpacityByte;
                        SetLayeredWindowAttributes(s_Wnd, NULL, currentOpacityByte, LWA_ALPHA);
                    }
                }
            }

            DeleteObject(s_Bitmap);
            s_Bitmap = NULL;

            DeleteObject(s_ProgressBitmap);
            s_ProgressBitmap = NULL;
        }

        UnregisterClass(wc.lpszClassName, (HINSTANCE)hInstance);
        return 0;
    }

    static void StartSetSplashText(const SplashTextType::Type InType, const wchar_t* text)
    {
        s_Text[InType] = text;
    }

    void SplashImage::Show(bool bFading)
    {
        s_AllowFading = bFading;
        s_AppName     = TEXT("Silex");

        StartSetSplashText(SplashTextType::StartupProgress, L"Initialize Renderer...");
        StartSetSplashText(SplashTextType::AppName,         L"Silex");
        StartSetSplashText(SplashTextType::VersionInfo,     L"1.0");
        StartSetSplashText(SplashTextType::CopyrightInfo,   L"@ Sugiyama Shota");

#if 1
        s_GuardWnd = CreateWindowExA(
            NULL,
            "STATIC",
            "SplashScreenGuard",
            0,
            0,
            0,
            0,
            0,
            HWND_MESSAGE,
            (HMENU)NULL,
            (HINSTANCE)hInstance,
            (LPVOID)NULL);

        if (s_GuardWnd)
        {
            ShowWindow(s_GuardWnd, SW_SHOW);
        }
#endif

        DWORD ThreadID = 0;
        s_Thread = CreateThread(NULL, 1024 * 1024, (LPTHREAD_START_ROUTINE)StartSplashScreenThread, (LPVOID)NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, &ThreadID);
    }

    void SplashImage::Hide()
    {
        if (s_Thread)
        {
            // ウィンドウ破棄
            if (s_Wnd)
            {
                PostMessageW(s_Wnd, WM_CLOSE, 0, 0);
                s_Wnd = NULL;
            }

            // スレッド停止
            WaitForSingleObject(s_Thread, INFINITE);
            CloseHandle(s_Thread);
            s_Thread = NULL;

            // ウィンドウを閉じる
            if (s_GuardWnd)
            {
                PostMessageW(s_GuardWnd, WM_DESTROY, 0, 0);
                s_GuardWnd = NULL;
            }
        }

        // フォント解放
        RemoveFontResourceW(L"Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    bool SplashImage::IsShown()
    {
        return (s_Thread != nullptr);
    }

    void SplashImage::SetLoadProgressPercentage(float percentage)
    {
        s_Progress = 720.f * (percentage / 100.0f);
        RECT rect = { 0, 270, 720, 275 };
        InvalidateRect(s_Wnd, &rect, FALSE); // WM_PAINT をトリガーさせる
    }

    void SplashImage::SetText(const wchar_t* InText)
    {
        SetText(SplashTextType::StartupProgress, InText);
    }

    void SplashImage::SetText(const wchar_t* InText, float percentage)
    {
        SetText(SplashTextType::StartupProgress, InText);
        SetLoadProgressPercentage(percentage);
    }

    void SplashImage::SetText(const SplashTextType::Type InType, const wchar_t* InText)
    {
        std::wstring InTextW = InText;

        if (s_Thread)
        {
            bool bWasUpdated = false;
            {
                if (InTextW != s_Text[InType].c_str())
                {
                    s_Text[InType] = InTextW;
                    bWasUpdated = true;
                }
            }

            if (bWasUpdated)
            {
                const BOOL bErase = true;
                InvalidateRect(s_Wnd, &s_TextRects[InType], bErase);
            }
        }
    }

    HWND GetSplashScreenWindowHandle()
    {
        return s_Wnd;
    }
}
