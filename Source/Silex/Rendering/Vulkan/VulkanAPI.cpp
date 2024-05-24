
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
        SL_CHECK_VKRESULT(result, false);

        // 拡張機能関数ロード
        extensions.vkCreateSwapchainKHR    = GET_VULKAN_DEVICE_PROC(device, vkCreateSwapchainKHR);
        extensions.vkDestroySwapchainKHR   = GET_VULKAN_DEVICE_PROC(device, vkDestroySwapchainKHR);
        extensions.vkGetSwapchainImagesKHR = GET_VULKAN_DEVICE_PROC(device, vkGetSwapchainImagesKHR);
        extensions.vkAcquireNextImageKHR   = GET_VULKAN_DEVICE_PROC(device, vkAcquireNextImageKHR);
        extensions.vkQueuePresentKHR       = GET_VULKAN_DEVICE_PROC(device, vkQueuePresentKHR);
        extensions.vkCreateRenderPass2KHR  = GET_VULKAN_DEVICE_PROC(device, vkCreateRenderPass2KHR);

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
        result = extensions.vkCreateRenderPass2KHR(device, &passInfo, nullptr, &renderPass);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanSwapChain* swapchain = Memory::Allocate<VulkanSwapChain>();
        swapchain->surface    = ((VulkanSurface*)surface);
        swapchain->format     = format;
        swapchain->colorspace = colorspace;
        swapchain->renderpass = renderPass;

        return swapchain;
    }

    bool VulkanAPI::ResizeSwapChain(SwapChain* swapchain, uint32 requestFramebufferCount)
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

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        switch (surface->vsyncMode)
        {
            default:;
            case VSYNC_MODE_DISABLED: presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;    break;
            case VSYNC_MODE_MAILBOX:  presentMode = VK_PRESENT_MODE_MAILBOX_KHR;      break;
            case VSYNC_MODE_ENABLED:  presentMode = VK_PRESENT_MODE_FIFO_KHR;         break;
            case VSYNC_MODE_ADAPTIVE: presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR; break;
        }

        bool findRequestMode = false;
        for (uint32 i = 0; i < presentModes.size(); i++)
        {
            if (presentMode == presentModes[i])
            {
                findRequestMode = true;
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
        uint32 imageCount = std::clamp(requestFramebufferCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);

        // 生成情報
        VkSwapchainCreateInfoKHR swapCreateInfo = {};
        swapCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapCreateInfo.surface          = surface->surface;
        swapCreateInfo.minImageCount    = imageCount;
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

        return true;
    }




    // アセットはレンダーオブジェクトをメンバーに内包する形で表現
    // TextureAsset : public Asset { Texture* tex; }

    // アセットは Ref<TextureAsset> 形式で保持？
}
