
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
        if (allocator)
        {
            vmaDestroyAllocator(allocator);
        }

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
        
        // メモリアロケータ（VMA）生成
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = context->GetPhysicalDevice();
        allocatorInfo.device         = device;
        allocatorInfo.instance       = context->GetInstance();

        result = vmaCreateAllocator(&allocatorInfo, &allocator);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vmaCreateAllocator: {}", VkResultToString(result));
            return false;
        }

        return true;
    }

    CommandQueue* VulkanAPI::CreateCommandQueue(QueueFamily family, uint32 indexInFamily)
    {
        // queueIndex は キューファミリ内に複数キューが存在する場合のインデックスを指定する
        VkQueue vkQueue = nullptr;
        vkGetDeviceQueue(device, family, indexInFamily, &vkQueue);

        VulkanCommandQueue* queue = Memory::Allocate<VulkanCommandQueue>();
        queue->family = family;
        queue->index  = indexInFamily;
        queue->queue  = vkQueue;

        return queue;
    }

    void VulkanAPI::DestroyCommandQueue(CommandQueue* queue)
    {
        if (queue)
        {
            // VkQueue に解放処理はない
            //...

            Memory::Deallocate(queue);
        }
    }

    QueueFamily VulkanAPI::QueryQueueFamily(uint32 queueFlag, Surface* surface) const
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
            const bool includeAll = (queueFamilyProperties[i].queueFlags & queueFlag) == queueFlag;
            if (includeAll)
            {
                familyIndex = i;
                break;
            }
        }

        return familyIndex;
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

    void VulkanAPI::DestroyCommandPool(CommandPool* pool)
    {
        if (pool)
        {
            VulkanCommandPool* vkpool = (VulkanCommandPool*)pool;
            vkDestroyCommandPool(device, vkpool->commandPool, nullptr);

            Memory::Deallocate(pool);
        }
    }

    CommandBuffer* VulkanAPI::CreateCommandBuffer(CommandPool* pool)
    {
        VulkanCommandPool* vkpool = (VulkanCommandPool*)pool;

        VkCommandBufferAllocateInfo createInfo = {};
        createInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        createInfo.commandPool        = vkpool->commandPool;
        createInfo.commandBufferCount = 1;
        createInfo.level              = (VkCommandBufferLevel)vkpool->type;

        VkCommandBuffer vkcommandbuffer = nullptr;
        VkResult result = vkAllocateCommandBuffers(device, &createInfo, &vkcommandbuffer);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkAllocateCommandBuffers: {}", VkResultToString(result));
            return nullptr;
        }

        VulkanCommandBuffer* cmdBuffer = Memory::Allocate<VulkanCommandBuffer>();
        cmdBuffer->commandBuffer = vkcommandbuffer;

        return cmdBuffer;
    }

    void VulkanAPI::DestroyCommandBuffer(CommandBuffer* commandBuffer)
    {
        if (commandBuffer)
        {
            // VkCommandBuffer は コマンドプールが対応するので 解放処理しない (※ vkFreeCommandBuffers は？)
            //...

            Memory::Deallocate(commandBuffer);
        }
    }

    bool VulkanAPI::BeginCommandBuffer(CommandBuffer* commandBuffer)
    {
        VulkanCommandBuffer* vkcmdBuffer = (VulkanCommandBuffer*)commandBuffer;

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkResult result = vkBeginCommandBuffer(vkcmdBuffer->commandBuffer, &beginInfo);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkBeginCommandBuffer: {}", VkResultToString(result));
            return false;
        }

        return true;
    }

    bool VulkanAPI::EndCommandBuffer(CommandBuffer* commandBuffer)
    {
        VulkanCommandBuffer* vkcmdBuffer = (VulkanCommandBuffer*)commandBuffer;

        VkResult result = vkEndCommandBuffer((VkCommandBuffer)vkcmdBuffer->commandBuffer);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkEndCommandBuffer: {}", VkResultToString(result));
            return false;
        }

        return true;
    }

    Semaphore* VulkanAPI::CreateSemaphore()
    {
        VkSemaphore vkSemaphore = nullptr;
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = nullptr;

        VkResult result = vkCreateSemaphore(device, &createInfo, nullptr, &vkSemaphore);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkCreateSemaphore: {}", VkResultToString(result));
            return nullptr;
        }

        VulkanSemaphore* semaphore = Memory::Allocate<VulkanSemaphore>();
        semaphore->semaphore = vkSemaphore;

        return semaphore;
    }

    void VulkanAPI::DestroySemaphore(Semaphore* semaphore)
    {
        if (semaphore)
        {
            VulkanSemaphore* vkSemaphore = (VulkanSemaphore*)semaphore;
            vkDestroySemaphore(device, vkSemaphore->semaphore, nullptr);

            Memory::Deallocate(semaphore);
        }
    }

    Fence* VulkanAPI::CreateFence()
    {
        VkFence vkfence = VK_NULL_HANDLE;
        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkResult result = vkCreateFence(device, &createInfo, nullptr, &vkfence);
        if (result != VK_SUCCESS)
        {
            SL_LOG_ERROR("vkCreateFence: {}", VkResultToString(result));
            return nullptr;
        }

        VulkanFence* fence = Memory::Allocate<VulkanFence>();
        fence->fence         = vkfence;
        fence->queueSignaled = nullptr;

        return fence;
    }

    void VulkanAPI::DestroyFence(Fence* fence)
    {
        if (fence)
        {
            VulkanFence* vkfence = (VulkanFence*)fence;
            vkDestroyFence(device, vkfence->fence, nullptr);

            Memory::Deallocate(fence);
        }
    }

    bool VulkanAPI::WaitFence(Fence* fence)
    {
        return true;
    }
}
