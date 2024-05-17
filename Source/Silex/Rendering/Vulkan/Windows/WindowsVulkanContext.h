
#pragma once

#include "Rendering/Vulkan/VulkanContext.h"


namespace Silex
{
    // Windows vkSurface に必要な情報
    struct WindowsVulkanSurfaceData
    {
        HINSTANCE instance;
        HWND      window;
    };


    class WindowsVulkanContext final : public VulkanContext
    {
        SL_DECLARE_CLASS(WindowsVulkanContext, VulkanContext)

    public:

        WindowsVulkanContext()  {}
        ~WindowsVulkanContext() {}

        // サーフェース
        const char* GetPlatformSurfaceExtensionName() override final;
        Surface*    CreateSurface(void* platformData) override final;
        void        DestroySurface(Surface* surface)  override final;
    };
}
