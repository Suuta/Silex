
#include "PCH.h"
#include "Rendering/Vulkan/Windows/WindowsVulkanContext.h"


namespace Silex
{
    Surface* WindowsVulkanContext::CreateSurface(void* platformData)
    {
        WindowsVulkanSurfaceData* data = (WindowsVulkanSurfaceData*)platformData;

        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = data->instance;
        createInfo.hwnd      = data->window;

        VulkanSurface* surface = Memory::Allocate<VulkanSurface>();
        VkResult err = vkCreateWin32SurfaceKHR(nullptr, &createInfo, nullptr, &surface->surface);

        return surface;
    }

    void WindowsVulkanContext::DestroySurface()
    {
    }
}
