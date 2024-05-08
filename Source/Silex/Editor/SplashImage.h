
#pragma once

namespace Silex
{
    namespace SplashTextType
    {
        enum Type
        {
            StartupProgress = 0,
            VersionInfo,
            CopyrightInfo,
            AppName,
            NumTextTypes
        };
    }

    struct SplashImage
    {
        static void Show(bool bFading = false);
        static void Hide();

        static void SetText(const SplashTextType::Type InType, const wchar_t* InText);
        static void SetText(const wchar_t* InText);
        static void SetText(const wchar_t* InText, float percentage);

        static void SetLoadProgressPercentage(float percentage);

        static bool IsShown();
    };


#define LOAD_PROCESS(text, percentage) SplashImage::SetText(L##text, percentage);
}
