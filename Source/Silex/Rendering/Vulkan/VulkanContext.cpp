
#include "PCH.h"
#include "Rendering/Vulkan/VulkanContext.h"
#include "Rendering/Vulkan/VulkanAPI.h"


namespace Silex
{
#define GET_VULKAN_INSTANCE_PROC(instance, func) (PFN_##func)vkGetInstanceProcAddr(instance, #func);
#define GET_VULKAN_DEVICE_PROC(device, func)     (PFN_##func)vkGetDeviceProcAddr(device, #func);

    static VkBool32 DebugMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                       pUserData)
    {
        const char* message = pCallbackData->pMessage;

        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:  SL_LOG_TRACE("{}", message); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:     SL_LOG_INFO("{}",  message); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:  SL_LOG_WARN("{}",  message); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:    SL_LOG_ERROR("{}", message); break;

            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
                break;
        }

        return VK_FALSE;
    }


    // NOTE:========================================================================
    // 以前の実装では、インスタンス固有の検証レイヤーとデバイス固有の検証レイヤーが区別されていたが
    // 最新の実装では enabledLayerCount / ppEnabledLayerNames フィールドは 無視される
    // ただし、古い実装との互換性を保つ必要があれば、設定する必要がある
    // =============================================================================
    // if (enableValidationLayers)
    // {
    //     // インスタンスに設定した "VK_LAYER_KHRONOS_validation" レイヤーをここでも有効にする必要があった
    //     createInfo.enabledLayerCount   = validationLayers.size();
    //     createInfo.ppEnabledLayerNames = validationLayers.data();
    // }
    // else
    // {
    //     createInfo.enabledLayerCount   = 0;
    //     createInfo.ppEnabledLayerNames = nullptr;
    // }


    VulkanContext::VulkanContext()
    {
    }

    VulkanContext::~VulkanContext()
    {
        vkDestroyDebugUtilsMessengerEXT_PFN(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    bool VulkanContext::Initialize(bool enableValidation)
    {
        uint32 instanceVersion = VK_API_VERSION_1_0;

        // ドライバーの Vulkan バージョンを取得(ver 1.1 から有効であり、関数のロードが失敗した場合 ver 1.0)
        auto vkEnumerateInstanceVersion_PFN = GET_VULKAN_INSTANCE_PROC(nullptr, vkEnumerateInstanceVersion);
        if (vkEnumerateInstanceVersion_PFN)
        {
            if (vkEnumerateInstanceVersion_PFN(&instanceVersion) == VK_SUCCESS)
            {
                uint32 major = VK_VERSION_MAJOR(instanceVersion);
                uint32 minor = VK_VERSION_MINOR(instanceVersion);
                uint32 patch = VK_VERSION_PATCH(instanceVersion);

                SL_LOG_INFO("Vulkan Instance Version: {}.{}.{}", major, minor, patch);
            }
            else
            {
                SL_LOG_ERROR("原因不明のエラーです");
            }
        }

        // 要求インスタンス拡張機能
        requestInstanceExtensions.insert(VK_KHR_SURFACE_EXTENSION_NAME);
        requestInstanceExtensions.insert(GetPlatformSurfaceExtensionName());

        // バリデーションが有効な場合
        if (enableValidation)
        {
            requestInstanceExtensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // インスタンス拡張機能のクエリ
        uint32 instanceExtensionCount = 0;
        VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);

        std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
        result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data());

        // 要求機能が有効かどうかを確認
        for (const VkExtensionProperties& extension : instanceExtensions)
        {
            const auto& itr = requestInstanceExtensions.find(extension.extensionName);
            if (itr != requestInstanceExtensions.end())
            {
                enableInstanceExtensions.push_back(itr->c_str());
            }
        }

        // バリデーションが有効な場合
        if (enableValidation)
        {
            requestLayers.insert(VK_LAYER_KHRONOS_VALIDATION_NAME);
        }

        // インスタンスレイヤーのクエリ
        uint32 instanceLayerCount = 0;
        result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);

        std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
        result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

        // 要求レイヤーが有効かどうかを確認
        for (const VkLayerProperties& property : instanceLayers)
        {
            const auto& itr = requestLayers.find(property.layerName);
            if (itr != requestLayers.end())
            {
                enableLayers.push_back(itr->c_str());
            }
        }

        // デバック出力拡張機能の情報（バリデーションレイヤーのメッセージ出力もこの機能が行う）
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
        debugMessengerInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerInfo.pNext           = nullptr;
        debugMessengerInfo.flags           = 0;
        debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerInfo.pfnUserCallback = DebugMessengerCallback;
        debugMessengerInfo.pUserData       = this;


        // 1.0 を除き、GPU側がサポートしている限りは、指定バージョンに関係なくGPUバージョンレベルの機能が利用可能
        // つまり、物理デバイスのバージョンではなく、開発の最低保証バージョンを指定した方が良い
        uint32 apiVersion = instanceVersion == VK_API_VERSION_1_0 ? VK_API_VERSION_1_0 : VK_API_VERSION_1_2;

        // アプリ情報
        VkApplicationInfo appInfo = {};
        appInfo.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Silex";
        appInfo.pEngineName      = "Silex";
        appInfo.engineVersion    = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion       = apiVersion;

        // インスタンス情報
        VkInstanceCreateInfo instanceInfo = {};
        instanceInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo        = &appInfo;
        instanceInfo.enabledExtensionCount   = enableInstanceExtensions.size();
        instanceInfo.ppEnabledExtensionNames = enableInstanceExtensions.data();
        instanceInfo.enabledLayerCount       = enableLayers.size();
        instanceInfo.ppEnabledLayerNames     = enableLayers.data();
        instanceInfo.pNext                   = &debugMessengerInfo;

        result = vkCreateInstance(&instanceInfo, nullptr, &instance);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkCreateInstance: {}", VkResultToString(result));
            return false;
        }

        // デバッグメッセンジャーの関数ポインタを取得
        vkCreateDebugUtilsMessengerEXT_PFN  = GET_VULKAN_INSTANCE_PROC(instance, vkCreateDebugUtilsMessengerEXT);
        vkDestroyDebugUtilsMessengerEXT_PFN = GET_VULKAN_INSTANCE_PROC(instance, vkDestroyDebugUtilsMessengerEXT);

        if (!vkCreateDebugUtilsMessengerEXT_PFN or !vkDestroyDebugUtilsMessengerEXT_PFN)
        {
            SL_LOG_ERROR("DebugUtilsMessengerEXT 関数が null です");
            return false;
        }

        // デバッグメッセンジャー生成
        result = vkCreateDebugUtilsMessengerEXT_PFN(instance, &debugMessengerInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkCreateDebugUtilsMessengerEXT: {}", VkResultToString(result));
            return false;
        }

        // 物理デバイスの列挙
        uint32 physicalDeviceCount = 0;
        result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

        // 物理デバイスの選択
        for (const auto& pd : physicalDevices)
        {
            VkPhysicalDeviceProperties property;
            vkGetPhysicalDeviceProperties(pd, &property);
            
            VkPhysicalDeviceFeatures feature;
            vkGetPhysicalDeviceFeatures(pd, &feature);

            // 外部GPUでジオメトリシェーダをサポートしているデバイスのみを選択
            if (property.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && feature.geometryShader)
            {
                deviceInfo.name   = property.deviceName;
                deviceInfo.vendor = (DeviceVendor)property.vendorID;
                deviceInfo.type   = (DeviceType)property.deviceType;

                physicalDevice = pd;
                break;
            }
        }

        if (physicalDevice == nullptr)
        {
            SL_ASSERT("適切なGPUが見つかりませんでした");
            return false;
        }

        // キューファミリーの取得
        uint32 queueFamilyPropertiesCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, nullptr);

        queueFamilyProperties.resize(queueFamilyPropertiesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties.data());


        // 要求デバイス拡張
        requestDeviceExtensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // デバイス拡張機能のクエリ
        uint32 deviceExtensionCount = 0;
        result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);

        std::vector<VkExtensionProperties> deviceExtension(deviceExtensionCount);
        result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtension.data());

        for (const VkExtensionProperties& property : deviceExtension)
        {
            const auto& itr = requestDeviceExtensions.find(property.extensionName);
            if (itr != requestDeviceExtensions.end())
            {
                enableDeviceExtensions.push_back(itr->c_str());
            }
        }

        // 物理デバイスの有効機能リスト取得
        vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

        return true;
    }

    RenderingAPI* VulkanContext::CreateRendringAPI()
    {
        return Memory::Allocate<VulkanAPI>(this);
    }

    void VulkanContext::DestroyRendringAPI(RenderingAPI* api)
    {
        Memory::Deallocate(api);
    }

    bool VulkanContext::DeviceCanPresent(Surface* surface) const
    {
        VkSurfaceKHR vkSurface = ((VulkanSurface*)surface)->surface;

        for (uint32 i = 0; i < queueFamilyProperties.size(); i++)
        {
            VkBool32 supported = false;
            VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, vkSurface, &supported);
            if (result != VK_SUCCESS)
            {
                SL_LOG_ERROR("vkGetPhysicalDeviceSurfaceSupportKHR: {}", VkResultToString(result));
            }

            if (supported)
                return true;
        }

        return false;
    }

    const DeviceInfo& VulkanContext::GetDeviceInfo() const
    {
        return deviceInfo;
    }

    bool VulkanContext::QueueHasPresent(Surface* surface, uint32 queueIndex) const
    {
        VkSurfaceKHR vkSurface = ((VulkanSurface*)surface)->surface;
        VkBool32 supported = false;

        VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, vkSurface, &supported);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkGetPhysicalDeviceSurfaceSupportKHR: {}", VkResultToString(result));
        }

        return supported;
    }

    const std::vector<VkQueueFamilyProperties>& VulkanContext::GetQueueFamilyProperties() const
    {
        return queueFamilyProperties;
    }

    const std::vector<const char*>& VulkanContext::GetEnabledInstanceExtensions() const
    {
        return enableInstanceExtensions;
    }

    const std::vector<const char*>& VulkanContext::GetEnabledDeviceExtensions() const
    {
        return enableDeviceExtensions;
    }

    const VkPhysicalDeviceFeatures& VulkanContext::GetPhysicalDeviceFeatureList() const
    {
        return physicalDeviceFeatures;
    }

    VkPhysicalDevice VulkanContext::GetPhysicalDevice() const
    {
        return physicalDevice;
    }
}