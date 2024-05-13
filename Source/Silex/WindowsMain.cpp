
#include "PCH.h"
#include "Platform/Windows/WindowsOS.h"


namespace Silex
{
    extern Result LaunchEngine();
    extern void   ShutdownEngine();

    int32 Main()
    {
        WindowsOS os;

        Result result = LaunchEngine();
        if (result == OK)
        {
            os.Run();
        }

        ShutdownEngine();
        return result;
    }
}

int32 WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ char* lpCmdLine, _In_ int32 nCmdShow)
{
    return Silex::Main();
}
