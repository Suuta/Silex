
#pragma once

#include "Rendering/Vulkan/VulkanContext.h"


namespace Silex
{
    class WindowsVulkanContext final : public VulkanContext
    {
        SL_CLASS(WindowsVulkanContext, VulkanContext)

    public:

        WindowsVulkanContext(void* platformHandle);
        ~WindowsVulkanContext();

        // サーフェース
        const char* GetPlatformSurfaceExtensionName() override final;
        Surface*    CreateSurface()                   override final;
        void        DestroySurface(Surface* surface)  override final;

    private:

        HINSTANCE instanceHandle;
        HWND      windowHandle;
    };
}
