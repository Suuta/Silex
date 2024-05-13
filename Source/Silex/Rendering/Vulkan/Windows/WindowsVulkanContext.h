
#pragma once

#include "Rendering/Vulkan/VulkanContext.h"
#include "vulkan/vulkan_win32.h"


namespace Silex
{
    // Windows vkSurface に必要な情報
    struct WindowsVulkanSurfaceData
    {
        HWND      window;
        HINSTANCE instance;
    };


    class WindowsVulkanContext : public VulkanContext
    {
    public:

        // サーフェース
        Surface* CreateSurface(void* platformData)  override final;
        void     DestroySurface()                   override final;
    };
}
