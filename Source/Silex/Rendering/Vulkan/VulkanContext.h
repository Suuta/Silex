
#pragma once

#include "Rendering/RenderingContext.h"
#include <vulkan/vulkan.h>


namespace Silex
{
	struct VulkanSurface : public Surface
	{
		VkSurfaceKHR surface = nullptr;
		uint32       width   = 0;
		uint32       height  = 0;
	};


    class VulkanContext : public RenderingContext
    {
	public:

		void Initialize() override;

		// API実装
		//RenderingAPI CreateRendringAPI()  override;
		//void         DestroyRendringAPI() override;

		// デバイス情報
		const DeviceInfo& GetDeviceInfo() const override;

	private:

		DeviceInfo deviceInfo;
    };
}
