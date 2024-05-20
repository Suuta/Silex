
#include "PCH.h"
#include "Platform/Windows/WindowsOS.h"


namespace Silex
{
    extern bool LaunchEngine();
    extern void ShutdownEngine();

    int32 Main()
    {
        WindowsOS os;

        if (LaunchEngine())
        {
            os.Run();
        }

        ShutdownEngine();
        return 0;
    }
}

int32 WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ char* lpCmdLine, _In_ int32 nCmdShow)
{
    return Silex::Main();
}
