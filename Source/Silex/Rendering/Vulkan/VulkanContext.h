
#pragma once

#include "Rendering/RenderingContext.h"
#include <vulkan/vulkan.h>


namespace Silex
{
#define VK_LAYER_KHRONOS_VALIDATION_NAME "VK_LAYER_KHRONOS_validation"


	inline const char* VkResultToString(VkResult result)
	{
		switch (result)
		{
			default: return "";

			case VK_SUCCESS:                        return "VK_SUCCESS";
			case VK_NOT_READY:                      return "VK_NOT_READY";
			case VK_TIMEOUT:                        return "VK_TIMEOUT";
			case VK_EVENT_SET:                      return "VK_EVENT_SET";
			case VK_EVENT_RESET:                    return "VK_EVENT_RESET";
			case VK_INCOMPLETE:                     return "VK_INCOMPLETE";
			case VK_ERROR_OUT_OF_HOST_MEMORY:       return "VK_ERROR_OUT_OF_HOST_MEMORY";
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:     return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
			case VK_ERROR_INITIALIZATION_FAILED:    return "VK_ERROR_INITIALIZATION_FAILED";
			case VK_ERROR_DEVICE_LOST:              return "VK_ERROR_DEVICE_LOST ";
			case VK_ERROR_MEMORY_MAP_FAILED:        return "VK_ERROR_MEMORY_MAP_FAILED";
			case VK_ERROR_LAYER_NOT_PRESENT:        return "VK_ERROR_LAYER_NOT_PRESENT";
			case VK_ERROR_EXTENSION_NOT_PRESENT:    return "VK_ERROR_EXTENSION_NOT_PRESENT";
			case VK_ERROR_FEATURE_NOT_PRESENT:      return "VK_ERROR_FEATURE_NOT_PRESENT";
			case VK_ERROR_INCOMPATIBLE_DRIVER:      return "VK_ERROR_INCOMPATIBLE_DRIVER";
			case VK_ERROR_TOO_MANY_OBJECTS:         return "VK_ERROR_TOO_MANY_OBJECTS";
			case VK_ERROR_FORMAT_NOT_SUPPORTED:     return "VK_ERROR_FORMAT_NOT_SUPPORTED";
			case VK_ERROR_SURFACE_LOST_KHR:         return "VK_ERROR_SURFACE_LOST_KHR";
			case VK_SUBOPTIMAL_KHR:                 return "VK_SUBOPTIMAL_KHR";
			case VK_ERROR_OUT_OF_DATE_KHR:          return "VK_ERROR_OUT_OF_DATE_KHR";
			case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
			case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
			case VK_ERROR_VALIDATION_FAILED_EXT:    return "VK_ERROR_VALIDATION_FAILED_EXT";

			case VK_RESULT_MAX_ENUM:                return "VK_RESULT_MAX_ENUM";
		}
	}







	struct VulkanSurface : public Surface
	{
		VkSurfaceKHR surface = nullptr;
	};


    class VulkanContext : public RenderingContext
    {
		SL_CLASS(VulkanContext, RenderingContext)

	public:

		VulkanContext();
		~VulkanContext();

		bool Initialize(bool enableValidation) override;

		RenderingAPI* CreateRendringAPI() override;
		void DestroyRendringAPI(RenderingAPI* api) override;

		bool DeviceCanPresent(Surface* surface) const override;
		const DeviceInfo& GetDeviceInfo() const override;

	public:

		bool QueueHasPresent(Surface* surface, uint32 queueIndex) const;
		const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() const;
		const std::vector<const char*>& GetEnabledInstanceExtensions() const;
		const std::vector<const char*>& GetEnabledDeviceExtensions() const;
		const VkPhysicalDeviceFeatures& GetPhysicalDeviceFeatureList() const;

		VkPhysicalDevice GetPhysicalDevice() const;

	public:

		virtual const char* GetPlatformSurfaceExtensionName() = 0;

	protected:

		DeviceInfo deviceInfo;

		// インスタンス
		VkInstance               instance       = nullptr;
		VkDebugUtilsMessengerEXT debugMessenger = nullptr;

		// 物理デバイス
		VkPhysicalDevice                     physicalDevice        = nullptr;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties = {};

		// デバッグメッセンジャー関数ポインタ
		PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT_PFN  = nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT_PFN = nullptr;

		// レイヤー
		std::unordered_set<std::string> requestLayers;
		std::vector<const char*>        enableLayers;

		// インスタンス拡張
		std::unordered_set<std::string> requestInstanceExtensions;
		std::vector<const char*>        enableInstanceExtensions;

		// デバイス拡張
		std::unordered_set<std::string> requestDeviceExtensions;
		std::vector<const char*>        enableDeviceExtensions;

		// デバイス機能
		VkPhysicalDeviceFeatures physicalDeviceFeatures;
    };
}
