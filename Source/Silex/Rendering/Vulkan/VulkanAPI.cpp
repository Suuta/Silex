
#include "PCH.h"
#include "Rendering/Vulkan/VulkanAPI.h"
#include "Rendering/Vulkan/VulkanContext.h"


namespace Silex
{
    static const VkFormat VulkanFormats[] =
    {
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_R4G4_UNORM_PACK8,
        VK_FORMAT_R4G4B4A4_UNORM_PACK16,
        VK_FORMAT_B4G4R4A4_UNORM_PACK16,
        VK_FORMAT_R5G6B5_UNORM_PACK16,
        VK_FORMAT_B5G6R5_UNORM_PACK16,
        VK_FORMAT_R5G5B5A1_UNORM_PACK16,
        VK_FORMAT_B5G5R5A1_UNORM_PACK16,
        VK_FORMAT_A1R5G5B5_UNORM_PACK16,
        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8_SNORM,
        VK_FORMAT_R8_USCALED,
        VK_FORMAT_R8_SSCALED,
        VK_FORMAT_R8_UINT,
        VK_FORMAT_R8_SINT,
        VK_FORMAT_R8_SRGB,
        VK_FORMAT_R8G8_UNORM,
        VK_FORMAT_R8G8_SNORM,
        VK_FORMAT_R8G8_USCALED,
        VK_FORMAT_R8G8_SSCALED,
        VK_FORMAT_R8G8_UINT,
        VK_FORMAT_R8G8_SINT,
        VK_FORMAT_R8G8_SRGB,
        VK_FORMAT_R8G8B8_UNORM,
        VK_FORMAT_R8G8B8_SNORM,
        VK_FORMAT_R8G8B8_USCALED,
        VK_FORMAT_R8G8B8_SSCALED,
        VK_FORMAT_R8G8B8_UINT,
        VK_FORMAT_R8G8B8_SINT,
        VK_FORMAT_R8G8B8_SRGB,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_B8G8R8_SNORM,
        VK_FORMAT_B8G8R8_USCALED,
        VK_FORMAT_B8G8R8_SSCALED,
        VK_FORMAT_B8G8R8_UINT,
        VK_FORMAT_B8G8R8_SINT,
        VK_FORMAT_B8G8R8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SNORM,
        VK_FORMAT_R8G8B8A8_USCALED,
        VK_FORMAT_R8G8B8A8_SSCALED,
        VK_FORMAT_R8G8B8A8_UINT,
        VK_FORMAT_R8G8B8A8_SINT,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_B8G8R8A8_SNORM,
        VK_FORMAT_B8G8R8A8_USCALED,
        VK_FORMAT_B8G8R8A8_SSCALED,
        VK_FORMAT_B8G8R8A8_UINT,
        VK_FORMAT_B8G8R8A8_SINT,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_A8B8G8R8_UNORM_PACK32,
        VK_FORMAT_A8B8G8R8_SNORM_PACK32,
        VK_FORMAT_A8B8G8R8_USCALED_PACK32,
        VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
        VK_FORMAT_A8B8G8R8_UINT_PACK32,
        VK_FORMAT_A8B8G8R8_SINT_PACK32,
        VK_FORMAT_A8B8G8R8_SRGB_PACK32,
        VK_FORMAT_A2R10G10B10_UNORM_PACK32,
        VK_FORMAT_A2R10G10B10_SNORM_PACK32,
        VK_FORMAT_A2R10G10B10_USCALED_PACK32,
        VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
        VK_FORMAT_A2R10G10B10_UINT_PACK32,
        VK_FORMAT_A2R10G10B10_SINT_PACK32,
        VK_FORMAT_A2B10G10R10_UNORM_PACK32,
        VK_FORMAT_A2B10G10R10_SNORM_PACK32,
        VK_FORMAT_A2B10G10R10_USCALED_PACK32,
        VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
        VK_FORMAT_A2B10G10R10_UINT_PACK32,
        VK_FORMAT_A2B10G10R10_SINT_PACK32,
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R16_SNORM,
        VK_FORMAT_R16_USCALED,
        VK_FORMAT_R16_SSCALED,
        VK_FORMAT_R16_UINT,
        VK_FORMAT_R16_SINT,
        VK_FORMAT_R16_SFLOAT,
        VK_FORMAT_R16G16_UNORM,
        VK_FORMAT_R16G16_SNORM,
        VK_FORMAT_R16G16_USCALED,
        VK_FORMAT_R16G16_SSCALED,
        VK_FORMAT_R16G16_UINT,
        VK_FORMAT_R16G16_SINT,
        VK_FORMAT_R16G16_SFLOAT,
        VK_FORMAT_R16G16B16_UNORM,
        VK_FORMAT_R16G16B16_SNORM,
        VK_FORMAT_R16G16B16_USCALED,
        VK_FORMAT_R16G16B16_SSCALED,
        VK_FORMAT_R16G16B16_UINT,
        VK_FORMAT_R16G16B16_SINT,
        VK_FORMAT_R16G16B16_SFLOAT,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R16G16B16A16_SNORM,
        VK_FORMAT_R16G16B16A16_USCALED,
        VK_FORMAT_R16G16B16A16_SSCALED,
        VK_FORMAT_R16G16B16A16_UINT,
        VK_FORMAT_R16G16B16A16_SINT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R32_UINT,
        VK_FORMAT_R32_SINT,
        VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R32G32_SINT,
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32B32_UINT,
        VK_FORMAT_R32G32B32_SINT,
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32A32_UINT,
        VK_FORMAT_R32G32B32A32_SINT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R64_UINT,
        VK_FORMAT_R64_SINT,
        VK_FORMAT_R64_SFLOAT,
        VK_FORMAT_R64G64_UINT,
        VK_FORMAT_R64G64_SINT,
        VK_FORMAT_R64G64_SFLOAT,
        VK_FORMAT_R64G64B64_UINT,
        VK_FORMAT_R64G64B64_SINT,
        VK_FORMAT_R64G64B64_SFLOAT,
        VK_FORMAT_R64G64B64A64_UINT,
        VK_FORMAT_R64G64B64A64_SINT,
        VK_FORMAT_R64G64B64A64_SFLOAT,
        VK_FORMAT_B10G11R11_UFLOAT_PACK32,
        VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_BC1_RGB_UNORM_BLOCK,
        VK_FORMAT_BC1_RGB_SRGB_BLOCK,
        VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
        VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
        VK_FORMAT_BC2_UNORM_BLOCK,
        VK_FORMAT_BC2_SRGB_BLOCK,
        VK_FORMAT_BC3_UNORM_BLOCK,
        VK_FORMAT_BC3_SRGB_BLOCK,
        VK_FORMAT_BC4_UNORM_BLOCK,
        VK_FORMAT_BC4_SNORM_BLOCK,
        VK_FORMAT_BC5_UNORM_BLOCK,
        VK_FORMAT_BC5_SNORM_BLOCK,
        VK_FORMAT_BC6H_UFLOAT_BLOCK,
        VK_FORMAT_BC6H_SFLOAT_BLOCK,
        VK_FORMAT_BC7_UNORM_BLOCK,
        VK_FORMAT_BC7_SRGB_BLOCK,
        VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
        VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
        VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
        VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
        VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
        VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
        VK_FORMAT_EAC_R11_UNORM_BLOCK,
        VK_FORMAT_EAC_R11_SNORM_BLOCK,
        VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
        VK_FORMAT_EAC_R11G11_SNORM_BLOCK,
        VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
        VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
        VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
        VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
        VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
        VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
        VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
        VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
        VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
        VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
        VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
        VK_FORMAT_ASTC_12x12_SRGB_BLOCK,
        VK_FORMAT_G8B8G8R8_422_UNORM,
        VK_FORMAT_B8G8R8G8_422_UNORM,
        VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
        VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
        VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,
        VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
        VK_FORMAT_R10X6_UNORM_PACK16,
        VK_FORMAT_R10X6G10X6_UNORM_2PACK16,
        VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
        VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
        VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
        VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
        VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
        VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
        VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
        VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
        VK_FORMAT_R12X4_UNORM_PACK16,
        VK_FORMAT_R12X4G12X4_UNORM_2PACK16,
        VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
        VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
        VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
        VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
        VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
        VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
        VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
        VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
        VK_FORMAT_G16B16G16R16_422_UNORM,
        VK_FORMAT_B16G16R16G16_422_UNORM,
        VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
        VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,
        VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
        VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,
        VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,
    };




    static const VkImageType VulkanImageTypes[] =
    {
        VK_IMAGE_TYPE_1D,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_TYPE_3D,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_TYPE_1D,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_TYPE_2D,
    };

    static const VkSampleCountFlagBits VulkanSampleCounts[] =
    {
        VK_SAMPLE_COUNT_1_BIT,
        VK_SAMPLE_COUNT_2_BIT,
        VK_SAMPLE_COUNT_4_BIT,
        VK_SAMPLE_COUNT_8_BIT,
        VK_SAMPLE_COUNT_16_BIT,
        VK_SAMPLE_COUNT_32_BIT,
        VK_SAMPLE_COUNT_64_BIT,
    };


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

        // デバイス機能を取得
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(context->GetPhysicalDevice(), &features);

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
        deviceCreateInfo.pEnabledFeatures        = &features;

        VkResult result = vkCreateDevice(context->GetPhysicalDevice(), &deviceCreateInfo, nullptr, &device);
        SL_CHECK_VKRESULT(result, false);

        // 拡張機能関数ロード
        extensions.vkCreateSwapchainKHR    = GET_VULKAN_DEVICE_PROC(device, vkCreateSwapchainKHR);
        extensions.vkDestroySwapchainKHR   = GET_VULKAN_DEVICE_PROC(device, vkDestroySwapchainKHR);
        extensions.vkGetSwapchainImagesKHR = GET_VULKAN_DEVICE_PROC(device, vkGetSwapchainImagesKHR);
        extensions.vkAcquireNextImageKHR   = GET_VULKAN_DEVICE_PROC(device, vkAcquireNextImageKHR);
        extensions.vkQueuePresentKHR       = GET_VULKAN_DEVICE_PROC(device, vkQueuePresentKHR);

        // メモリアロケータ（VMA）生成
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = context->GetPhysicalDevice();
        allocatorInfo.device         = device;
        allocatorInfo.instance       = context->GetInstance();

        result = vmaCreateAllocator(&allocatorInfo, &allocator);
        SL_CHECK_VKRESULT(result, false);

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
        commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = familyIndex;

        VkCommandPool vkCommandPool = nullptr;
        VkResult result = vkCreateCommandPool(device, &commandPoolInfo, nullptr, &vkCommandPool);
        SL_CHECK_VKRESULT(result, nullptr);

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
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanCommandBuffer* cmdBuffer = Memory::Allocate<VulkanCommandBuffer>();
        cmdBuffer->commandBuffer = vkcommandbuffer;

        return cmdBuffer;
    }

    void VulkanAPI::DestroyCommandBuffer(CommandBuffer* commandBuffer)
    {
        if (commandBuffer)
        {
            // VkCommandBuffer は コマンドプールが対応するので 解放処理しない
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
        SL_CHECK_VKRESULT(result, false);

        return true;
    }

    bool VulkanAPI::EndCommandBuffer(CommandBuffer* commandBuffer)
    {
        VulkanCommandBuffer* vkcmdBuffer = (VulkanCommandBuffer*)commandBuffer;

        VkResult result = vkEndCommandBuffer((VkCommandBuffer)vkcmdBuffer->commandBuffer);
        SL_CHECK_VKRESULT(result, false);

        return true;
    }

    Semaphore* VulkanAPI::CreateSemaphore()
    {
        VkSemaphore vkSemaphore = nullptr;
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = nullptr;

        VkResult result = vkCreateSemaphore(device, &createInfo, nullptr, &vkSemaphore);
        SL_CHECK_VKRESULT(result, nullptr);

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
        VkFence vkfence = nullptr;
        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkResult result = vkCreateFence(device, &createInfo, nullptr, &vkfence);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanFence* fence = Memory::Allocate<VulkanFence>();
        fence->fence = vkfence;

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
        VulkanFence* vkfence = (VulkanFence*)fence;
        VkResult result = vkWaitForFences(device, 1, &vkfence->fence, true, UINT64_MAX);
        SL_CHECK_VKRESULT(result, false);

        result = vkResetFences(device, 1, &vkfence->fence);
        SL_CHECK_VKRESULT(result, false);

        return true;
    }

    SwapChain* VulkanAPI::CreateSwapChain(Surface* surface)
    {
        VkResult result;

        VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();
        VkSurfaceKHR     vkSurface      = ((VulkanSurface*)surface)->surface;

        uint32 formatCount = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface, &formatCount, nullptr);
        SL_CHECK_VKRESULT(result, nullptr);

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface, &formatCount, formats.data());
        SL_CHECK_VKRESULT(result, nullptr);

        VkFormat        format     = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        {
            // VK_FORMAT_UNDEFINED が1つだけ含まれている場合、サーフェスには優先フォーマットがない
            format = VK_FORMAT_B8G8R8A8_UNORM;
            colorspace = formats[0].colorSpace;
        }
        else
        {
            // BGRA8_UNORM がサポートされていればそれが推奨される
            const VkFormat firstChoice  = VK_FORMAT_B8G8R8A8_UNORM;
            const VkFormat secondChoice = VK_FORMAT_R8G8B8A8_UNORM;

            for (uint32_t i = 0; i < formatCount; i++)
            {
                if (formats[i].format == firstChoice || formats[i].format == secondChoice)
                {
                    format = formats[i].format;
                    if (formats[i].format == firstChoice)
                    {
                        break;
                    }
                }
            }
        }

        SL_CHECK(format == VK_FORMAT_UNDEFINED, nullptr);

    //--------------------------------------------------------------------------------------------------------
    // --- NOTE ---
    // VulkanSwapChain が VulkanRenderPassを保持し、GetSwapChainRenderPassによって RenderPassインターフェースを
    // 返す必要があり、RenderPass オブジェクトを生成するために、Vulkan実装内でありながらも抽象化コードを使用している
    //--------------------------------------------------------------------------------------------------------
#if 0
        VkAttachmentDescription2KHR attachment = {};
        attachment.sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR;
        attachment.format         = format;
        attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;      // 描画 前 処理
        attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;     // 描画 後 処理
        attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // 
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 
        attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;        // イメージの初期レイアウト
        attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // レンダーパス終了後に自動で移行するレイアウト

        VkAttachmentReference2KHR colorReference = {};
        colorReference.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR;
        colorReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorReference.attachment = 0;

        VkSubpassDescription2KHR subpass = {};
        subpass.sType                = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR;
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorReference;

        VkRenderPassCreateInfo2KHR passInfo = {};
        passInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
        passInfo.attachmentCount = 1;
        passInfo.pAttachments    = &attachment;
        passInfo.subpassCount    = 1;
        passInfo.pSubpasses      = &subpass;

        VkRenderPass renderPass = nullptr;
        result = vkCreateRenderPass2(device, &passInfo, nullptr, &renderPass);
        SL_CHECK_VKRESULT(result, nullptr);
#endif

        Attachment attachments;
        attachments.format         = (RenderingFormat)format;
        attachments.samples        = TEXTURE_SAMPLES_1;
        attachments.initialLayout  = TEXTURE_LAYOUT_UNDEFINED;
        attachments.finalLayout    = TEXTURE_LAYOUT_PRESENT_SRC;
        attachments.loadOp         = ATTACHMENT_LOAD_OP_CLEAR;
        attachments.storeOp        = ATTACHMENT_STORE_OP_STORE;

        AttachmentReference reference;
        reference.attachment = 0;
        reference.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        reference.aspect     = TEXTURE_ASPECT_COLOR_BIT;

        Subpass subpasses;
        subpasses.colorReferences.push_back(reference);

        VulkanSwapChain* swapchain = Memory::Allocate<VulkanSwapChain>();
        swapchain->surface    = ((VulkanSurface*)surface);
        swapchain->format     = format;
        swapchain->colorspace = colorspace;
        swapchain->renderpass = (VulkanRenderPass*)CreateRenderPass(1, &attachments, 1, &subpasses, 0, nullptr);

        return swapchain;
    }

    bool VulkanAPI::ResizeSwapChain(SwapChain* swapchain, uint32 requestFramebufferCount, VSyncMode mode)
    {
        VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();
        VulkanSwapChain* vkSwapchain = (VulkanSwapChain*)swapchain;
        VulkanSurface* surface = vkSwapchain->surface;

        // サーフェース仕様取得
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface->surface, &surfaceCapabilities);
        SL_CHECK_VKRESULT(result, false);

        // サイズ
        VkExtent2D extent;
        if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) // == (uint32)-1
        {
            // 0xFFFFFFFF の場合は、仕様が許す限り好きなサイズで生成できることを表す
            extent.width  = std::clamp(surface->width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
            extent.height = std::clamp(surface->height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
        }
        else
        {
            // 0xFFFFFFFF 以外は、指定された値で生成する必要がある
            extent = surfaceCapabilities.currentExtent;
            surface->width = extent.width;
            surface->height = extent.height;
        }

        if (surface->width == 0 || surface->height == 0)
        {
            return false;
        }

        // プレゼントモード
        uint32 presentModeCount = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface->surface, &presentModeCount, nullptr);
        SL_CHECK_VKRESULT(result, false);

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface->surface, &presentModeCount, presentModes.data());
        SL_CHECK_VKRESULT(result, false);

        // VSYNC_MODE_DISABLED: VK_PRESENT_MODE_IMMEDIATE_KHR
        // VSYNC_MODE_MAILBOX : VK_PRESENT_MODE_MAILBOX_KHR
        // VSYNC_MODE_ENABLED : VK_PRESENT_MODE_FIFO_KHR
        // VSYNC_MODE_ADAPTIVE: VK_PRESENT_MODE_FIFO_RELAXED_KHR
        VkPresentModeKHR presentMode = (VkPresentModeKHR)mode;

        bool findRequestMode = false;
        for (uint32 i = 0; i < presentModes.size(); i++)
        {
            if (presentMode == presentModes[i])
            {
                findRequestMode = true;
                surface->vsyncMode = (VSyncMode)presentMode;
                break;
            }
        }

        // 見つからない場合は、必ずサポートされている（VK_PRESENT_MODE_FIFO_KHR）モードを選択
        if (!findRequestMode)
        {
            surface->vsyncMode = VSYNC_MODE_ENABLED;
            presentMode        = VK_PRESENT_MODE_FIFO_KHR;
        }

        // トランスフォーム情報（回転・反転）
        VkSurfaceTransformFlagBitsKHR surfaceTransformBits;
        if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            surfaceTransformBits = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            surfaceTransformBits = surfaceCapabilities.currentTransform;
        }

        // アルファモードが有効なら（現状: 使用しない）
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (false || !(surfaceCapabilities.supportedCompositeAlpha & compositeAlpha))
        {
            VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] =
            {
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
            };

            for (uint32 i = 0; i < std::size(compositeAlphaFlags); i++)
            {
                if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i])
                {
                    compositeAlpha = compositeAlphaFlags[i];
                    break;
                }
            }
        }

        // フレームバッファ数決定（現状: 3）
        uint32 requestImageCount = std::clamp(requestFramebufferCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);

        // スワップチェイン生成
        VkSwapchainCreateInfoKHR swapCreateInfo = {};
        swapCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapCreateInfo.surface          = surface->surface;
        swapCreateInfo.minImageCount    = requestImageCount;
        swapCreateInfo.imageFormat      = vkSwapchain->format;
        swapCreateInfo.imageColorSpace  = vkSwapchain->colorspace;
        swapCreateInfo.imageExtent      = extent;
        swapCreateInfo.imageArrayLayers = 1;
        swapCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapCreateInfo.preTransform     = surfaceTransformBits;
        swapCreateInfo.compositeAlpha   = compositeAlpha;
        swapCreateInfo.presentMode      = presentMode;
        swapCreateInfo.clipped          = true;

        //------------------------------------------------------------------------------------------
        // NOTE: VK_ERROR_NATIVE_WINDOW_IN_USE_KHR OpenGL でウィンドウコンテキストが生成されている場合に発生
        //------------------------------------------------------------------------------------------
        result = extensions.vkCreateSwapchainKHR(device, &swapCreateInfo, nullptr, &vkSwapchain->swapchain);
        SL_CHECK_VKRESULT(result, false);

        // イメージ取得
        uint32 imageCount = 0;
        result = extensions.vkGetSwapchainImagesKHR(device, vkSwapchain->swapchain, &imageCount, nullptr);
        SL_CHECK_VKRESULT(result, false);

        vkSwapchain->images.resize(imageCount);
        result = extensions.vkGetSwapchainImagesKHR(device, vkSwapchain->swapchain, &imageCount, vkSwapchain->images.data());
        SL_CHECK_VKRESULT(result, false);

        // イメージビュー生成
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format                      = vkSwapchain->format;
        viewCreateInfo.components.r                = VK_COMPONENT_SWIZZLE_R;
        viewCreateInfo.components.g                = VK_COMPONENT_SWIZZLE_G;
        viewCreateInfo.components.b                = VK_COMPONENT_SWIZZLE_B;
        viewCreateInfo.components.a                = VK_COMPONENT_SWIZZLE_A;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.layerCount = 1;

        for (uint32 i = 0; i < imageCount; i++)
        {
            VkImageView view = nullptr;
            viewCreateInfo.image = vkSwapchain->images[i];

            result = vkCreateImageView(device, &viewCreateInfo, nullptr, &view);
            SL_CHECK_VKRESULT(result, false);

            vkSwapchain->views.push_back(view);
        }

        // フレームバッファ生成
        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass      = vkSwapchain->renderpass->renderpass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.width           = surface->width;
        framebufferCreateInfo.height          = surface->height;
        framebufferCreateInfo.layers          = 1;

        for (uint32 i = 0; i < imageCount; i++)
        {
            VkFramebuffer framebuffer = nullptr;
            framebufferCreateInfo.pAttachments = &vkSwapchain->views[i];

            result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer);
            SL_CHECK_VKRESULT(result, false);

            vkSwapchain->framebuffers.push_back(framebuffer);
        }

        return true;
    }

    FramebufferHandle* VulkanAPI::GetSwapChainNextFramebuffer()
    {
        return nullptr;
    }

    RenderPass* VulkanAPI::GetSwapChainRenderPass(SwapChain* swapchain)
    {
        return nullptr;
    }

    RenderingFormat VulkanAPI::GetSwapChainFormat(SwapChain* swapchain)
    {
        return RenderingFormat();
    }

    void VulkanAPI::DestroySwapChain(SwapChain* swapchain)
    {
        if (swapchain)
        {
            VulkanSwapChain* vkSwapchain = (VulkanSwapChain*)swapchain;

            // フレームバッファ破棄
            for (uint32 i = 0; i < vkSwapchain->framebuffers.size(); i++)
                vkDestroyFramebuffer(device, vkSwapchain->framebuffers[i], nullptr);

            // イメージビュー破棄
            for (uint32 i = 0; i < vkSwapchain->images.size(); i++)
                vkDestroyImageView(device, vkSwapchain->views[i], nullptr);

            // イメージはスワップチェイン側が管理しているので破棄しない
            // ...

            // スワップチェイン破棄
            extensions.vkDestroySwapchainKHR(device, vkSwapchain->swapchain, nullptr);

            //レンダーパス破棄
            vkDestroyRenderPass(device, vkSwapchain->renderpass->renderpass, nullptr);

            Memory::Deallocate(swapchain);
        }
    }



    VkSampleCountFlagBits VulkanAPI::_GetSupportedSampleCounts(TextureSamples samples)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(context->GetPhysicalDevice(), &properties);

        VkSampleCountFlags sampleFlags = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

        // フラグが一致すれば、そのサンプル数を使用する
        if (sampleFlags & VulkanSampleCounts[samples])
        {
            return VulkanSampleCounts[samples];
        }
        else
        {
            // 一致しなければ、一致するまでサンプル数を下げながら、一致するサンプル数にフォールバック
            VkSampleCountFlagBits sampleBits = VulkanSampleCounts[samples];
            while (sampleBits > VK_SAMPLE_COUNT_1_BIT)
            {
                if (sampleFlags & sampleBits)
                    return sampleBits;

                sampleBits = (VkSampleCountFlagBits)(sampleBits >> 1);
            }
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    RenderPass* VulkanAPI::CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies)
    {
        // アタッチメント
        std::vector<VkAttachmentDescription2> vkAttachments(numAttachments);
        for (uint32 i = 0; i < numAttachments; i++)
        {
            vkAttachments[i] = {};
            vkAttachments[i].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR;
            vkAttachments[i].format         = VulkanFormats[attachments[i].format];
            vkAttachments[i].samples        = _GetSupportedSampleCounts(attachments[i].samples);
            vkAttachments[i].loadOp         = (VkAttachmentLoadOp)attachments[i].loadOp;
            vkAttachments[i].storeOp        = (VkAttachmentStoreOp)attachments[i].storeOp;
            vkAttachments[i].stencilLoadOp  = (VkAttachmentLoadOp)attachments[i].stencilLoadOp;
            vkAttachments[i].stencilStoreOp = (VkAttachmentStoreOp)attachments[i].stencilStoreOp;
            vkAttachments[i].initialLayout  = (VkImageLayout)attachments[i].initialLayout;
            vkAttachments[i].finalLayout    = (VkImageLayout)attachments[i].finalLayout;
        }

        // アタッチメント参照
        std::vector<VkAttachmentReference2> inputAttachmentRefs = {};
        std::vector<VkAttachmentReference2> colorAttachmentRefs = {};
        std::vector<VkAttachmentReference2> resolveAttachmentRef = {};
        std::vector<VkAttachmentReference2> depthstencilAttachmentRef = {};

        // サブパス
        std::vector<VkSubpassDescription2> vkSubpasses(numSubpasses);
        for (uint32 i = 0; i < numSubpasses; i++)
        {
            // 入力アタッチメント参照
            for (uint32 j = 0; j < subpasses[i].inputReferences.size(); j++)
            {
                VkAttachmentReference2 attachment = {};
                attachment.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                attachment.attachment = subpasses[i].inputReferences[j].attachment;
                attachment.layout     = (VkImageLayout)subpasses[i].inputReferences[j].layout;
                attachment.aspectMask = (VkImageAspectFlags)subpasses[i].inputReferences[j].aspect;

                inputAttachmentRefs.push_back(attachment);
            }

            // カラーアタッチメント参照
            for (uint32 j = 0; j < subpasses[i].colorReferences.size(); j++)
            {
                VkAttachmentReference2 attachment = {};
                attachment.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                attachment.attachment = subpasses[i].colorReferences[j].attachment;
                attachment.layout     = (VkImageLayout)subpasses[i].colorReferences[j].layout;
                attachment.aspectMask = (VkImageAspectFlags)subpasses[i].colorReferences[j].aspect;

                colorAttachmentRefs.push_back(attachment);
            }

            // マルチサンプル解決アタッチメント参照
            if (subpasses[i].resolveReferences.attachment != INVALID_RENDER_ID)
            {
                VkAttachmentReference2 attachment = {};
                attachment.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                attachment.attachment = subpasses[i].resolveReferences.attachment;
                attachment.layout     = (VkImageLayout)subpasses[i].resolveReferences.layout;
                attachment.aspectMask = (VkImageAspectFlags)subpasses[i].resolveReferences.aspect;

                resolveAttachmentRef.push_back(attachment);
            }

            // 深度ステンシルアタッチメント参照
            if (subpasses[i].depthstencilReference.attachment != INVALID_RENDER_ID)
            {
                VkAttachmentReference2 attachment = {};
                attachment = {};
                attachment.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                attachment.attachment = subpasses[i].depthstencilReference.attachment;
                attachment.layout     = (VkImageLayout)subpasses[i].depthstencilReference.layout;
                attachment.aspectMask = (VkImageAspectFlags)subpasses[i].depthstencilReference.aspect;

                depthstencilAttachmentRef.push_back(attachment);
            }

            vkSubpasses[i] = {};
            vkSubpasses[i].sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR;
            vkSubpasses[i].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            vkSubpasses[i].viewMask                = 0;
            vkSubpasses[i].inputAttachmentCount    = inputAttachmentRefs.size();
            vkSubpasses[i].pInputAttachments       = inputAttachmentRefs.data();
            vkSubpasses[i].colorAttachmentCount    = colorAttachmentRefs.size();
            vkSubpasses[i].pColorAttachments       = colorAttachmentRefs.data();
            vkSubpasses[i].pResolveAttachments     = resolveAttachmentRef.data();
            vkSubpasses[i].pDepthStencilAttachment = depthstencilAttachmentRef.data();
            vkSubpasses[i].preserveAttachmentCount = subpasses[i].preserveAttachments.size();
            vkSubpasses[i].pPreserveAttachments    = subpasses[i].preserveAttachments.data();
        }

        // サブパス依存関係
        std::vector<VkSubpassDependency2> vkSubpassDependencies(numSubpassDependencies);
        for (uint32 i = 0; i < numSubpassDependencies; i++)
        {
            vkSubpassDependencies[i] = {};
            vkSubpassDependencies[i].sType         = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            vkSubpassDependencies[i].srcSubpass    = subpassDependencies[i].srcSubpass;
            vkSubpassDependencies[i].dstSubpass    = subpassDependencies[i].dstSubpass;
            vkSubpassDependencies[i].srcStageMask  = (VkPipelineStageFlags)subpassDependencies[i].srcStages;
            vkSubpassDependencies[i].dstStageMask  = (VkPipelineStageFlags)subpassDependencies[i].dstStages;
            vkSubpassDependencies[i].srcAccessMask = (VkAccessFlags)subpassDependencies[i].srcAccess;
            vkSubpassDependencies[i].dstAccessMask = (VkAccessFlags)subpassDependencies[i].dstAccess;
        }

        // レンダーパス生成
        VkRenderPassCreateInfo2 createInfo = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
        createInfo.attachmentCount         = vkAttachments.size();
        createInfo.pAttachments            = vkAttachments.data();
        createInfo.subpassCount            = vkSubpasses.size();
        createInfo.pSubpasses              = vkSubpasses.data();
        createInfo.dependencyCount         = vkSubpassDependencies.size();
        createInfo.pDependencies           = vkSubpassDependencies.data();
        createInfo.correlatedViewMaskCount = 0;
        createInfo.pCorrelatedViewMasks    = nullptr;

        VkRenderPass vkRenderPass = nullptr;
        VkResult result = vkCreateRenderPass2(device, &createInfo, nullptr, &vkRenderPass);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanRenderPass* renderpass = Memory::Allocate<VulkanRenderPass>();
        renderpass->renderpass = vkRenderPass;

        return renderpass;
    }

    void VulkanAPI::DestroyRenderPass(RenderPass* renderpass)
    {
        if (renderpass)
        {
            VulkanRenderPass* vkRenderpass = (VulkanRenderPass*)renderpass;
            vkDestroyRenderPass(device, vkRenderpass->renderpass, nullptr);

            Memory::Deallocate(renderpass);
        }
    }




    // アセットはレンダーオブジェクトをメンバーに内包する形で表現
    // TextureAsset : public Asset { Texture* tex; }

    // アセットは Ref<TextureAsset> 形式で保持？
}
