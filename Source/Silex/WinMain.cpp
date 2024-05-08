
#include "PCH.h"

namespace Silex
{
    extern int32 LaunchEngine();
    extern void  ShutdownEngine();
}

int32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int32 nCmdShow)
{
    SL_TRACK_CRT_MEMORY_LEAK();

    int32 result = Silex::LaunchEngine();
    Silex::ShutdownEngine();

    return result;
}
