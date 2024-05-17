
#include "PCH.h"
#include "Rendering/Vulkan/Windows/WindowsVulkanContext.h"
#include <vulkan/vulkan_win32.h>


namespace Silex
{
    const char* WindowsVulkanContext::GetPlatformSurfaceExtensionName()
    {
        return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    }

    Surface* WindowsVulkanContext::CreateSurface(void* platformData)
    {
        WindowsVulkanSurfaceData* data = (WindowsVulkanSurfaceData*)platformData;

        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = data->instance;
        createInfo.hwnd      = data->window;

        VulkanSurface* surface = Memory::Allocate<VulkanSurface>();
        VkResult res = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface->surface);
        if (res != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkCreateWin32SurfaceKHR: {}", VkResultToString(res));
        }

        return surface;
    }

    void WindowsVulkanContext::DestroySurface(Surface* surface)
    {
        VulkanSurface* vkSurface = (VulkanSurface*)surface;
        vkDestroySurfaceKHR(instance, vkSurface->surface, nullptr);

        Memory::Deallocate(vkSurface);
    }
}
