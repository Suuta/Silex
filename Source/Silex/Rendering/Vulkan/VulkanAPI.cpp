
#include "PCH.h"
#include "Rendering/Vulkan/VulkanAPI.h"
#include "Rendering/Vulkan/VulkanContext.h"


namespace Silex
{
    VulkanAPI::VulkanAPI(VulkanContext* context)
        : context(context)
    {
    }

    VulkanAPI::~VulkanAPI()
    {
        if (device)
        {
            vkDestroyDevice(device, nullptr);
        }
    }

    bool VulkanAPI::Initialize()
    {
        // --- デバイス生成 ---
        // 専用キュー検索のために、機能を1つでも満たしているキューは生成リストに追加
        // 要求を満たすキューをすべて生成し、使用したいキューのみインデックスでアクセスする

        const auto& queueProperties = context->GetQueueFamilyProperties();
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        static const float queuePriority[] = { 0.0f };

        // デバイスキュー生成情報
        for (uint32 i = 0; i < queueProperties.size(); i++)
        {
            // 1つでもサポートしていれば
            if (queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)
            {
                VkDeviceQueueCreateInfo createInfo = {};
                createInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                createInfo.queueFamilyIndex = i;
                createInfo.queueCount       = 1;
                createInfo.pQueuePriorities = queuePriority;

                queueCreateInfos.push_back(createInfo);
            }
        }

        // 拡張機能ポインタチェイン
        void* createInfoNext = nullptr;

        // デバイス生成
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext                   = createInfoNext;
        deviceCreateInfo.queueCreateInfoCount    = queueCreateInfos.size();
        deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
        deviceCreateInfo.enabledExtensionCount   = context->GetEnabledDeviceExtensions().size();
        deviceCreateInfo.ppEnabledExtensionNames = context->GetEnabledDeviceExtensions().data();
        deviceCreateInfo.pEnabledFeatures        = &context->GetPhysicalDeviceFeatureList();

        VkResult result = vkCreateDevice(context->GetPhysicalDevice(), &deviceCreateInfo, nullptr, &device);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkCreateDevice: {}", VkResultToString(result));
            return false;
        }


        return true;
    }

    QueueFamily VulkanAPI::GetQueueFamily(uint32 flag, Surface* surface) const
    {
        QueueFamily familyIndex = INVALID_RENDER_ID;

        const auto& queueFamilyProperties = context->GetQueueFamilyProperties();
        for (uint32 i = 0; i < queueFamilyProperties.size(); i++)
        {
            // サーフェースが有効であれば、プレゼントキューが有効なキューが必須となる
            if (surface != nullptr && !context->QueueHasPresent(surface, i))
            {
                continue;
            }

            // TODO: 専用キューを選択
            // 独立したキューがあればそのキューの方が性能が良いとされるので、flag値が低いものが専用キューになる。

            // 全フラグが立っていたら（全てのキューをサポートしている場合）
            const bool includeAll = (queueFamilyProperties[i].queueFlags & flag) == flag;
            if (includeAll)
            {
                familyIndex = i;
                break;
            }
        }

        return familyIndex;
    }

    CommandQueue* VulkanAPI::CreateCommandQueue(QueueFamily family)
    {
        VulkanCommandQueue* vkQueue = Memory::Allocate<VulkanCommandQueue>();
        vkQueue->queueIndex = family;

        return vkQueue;
    }

    CommandPool* VulkanAPI::CreateCommandPool(QueueFamily family, CommandBufferType type)
    {
        uint32 familyIndex = family;

        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.queueFamilyIndex = familyIndex;
        commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool vkCommandPool = nullptr;
        VkResult result = vkCreateCommandPool(device, &commandPoolInfo, nullptr, &vkCommandPool);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkCreateCommandPool: {}", VkResultToString(result));
            return nullptr;
        }

        VulkanCommandPool* commandPool = Memory::Allocate<VulkanCommandPool>();
        commandPool->commandPool = vkCommandPool;
        commandPool->type        = type;

        return commandPool;
    }
}
