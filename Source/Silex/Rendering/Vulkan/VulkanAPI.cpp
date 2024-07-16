
#include "PCH.h"

#include "Rendering/ShaderCompiler.h"
#include "Rendering/Vulkan/VulkanAPI.h"
#include "Rendering/Vulkan/VulkanContext.h"
#include "ImGui/Vulkan/VulkanGUI.h"


namespace Silex
{
    //==================================================================================
    // Vulkan ヘルパー
    //==================================================================================
    static constexpr uint32 MaxDescriptorsetPerPool = 64;

    // デスクリプターセットシグネチャから同一シグネチャのプールがあれば取得、なければ新規生成
    VkDescriptorPool VulkanAPI::_FindOrCreateDescriptorPool(const DescriptorSetPoolKey& key)
    {
        // プールが既に存在し、そのプールの参照カウントが MaxDescriptorsetPerPool 以下ならそのプールを使用
        const auto& find = descriptorsetPools.find(key);
        if (find != descriptorsetPools.end())
        {
            for (auto& [pool, count] : find->second)
            {
                if (count < MaxDescriptorsetPerPool)
                {
                    find->second[pool]++;
                    return pool;
                }
            }
        }

        // キーからデスクリプタータイプを設定
        VkDescriptorPoolSize* sizesPtr = SL_STACK(VkDescriptorPoolSize, DESCRIPTOR_TYPE_MAX);
        uint32 sizeCount = 0;
        {
            VkDescriptorPoolSize* sizes = sizesPtr;

            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_SAMPLER])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_SAMPLER;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_SAMPLER] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_IMAGE_SAMPLER])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_IMAGE_SAMPLER] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_IMAGE])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_IMAGE] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_STORAGE_IMAGE])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_STORAGE_IMAGE] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_UNIFORM_TEXTURE_BUFFER])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_UNIFORM_TEXTURE_BUFFER] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_STORAGE_TEXTURE_BUFFER])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_STORAGE_TEXTURE_BUFFER] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_UNIFORM_BUFFER])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_UNIFORM_BUFFER] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_STORAGE_BUFFER]) {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_STORAGE_BUFFER] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_INPUT_ATTACHMENT])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_INPUT_ATTACHMENT] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
        }

        // デスクリプタープール生成
        VkDescriptorPoolCreateInfo createInfo = {};
        createInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        createInfo.maxSets       = MaxDescriptorsetPerPool;
        createInfo.poolSizeCount = sizeCount;
        createInfo.pPoolSizes    = sizesPtr;

        VkDescriptorPool pool = nullptr;
        VkResult result = vkCreateDescriptorPool(device, &createInfo, nullptr, &pool);
        SL_CHECK_VKRESULT(result, nullptr);

        auto& itr = descriptorsetPools[key];
        itr[pool]++;

        return pool;
    }

    // プールの参照カウントを減らす、0なら破棄
    void VulkanAPI::_DecrementPoolRefCount(VkDescriptorPool pool, DescriptorSetPoolKey& poolKey)
    {
        const auto& itr = descriptorsetPools.find(poolKey);
        itr->second[pool]--;

        if (itr->second[pool] == 0)
        {
            vkDestroyDescriptorPool(device, pool, nullptr);
            itr->second.erase(pool);

            if (itr->second.empty())
            {
                descriptorsetPools.erase(itr);
            }
        }
    }

    // 指定されたサンプル数が、利用可能なサンプル数かどうかチェックする
    VkSampleCountFlagBits VulkanAPI::_CheckSupportedSampleCounts(TextureSamples samples)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(context->GetPhysicalDevice(), &properties);

        VkSampleCountFlags sampleFlags = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

        // フラグが一致すれば、そのサンプル数を使用する
        if (sampleFlags & (1 << samples))
        {
            return VkSampleCountFlagBits(1 << samples);
        }
        else
        {
            // 一致しなければ、一致するまでサンプル数を下げながら、一致するサンプル数にフォールバック
            VkSampleCountFlagBits sampleBits = VkSampleCountFlagBits(1 << samples);
            while (sampleBits > VK_SAMPLE_COUNT_1_BIT)
            {
                if (sampleFlags & sampleBits)
                    return sampleBits;

                sampleBits = (VkSampleCountFlagBits)(sampleBits >> 1);
            }
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }



    //==================================================================================
    // Vulkan API
    //==================================================================================
    VulkanAPI::VulkanAPI(VulkanContext* context)
    {
        this->context = context;
    }

    VulkanAPI::~VulkanAPI()
    {
        vkDeviceWaitIdle(device);

        if (allocator) vmaDestroyAllocator(allocator);
        if (device)    vkDestroyDevice(device, nullptr);
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

        VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {};
        VkPhysicalDeviceBufferAddressFeaturesEXT   bufferAdressFeatures       = {};


        // デバイス機能を取得
        VkPhysicalDeviceVulkan11Features features_11 = {};
        features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        features_11.pNext = nullptr;

        VkPhysicalDeviceVulkan12Features features_12 = {};
        features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features_12.pNext = &features_11;

        VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        features2.pNext = &features_12;
        vkGetPhysicalDeviceFeatures2(context->GetPhysicalDevice(), &features2);

        // 拡張ポインタチェイン
        void* createInfoNext = &features_12;

        // デバイス生成
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext                   = createInfoNext;
        deviceCreateInfo.queueCreateInfoCount    = queueCreateInfos.size();
        deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
        deviceCreateInfo.enabledExtensionCount   = context->GetEnabledDeviceExtensions().size();
        deviceCreateInfo.ppEnabledExtensionNames = context->GetEnabledDeviceExtensions().data();
        deviceCreateInfo.pEnabledFeatures        = &features2.features;

        // NOTE:===================================================================
        // 以前の実装では、インスタンスの検証レイヤーとデバイス固有の検証レイヤーが区別されていたが
        // 最新の実装では enabledLayerCount / ppEnabledLayerNames フィールドは無視される
        // ただし、古い実装との互換性を保つ必要があれば、設定する必要がある
        // ========================================================================
        //deviceCreateInfo.ppEnabledLayerNames = nullptr;
        //deviceCreateInfo.enabledLayerCount   = 0;

        VkResult result = vkCreateDevice(context->GetPhysicalDevice(), &deviceCreateInfo, nullptr, &device);
        SL_CHECK_VKRESULT(result, false);

        // コンテキストでも使用できるように格納する
        context->device = device;

        // 拡張機能関数ロード
        CreateSwapchainKHR    = GET_VULKAN_DEVICE_PROC(device, vkCreateSwapchainKHR);
        DestroySwapchainKHR   = GET_VULKAN_DEVICE_PROC(device, vkDestroySwapchainKHR);
        GetSwapchainImagesKHR = GET_VULKAN_DEVICE_PROC(device, vkGetSwapchainImagesKHR);
        AcquireNextImageKHR   = GET_VULKAN_DEVICE_PROC(device, vkAcquireNextImageKHR);
        QueuePresentKHR       = GET_VULKAN_DEVICE_PROC(device, vkQueuePresentKHR);

        // メモリアロケータ（VMA）生成
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = context->GetPhysicalDevice();
        allocatorInfo.device         = device;
        allocatorInfo.instance       = context->GetInstance();

        result = vmaCreateAllocator(&allocatorInfo, &allocator);
        SL_CHECK_VKRESULT(result, false);

        return true;
    }

    //==================================================================================
    // コマンドキュー
    //==================================================================================
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

            VulkanCommandQueue* vkqueue = (VulkanCommandQueue*)queue;
            Memory::Deallocate(vkqueue);
        }
    }

    QueueFamily VulkanAPI::QueryQueueFamily(QueueFamilyFlags queueFlag, Surface* surface) const
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

    bool VulkanAPI::Present(CommandQueue* queue, SwapChain* swapchain, CommandBuffer* commandbuffer, Fence* fence, Semaphore* render, Semaphore* present)
    {
        VulkanSwapChain*     vkswapchain      = (VulkanSwapChain*)swapchain;
        VulkanCommandQueue*  vkqueue          = (VulkanCommandQueue*)queue;
        VulkanCommandBuffer* vkcommandBuffer  = (VulkanCommandBuffer*)commandbuffer;
        VulkanFence*         vkfence          = (VulkanFence*)fence;
        VulkanSemaphore*     renderSemaphore  = (VulkanSemaphore*)render;
        VulkanSemaphore*     presentSemaphore = (VulkanSemaphore*)present;

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo = {};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext                = nullptr;
        submitInfo.pWaitDstStageMask    = &waitStage;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &vkcommandBuffer->commandBuffer;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &presentSemaphore->semaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &renderSemaphore->semaphore;

        VkResult result = vkQueueSubmit(vkqueue->queue, 1, &submitInfo, vkfence->fence);
        SL_CHECK_VKRESULT(result, false);


        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = &renderSemaphore->semaphore;
        presentInfo.swapchainCount      = 1;
        presentInfo.pSwapchains         = &vkswapchain->swapchain;
        presentInfo.pImageIndices       = &vkswapchain->imageIndex;

        result = vkQueuePresentKHR(vkqueue->queue, &presentInfo);
        SL_CHECK_VKRESULT(result, false);

        return true;
    }

    //==================================================================================
    // コマンドプール
    //==================================================================================
    CommandPool* VulkanAPI::CreateCommandPool(QueueFamily family, CommandBufferType type)
    {
        uint32 familyIndex = family;

        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; 
        commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // (必須) コマンドバッファがリセットできるようにする
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

            Memory::Deallocate(vkpool);
        }
    }

    //==================================================================================
    // コマンドバッファ
    //==================================================================================
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

            VulkanCommandBuffer* vkcommandBuffer = (VulkanCommandBuffer*)commandBuffer;
            Memory::Deallocate(vkcommandBuffer);
        }
    }

    bool VulkanAPI::BeginCommandBuffer(CommandBuffer* commandBuffer)
    {
        // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT フラグで生成されたプールから割り当てられたバッファは
        // vkResetCommandBufferを呼び出すか、vkBeginCommandBuffer の呼び出しで暗黙的に リセットされる。
        // また、このフラグで生成されていないプールから割り当てられたバッファは vkResetCommandBuffer 呼び出しをしてはいけない
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPoolCreateFlagBits.html

        VulkanCommandBuffer* vkcmdBuffer = (VulkanCommandBuffer*)commandBuffer;

        VkResult result = vkResetCommandBuffer(vkcmdBuffer->commandBuffer, 0);
        SL_CHECK_VKRESULT(result, false);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result = vkBeginCommandBuffer(vkcmdBuffer->commandBuffer, &beginInfo);
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

    //==================================================================================
    // セマフォ
    //==================================================================================
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

            Memory::Deallocate(vkSemaphore);
        }
    }

    //==================================================================================
    // フェンス
    //==================================================================================
    Fence* VulkanAPI::CreateFence()
    {
        VkFence vkfence = nullptr;
        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        createInfo.pNext = nullptr;

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

            Memory::Deallocate(vkfence);
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

    //==================================================================================
    // スワップチェイン
    //==================================================================================
    SwapChain* VulkanAPI::CreateSwapChain(Surface* surface, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode)
    {
        if (width == 0 || height == 0)
        {
            SL_LOG_LOCATION_ERROR("width, height 共に 0以上である必要があります");
            return nullptr;
        }

        VkResult result;

        VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();
        VkSurfaceKHR     vkSurface      = ((VulkanSurface*)surface)->surface;

        uint32 formatCount = 0;
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface, &formatCount, nullptr);
        SL_CHECK_VKRESULT(result, nullptr);

        VkSurfaceFormatKHR* formats = SL_STACK(VkSurfaceFormatKHR, formatCount);
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface, &formatCount, formats);
        SL_CHECK_VKRESULT(result, nullptr);

        VkFormat        format     = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        //================================================================================
        // BGRA / RGBA
        // https://www.reddit.com/r/vulkan/comments/p3iy0o/why_use_bgra_instead_of_rgba/
        //================================================================================
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

            for (uint32 i = 0; i < formatCount; i++)
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

        VkAttachmentDescription attachment = {};
        attachment.format         = format;
        attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;      // 描画 前 処理
        attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;     // 描画 後 処理
        attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // 
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 
        attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;        // イメージの初期レイアウト
        attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // レンダーパス終了後に自動で移行するレイアウト

        VkAttachmentReference colorReference = {};
        colorReference.attachment = 0;
        colorReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorReference;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo passInfo = {};
        passInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        passInfo.attachmentCount = 1;
        passInfo.pAttachments    = &attachment;
        passInfo.subpassCount    = 1;
        passInfo.pSubpasses      = &subpass;
        //passInfo.dependencyCount = 1;
        //passInfo.pDependencies   = &dependency;

        VkRenderPass vkRenderPass = nullptr;
        result = vkCreateRenderPass(device, &passInfo, nullptr, &vkRenderPass);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanRenderPass* renderpass = Memory::Allocate<VulkanRenderPass>();
        renderpass->renderpass = vkRenderPass;

        VulkanSwapChain* vkSwapchain = Memory::Allocate<VulkanSwapChain>();
        vkSwapchain->surface    = ((VulkanSurface*)surface);
        vkSwapchain->format     = format;
        vkSwapchain->colorspace = colorspace;
        vkSwapchain->renderpass = renderpass;

        // サーフェース仕様取得
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vkSurface, &surfaceCapabilities);
        SL_CHECK_VKRESULT(result, nullptr);

        // サイズ
        VkExtent2D extent;
        if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) // == (uint32)-1
        {
            // 0xFFFFFFFF の場合は、仕様が許す限り好きなサイズで生成できることを表す
            extent.width  = std::clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
            extent.height = std::clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
        }
        else
        {
            // 0xFFFFFFFF 以外は、指定された値で生成する必要がある
            extent = surfaceCapabilities.currentExtent;
        }

        // プレゼントモード
        uint32 presentModeCount = 0;
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface, &presentModeCount, nullptr);
        SL_CHECK_VKRESULT(result, nullptr);

        VkPresentModeKHR* presentModes = SL_STACK(VkPresentModeKHR, presentModeCount);
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface, &presentModeCount, presentModes);
        SL_CHECK_VKRESULT(result, nullptr);

        VkPresentModeKHR selectMode = (VkPresentModeKHR)mode;
        bool findRequestMode = false;
        for (uint32 i = 0; i < presentModeCount; i++)
        {
            if (selectMode == presentModes[i])
            {
                findRequestMode = true;
                break;
            }
        }

        // 見つからない場合は、必ずサポートされている（VK_PRESENT_MODE_FIFO_KHR）モードを選択
        if (!findRequestMode)
        {
            selectMode = VK_PRESENT_MODE_FIFO_KHR;
            SL_LOG_WARN("指定されたプレゼントモードがサポートされていないので、FIFOモードが選択されました。");
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
        swapCreateInfo.surface          = vkSurface;
        swapCreateInfo.minImageCount    = requestImageCount;
        swapCreateInfo.imageFormat      = vkSwapchain->format;
        swapCreateInfo.imageColorSpace  = vkSwapchain->colorspace;
        swapCreateInfo.imageExtent      = extent;
        swapCreateInfo.imageArrayLayers = 1;
        swapCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapCreateInfo.preTransform     = surfaceTransformBits;
        swapCreateInfo.compositeAlpha   = compositeAlpha;
        swapCreateInfo.presentMode      = selectMode;
        swapCreateInfo.clipped          = true;

        //------------------------------------------------------------------------------------------
        // NOTE: VK_ERROR_NATIVE_WINDOW_IN_USE_KHR OpenGL でウィンドウコンテキストが生成されている場合に発生
        //------------------------------------------------------------------------------------------
        result = CreateSwapchainKHR(device, &swapCreateInfo, nullptr, &vkSwapchain->swapchain);
        SL_CHECK_VKRESULT(result, nullptr);

        // イメージ取得
        uint32 imageCount = 0;
        result = GetSwapchainImagesKHR(device, vkSwapchain->swapchain, &imageCount, nullptr);
        SL_CHECK_VKRESULT(result, nullptr);

        vkSwapchain->images.resize(imageCount);
        result = GetSwapchainImagesKHR(device, vkSwapchain->swapchain, &imageCount, vkSwapchain->images.data());
        SL_CHECK_VKRESULT(result, nullptr);

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
            SL_CHECK_VKRESULT(result, nullptr);

            vkSwapchain->views.push_back(view);
        }

        // フレームバッファ生成
        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass      = vkSwapchain->renderpass->renderpass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.width           = extent.width;
        framebufferCreateInfo.height          = extent.height;
        framebufferCreateInfo.layers          = 1;

        for (uint32 i = 0; i < imageCount; i++)
        {
            framebufferCreateInfo.pAttachments = &vkSwapchain->views[i];

            VkFramebuffer framebuffer = nullptr;
            result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer);
            SL_CHECK_VKRESULT(result, nullptr);

            VulkanFramebuffer* vkFramebuffer = Memory::Allocate<VulkanFramebuffer>();
            vkFramebuffer->framebuffer = framebuffer;
            vkFramebuffer->rect        = { 0, 0, extent.width, extent.height };

            vkSwapchain->framebuffers.push_back(vkFramebuffer);
        }

        return vkSwapchain;
    }

    bool VulkanAPI::ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode)
    {
        if (width == 0 || height == 0)
        {
            return true;
        }

        //=============================================================
        // GPU待機
        //=============================================================
        VkResult result = vkDeviceWaitIdle(device);
        SL_CHECK_VKRESULT(result, false);


        VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();
        VulkanSwapChain* vkSwapchain    = (VulkanSwapChain*)swapchain;
        VulkanSurface*   vkSurface      = vkSwapchain->surface;

        // 解放コード
        {
            // フレームバッファ破棄
            for (uint32 i = 0; i < vkSwapchain->framebuffers.size(); i++)
            {
                VulkanFramebuffer* vkfb = (VulkanFramebuffer*)vkSwapchain->framebuffers[i];
                vkDestroyFramebuffer(device, vkfb->framebuffer, nullptr);
            }

            // イメージビュー破棄
            for (uint32 i = 0; i < vkSwapchain->images.size(); i++)
            {
                vkDestroyImageView(device, vkSwapchain->views[i], nullptr);
            }

            // スワップチェイン破棄
            DestroySwapchainKHR(device, vkSwapchain->swapchain, nullptr);

            vkSwapchain->framebuffers.clear();
            vkSwapchain->views.clear();
        }

        // サーフェース仕様取得
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vkSurface->surface, &surfaceCapabilities);
        SL_CHECK_VKRESULT(result, false);

        // サイズ
        VkExtent2D extent;
        if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) // == (uint32)-1
        {
            // 0xFFFFFFFF の場合は、仕様が許す限り好きなサイズで生成できることを表す
            extent.width  = std::clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
            extent.height = std::clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
        }
        else
        {
            // 0xFFFFFFFF 以外は、指定された値で生成する必要がある
            extent = surfaceCapabilities.currentExtent;
        }

        // プレゼントモード
        uint32 presentModeCount = 0;
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface->surface, &presentModeCount, nullptr);
        SL_CHECK_VKRESULT(result, false);

        VkPresentModeKHR* presentModes = SL_STACK(VkPresentModeKHR, presentModeCount);
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface->surface, &presentModeCount, presentModes);
        SL_CHECK_VKRESULT(result, false);

        VkPresentModeKHR selectMode = (VkPresentModeKHR)mode;
        bool findRequestMode = false;
        for (uint32 i = 0; i < presentModeCount; i++)
        {
            if (selectMode == presentModes[i])
            {
                findRequestMode = true;
                break;
            }
        }

        // 見つからない場合は、必ずサポートされている（VK_PRESENT_MODE_FIFO_KHR）モードを選択
        if (!findRequestMode)
        {
            selectMode = VK_PRESENT_MODE_FIFO_KHR;
            SL_LOG_WARN("指定されたプレゼントモードがサポートされていないので、FIFOモードが選択されました。");
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
        swapCreateInfo.surface          = vkSurface->surface;
        swapCreateInfo.minImageCount    = requestImageCount;
        swapCreateInfo.imageFormat      = vkSwapchain->format;
        swapCreateInfo.imageColorSpace  = vkSwapchain->colorspace;
        swapCreateInfo.imageExtent      = extent;
        swapCreateInfo.imageArrayLayers = 1;
        swapCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapCreateInfo.preTransform     = surfaceTransformBits;
        swapCreateInfo.compositeAlpha   = compositeAlpha;
        swapCreateInfo.presentMode      = selectMode;
        swapCreateInfo.clipped          = true;

        //------------------------------------------------------------------------------------------
        // NOTE: VK_ERROR_NATIVE_WINDOW_IN_USE_KHR OpenGL でウィンドウコンテキストが生成されている場合に発生
        //------------------------------------------------------------------------------------------
        result = CreateSwapchainKHR(device, &swapCreateInfo, nullptr, &vkSwapchain->swapchain);
        SL_CHECK_VKRESULT(result, false);

        // イメージ取得
        uint32 imageCount = 0;
        result = GetSwapchainImagesKHR(device, vkSwapchain->swapchain, &imageCount, nullptr);
        SL_CHECK_VKRESULT(result, false);

        vkSwapchain->images.resize(imageCount);
        result = GetSwapchainImagesKHR(device, vkSwapchain->swapchain, &imageCount, vkSwapchain->images.data());
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
        framebufferCreateInfo.width           = extent.width;
        framebufferCreateInfo.height          = extent.height;
        framebufferCreateInfo.layers          = 1;

        for (uint32 i = 0; i < imageCount; i++)
        {
            framebufferCreateInfo.pAttachments = &vkSwapchain->views[i];

            VkFramebuffer framebuffer = nullptr;
            result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer);
            SL_CHECK_VKRESULT(result, false);

            VulkanFramebuffer* vkFramebuffer = Memory::Allocate<VulkanFramebuffer>();
            vkFramebuffer->framebuffer = framebuffer;
            vkFramebuffer->rect        = { 0, 0, extent.width, extent.height };

            vkSwapchain->framebuffers.push_back(vkFramebuffer);
        }

        return true;
    }

    FramebufferHandle* VulkanAPI::GetCurrentBackBuffer(SwapChain* swapchain, Semaphore* present)
    {
        VulkanSwapChain* vkswapchain = (VulkanSwapChain*)swapchain;
        VulkanSemaphore* vkpresent   = (VulkanSemaphore*)present;

        VkResult result = AcquireNextImageKHR(device, vkswapchain->swapchain, UINT64_MAX, vkpresent->semaphore, nullptr, &vkswapchain->imageIndex);
        SL_CHECK_VKRESULT(result, nullptr);

        return vkswapchain->framebuffers[vkswapchain->imageIndex];
    }

    RenderPass* VulkanAPI::GetSwapChainRenderPass(SwapChain* swapchain)
    {
        VulkanSwapChain* vkswapchain = (VulkanSwapChain*)swapchain;
        return vkswapchain->renderpass;
    }

    RenderingFormat VulkanAPI::GetSwapChainFormat(SwapChain* swapchain)
    {
        VulkanSwapChain* vkswapchain = (VulkanSwapChain*)swapchain;
        return RenderingFormat(vkswapchain->format);
    }

    void VulkanAPI::DestroySwapChain(SwapChain* swapchain)
    {
        if (swapchain)
        {
            //=============================================================
            // GPU待機
            //=============================================================
            VkResult result = vkDeviceWaitIdle(device);
            SL_CHECK_VKRESULT(result, SL_DONT_USE);


            VulkanSwapChain* vkSwapchain = (VulkanSwapChain*)swapchain;

            // フレームバッファ破棄
            for (uint32 i = 0; i < vkSwapchain->framebuffers.size(); i++)
            {
                VulkanFramebuffer* vkfb = (VulkanFramebuffer*)vkSwapchain->framebuffers[i];
                vkDestroyFramebuffer(device, vkfb->framebuffer, nullptr);

                Memory::Deallocate(vkfb);
            }

            // イメージビュー破棄
            for (uint32 i = 0; i < vkSwapchain->images.size(); i++)
            {
                vkDestroyImageView(device, vkSwapchain->views[i], nullptr);
            }

            // イメージはスワップチェイン側が管理しているので破棄しない
            // ...

            // スワップチェイン破棄
            DestroySwapchainKHR(device, vkSwapchain->swapchain, nullptr);

            //レンダーパス破棄
            vkDestroyRenderPass(device, vkSwapchain->renderpass->renderpass, nullptr);

            Memory::Deallocate(vkSwapchain);
        }
    }


    // const int32 f = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;


    //==================================================================================
    // バッファ
    //==================================================================================
    Buffer* VulkanAPI::CreateBuffer(uint64 size, BufferUsageFlags usage, MemoryAllocationType memoryType)
    {
        bool isInCpu = memoryType == MEMORY_ALLOCATION_TYPE_CPU;
        bool isSrc   = usage & BUFFER_USAGE_TRANSFER_SRC_BIT;
        bool isDst   = usage & BUFFER_USAGE_TRANSFER_DST_BIT;

        VkBufferCreateInfo createInfo = {};
        createInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size        = size;
        createInfo.usage       = usage;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCreateInfo = {};
        if (isInCpu)
        {
            if (isSrc && !isDst)
            {
                // 正しい順序で書き込まれることを保証する
                allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }

            if (!isSrc && isDst)
            {
                // ランダムに読み込まれることを許す
                allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            }

            // マップ可能にするためのフラグ
            allocationCreateInfo.usage         = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            allocationCreateInfo.requiredFlags = (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        }
        else
        {
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        }

        VmaAllocation     allocation = nullptr;
        VmaAllocationInfo allocationInfo = {};

        VkBuffer vkbuffer = nullptr;
        VkResult result = vmaCreateBuffer(allocator, &createInfo, &allocationCreateInfo, &vkbuffer, &allocation, &allocationInfo);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanBuffer* buffer = Memory::Allocate<VulkanBuffer>();
        buffer->allocationHandle = allocation;
        buffer->allocationSize   = allocationInfo.size;
        buffer->size             = size;
        buffer->buffer           = vkbuffer;
        buffer->view             = nullptr;

        return buffer;
    }

    void VulkanAPI::DestroyBuffer(Buffer* buffer)
    {
        if (buffer)
        {
            VulkanBuffer* vkbuffer = (VulkanBuffer*)buffer;
            if (vkbuffer->view)
            {
                vkDestroyBufferView(device, vkbuffer->view, nullptr);
            }

            vmaDestroyBuffer(allocator, vkbuffer->buffer, vkbuffer->allocationHandle);
            Memory::Deallocate(vkbuffer);
        }
    }

    byte* VulkanAPI::MapBuffer(Buffer* buffer)
    {
        VulkanBuffer* vkbuffer = (VulkanBuffer*)buffer;

        void* mappedPtr = nullptr;
        VkResult result = vmaMapMemory(allocator, vkbuffer->allocationHandle, &mappedPtr);
        SL_CHECK_VKRESULT(result, nullptr);

        return (byte*)mappedPtr;
    }

    void VulkanAPI::UnmapBuffer(Buffer* buffer)
    {
        VulkanBuffer* vkbuffer = (VulkanBuffer*)buffer;
        vmaUnmapMemory(allocator, vkbuffer->allocationHandle);
    }


    //==================================================================================
    // テクスチャ
    //==================================================================================
    TextureHandle* VulkanAPI::CreateTexture(const TextureFormat& format)
    {
        VkResult result;

        bool isCube          = format.type == TEXTURE_TYPE_CUBE || format.type == TEXTURE_TYPE_CUBE_ARRAY;
        bool isInCpuMemory   = format.usageBits & TEXTURE_USAGE_CPU_READ_BIT;
        bool isDepthStencil  = format.usageBits & TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        auto sampleCountBits = _CheckSupportedSampleCounts(format.samples);

        // ==================== イメージ生成 ====================
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.flags         = isCube? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
        imageCreateInfo.imageType     = (VkImageType)format.type;
        imageCreateInfo.format        = (VkFormat)format.format;
        imageCreateInfo.mipLevels     = format.mipmap;
        imageCreateInfo.arrayLayers   = format.array;
        imageCreateInfo.samples       = sampleCountBits;
        imageCreateInfo.extent.width  = format.width;
        imageCreateInfo.extent.height = format.height;
        imageCreateInfo.extent.depth  = format.depth;
        imageCreateInfo.tiling        = isInCpuMemory? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage         = (VkImageUsageFlagBits)format.usageBits;
        imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // VMA アロケーション
        VmaAllocation     allocation     = nullptr;
        VmaAllocationInfo allocationInfo = {};

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.flags = isInCpuMemory? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0;
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        VkImage vkimage = nullptr;
        result = vmaCreateImage(allocator, &imageCreateInfo, &allocationCreateInfo, &vkimage, &allocation, &allocationInfo);
        SL_CHECK_VKRESULT(result, nullptr);

        // ==================== ビュー生成 ====================
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image                       = vkimage;
        viewCreateInfo.viewType                    = (VkImageViewType)format.type;
        viewCreateInfo.format                      = (VkFormat)format.format;
        viewCreateInfo.components.r                = VK_COMPONENT_SWIZZLE_R;
        viewCreateInfo.components.g                = VK_COMPONENT_SWIZZLE_G;
        viewCreateInfo.components.b                = VK_COMPONENT_SWIZZLE_B;
        viewCreateInfo.components.a                = VK_COMPONENT_SWIZZLE_A;
        viewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
        viewCreateInfo.subresourceRange.layerCount = imageCreateInfo.arrayLayers;
        viewCreateInfo.subresourceRange.aspectMask = isDepthStencil? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageView vkview = nullptr;
        result = vkCreateImageView(device, &viewCreateInfo, nullptr, &vkview);
        if (result != VK_SUCCESS)
        {
            vmaDestroyImage(allocator, vkimage, allocation);
            SL_LOG_LOCATION_ERROR(VkResultToString(result));

            return nullptr;
        }

        VulkanTexture* texture = Memory::Allocate<VulkanTexture>();
        texture->allocationHandle = allocation;
        texture->allocationInfo   = allocationInfo;
        texture->image            = vkimage;
        texture->imageView        = vkview;
        texture->extent           = imageCreateInfo.extent;

        vmaGetAllocationInfo(allocator, texture->allocationHandle, &texture->allocationInfo);

        return texture;
    }

    void VulkanAPI::DestroyTexture(TextureHandle* texture)
    {
        if (texture)
        {
            VulkanTexture* vktexture = (VulkanTexture*)texture;
            vkDestroyImageView(device, vktexture->imageView, nullptr);
            vmaDestroyImage(allocator, vktexture->image, vktexture->allocationHandle);

            Memory::Deallocate(vktexture);
        }
    }


    //==================================================================================
    // サンプラー
    //==================================================================================
    Sampler* VulkanAPI::CreateSampler(const SamplerInfo& info)
    {
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext                   = nullptr;
        createInfo.flags                   = 0;
        createInfo.magFilter               = (VkFilter)info.magFilter;
        createInfo.minFilter               = (VkFilter)info.minFilter;
        createInfo.mipmapMode              = (VkSamplerMipmapMode)info.mipFilter;
        createInfo.addressModeU            = (VkSamplerAddressMode)info.repeatU;
        createInfo.addressModeV            = (VkSamplerAddressMode)info.repeatV;
        createInfo.addressModeW            = (VkSamplerAddressMode)info.repeatW;
        createInfo.mipLodBias              = info.lodBias;
        createInfo.anisotropyEnable        = info.useAnisotropy;
        createInfo.maxAnisotropy           = info.anisotropyMax;
        createInfo.compareEnable           = info.enableCompare;
        createInfo.compareOp               = (VkCompareOp)info.compareOp;
        createInfo.minLod                  = info.minLod;
        createInfo.maxLod                  = info.maxLod;
        createInfo.borderColor             = (VkBorderColor)info.borderColor;
        createInfo.unnormalizedCoordinates = info.unnormalized;

        VkSampler vksampler = nullptr;
        VkResult result = vkCreateSampler(device, &createInfo, nullptr, &vksampler);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanSampler* sampler = Memory::Allocate<VulkanSampler>();
        sampler->sampler = vksampler;

        return sampler;
    }

    void VulkanAPI::DestroySampler(Sampler* sampler)
    {
        if (sampler)
        {
            VulkanSampler* vksampler = (VulkanSampler * )sampler;
            vkDestroySampler(device, vksampler->sampler, nullptr);

            Memory::Deallocate(vksampler);
        }
    }

    //==================================================================================
    // フレームバッファ
    //==================================================================================
    FramebufferHandle* VulkanAPI::CreateFramebuffer(RenderPass* renderpass, uint32 numTexture, TextureHandle* textures, uint32 width, uint32 height)
    {
        VkImageView* views = SL_STACK(VkImageView, numTexture);
        for (uint32 i = 0; i < numTexture; i++)
        {
            VulkanTexture* vktexture = (VulkanTexture*)textures;
            views[i] = vktexture[i].imageView;
        }

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass      = ((VulkanRenderPass*)renderpass)->renderpass;
        createInfo.attachmentCount = numTexture;
        createInfo.pAttachments    = views;
        createInfo.width           = width;
        createInfo.height          = height;
        createInfo.layers          = 1;

        VkFramebuffer vkfb = nullptr;
        VkResult result = vkCreateFramebuffer(device, &createInfo, nullptr, &vkfb);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanFramebuffer* framebuffer = Memory::Allocate<VulkanFramebuffer>();
        framebuffer->framebuffer = vkfb;
        framebuffer->rect.x      = 0;
        framebuffer->rect.y      = 0;
        framebuffer->rect.width  = width;
        framebuffer->rect.height = height;

        return framebuffer;
    }

    void VulkanAPI::DestroyFramebuffer(FramebufferHandle* framebuffer)
    {
        if (framebuffer)
        {
            VulkanFramebuffer* vkfb = (VulkanFramebuffer*)framebuffer;
            vkDestroyFramebuffer(device, vkfb->framebuffer, nullptr);

            Memory::Deallocate(vkfb);
        }
    }

    //==================================================================================
    // 頂点フォーマット
    //==================================================================================
    InputLayout* VulkanAPI::CreateInputLayout(uint32 numBindings, InputBinding* bindings)
    {
        VulkanInputLayout* vklayout = Memory::Allocate<VulkanInputLayout>();

        uint32 attribeIndex = 0;
        uint32 attribeSize  = 0;

        for (uint32 i = 0; i < numBindings; i++)
            attribeSize += bindings[i].attributes.size();

        vklayout->attributes.resize(attribeSize);
        vklayout->bindings.resize(numBindings);


        for (uint32 i = 0; i < numBindings; i++)
        {
            vklayout->bindings[i]           = {};
            vklayout->bindings[i].binding   = bindings[i].binding;
            vklayout->bindings[i].stride    = bindings[i].stride;
            vklayout->bindings[i].inputRate = bindings[i].frequency == VERTEX_FREQUENCY_INSTANCE ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

            uint32 attributeSize = bindings[i].attributes.size();

            for (uint32 j = 0; j < attributeSize; j++)
            {
                vklayout->attributes[attribeIndex + j]          = {};
                vklayout->attributes[attribeIndex + j].binding  =           bindings[i].binding;
                vklayout->attributes[attribeIndex + j].format   = (VkFormat)bindings[i].attributes[j].format;
                vklayout->attributes[attribeIndex + j].location =           bindings[i].attributes[j].location;
                vklayout->attributes[attribeIndex + j].offset   =           bindings[i].attributes[j].offset;
            }

            attribeIndex += attributeSize;
        }

        vklayout->createInfo = {};
        vklayout->createInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vklayout->createInfo.vertexBindingDescriptionCount   = vklayout->bindings.size();
        vklayout->createInfo.pVertexBindingDescriptions      = vklayout->bindings.data();
        vklayout->createInfo.vertexAttributeDescriptionCount = attribeSize;
        vklayout->createInfo.pVertexAttributeDescriptions    = vklayout->attributes.data();

        return vklayout;
    }

    void VulkanAPI::DestroyInputLayout(InputLayout* layout)
    {
        if (layout)
        {
            VulkanInputLayout* vklayout = (VulkanInputLayout*)layout;
            vklayout->bindings.clear();
            vklayout->attributes.clear();
            vklayout->createInfo = {};

            Memory::Deallocate(vklayout);
        }
    }

    //==================================================================================
    // レンダーパス
    //==================================================================================
    RenderPass* VulkanAPI::CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies)
    {
        // アタッチメント
        VkAttachmentDescription* vkAttachments = SL_STACK(VkAttachmentDescription, numAttachments);
        for (uint32 i = 0; i < numAttachments; i++)
        {
            vkAttachments[i] = {};
            vkAttachments[i].format         = (VkFormat)attachments[i].format;
            vkAttachments[i].samples        = _CheckSupportedSampleCounts(attachments[i].samples);
            vkAttachments[i].loadOp         = (VkAttachmentLoadOp)attachments[i].loadOp;
            vkAttachments[i].storeOp        = (VkAttachmentStoreOp)attachments[i].storeOp;
            vkAttachments[i].stencilLoadOp  = (VkAttachmentLoadOp)attachments[i].stencilLoadOp;
            vkAttachments[i].stencilStoreOp = (VkAttachmentStoreOp)attachments[i].stencilStoreOp;
            vkAttachments[i].initialLayout  = (VkImageLayout)attachments[i].initialLayout;
            vkAttachments[i].finalLayout    = (VkImageLayout)attachments[i].finalLayout;
        }

        // サブパス
        VkSubpassDescription* vkSubpasses = SL_STACK(VkSubpassDescription, numSubpasses);
        for (uint32 i = 0; i < numSubpasses; i++)
        {
            // 入力アタッチメント参照
            uint32 numInputAttachmentRef = subpasses[i].inputReferences.size();
            VkAttachmentReference* inputAttachmentRefs = SL_STACK(VkAttachmentReference, numInputAttachmentRef);
            for (uint32 j = 0; j < numInputAttachmentRef; j++)
            {
                *inputAttachmentRefs = {};
                inputAttachmentRefs[i].attachment = subpasses[i].inputReferences[j].attachment;
                inputAttachmentRefs[i].layout     = (VkImageLayout)subpasses[i].inputReferences[j].layout;
            }

            // カラーアタッチメント参照
            uint32 numColorAttachmentRef = subpasses[i].colorReferences.size();
            VkAttachmentReference* colorAttachmentRefs = SL_STACK(VkAttachmentReference, numColorAttachmentRef);
            for (uint32 j = 0; j < numColorAttachmentRef; j++)
            {
                *colorAttachmentRefs = {};
                colorAttachmentRefs[i].attachment = subpasses[i].colorReferences[j].attachment;
                colorAttachmentRefs[i].layout     = (VkImageLayout)subpasses[i].colorReferences[j].layout;
            }

            // マルチサンプル解決アタッチメント参照
            VkAttachmentReference* resolveAttachmentRef = nullptr;
            if (subpasses[i].resolveReferences.attachment != INVALID_RENDER_ID)
            {
                resolveAttachmentRef = SL_STACK(VkAttachmentReference, 1);

                *resolveAttachmentRef = {};
                resolveAttachmentRef->attachment = subpasses[i].resolveReferences.attachment;
                resolveAttachmentRef->layout     = (VkImageLayout)subpasses[i].resolveReferences.layout;
            }

            // 深度ステンシルアタッチメント参照
            VkAttachmentReference* depthstencilAttachmentRef = nullptr;
            if (subpasses[i].depthstencilReference.attachment != INVALID_RENDER_ID)
            {
                depthstencilAttachmentRef = SL_STACK(VkAttachmentReference, 1);

                *depthstencilAttachmentRef = {};
                depthstencilAttachmentRef->attachment = subpasses[i].depthstencilReference.attachment;
                depthstencilAttachmentRef->layout     = (VkImageLayout)subpasses[i].depthstencilReference.layout;
            }

            vkSubpasses[i] = {};
            vkSubpasses[i].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            vkSubpasses[i].inputAttachmentCount    = numInputAttachmentRef;
            vkSubpasses[i].pInputAttachments       = inputAttachmentRefs;
            vkSubpasses[i].colorAttachmentCount    = numColorAttachmentRef;
            vkSubpasses[i].pColorAttachments       = colorAttachmentRefs;
            vkSubpasses[i].pResolveAttachments     = resolveAttachmentRef;
            vkSubpasses[i].pDepthStencilAttachment = depthstencilAttachmentRef;
            vkSubpasses[i].preserveAttachmentCount = subpasses[i].preserveAttachments.size();
            vkSubpasses[i].pPreserveAttachments    = subpasses[i].preserveAttachments.data();
        }

        // サブパス依存関係
        VkSubpassDependency* vkSubpassDependencies = SL_STACK(VkSubpassDependency, numSubpassDependencies);
        for (uint32 i = 0; i < numSubpassDependencies; i++)
        {
            vkSubpassDependencies[i] = {};
            vkSubpassDependencies[i].srcSubpass    = subpassDependencies[i].srcSubpass;
            vkSubpassDependencies[i].dstSubpass    = subpassDependencies[i].dstSubpass;
            vkSubpassDependencies[i].srcStageMask  = (VkPipelineStageFlags)subpassDependencies[i].srcStages;
            vkSubpassDependencies[i].dstStageMask  = (VkPipelineStageFlags)subpassDependencies[i].dstStages;
            vkSubpassDependencies[i].srcAccessMask = (VkAccessFlags)subpassDependencies[i].srcAccess;
            vkSubpassDependencies[i].dstAccessMask = (VkAccessFlags)subpassDependencies[i].dstAccess;
        }

        // レンダーパス生成
        VkRenderPassCreateInfo createInfo = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount         = numAttachments;
        createInfo.pAttachments            = vkAttachments;
        createInfo.subpassCount            = numSubpasses;
        createInfo.pSubpasses              = vkSubpasses;
        createInfo.dependencyCount         = numSubpassDependencies;
        createInfo.pDependencies           = vkSubpassDependencies;

        VkRenderPass vkRenderPass = nullptr;
        VkResult result = vkCreateRenderPass(device, &createInfo, nullptr, &vkRenderPass);
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

            Memory::Deallocate(vkRenderpass);
        }
    }

    //==================================================================================
    // コマンド
    //==================================================================================
    void VulkanAPI::PipelineBarrier(CommandBuffer* commanddBuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrierInfo* memoryBarrier, uint32 numBufferBarrier, BufferBarrierInfo* bufferBarrier, uint32 numTextureBarrier, TextureBarrierInfo* textureBarrier)
    {
        VkMemoryBarrier* memoryBarriers = SL_STACK(VkMemoryBarrier, numMemoryBarrier);
        for (uint32 i = 0; i < numMemoryBarrier; i++)
        {
            memoryBarriers[i] = {};
            memoryBarriers[i].sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            memoryBarriers[i].srcAccessMask = (VkPipelineStageFlags)memoryBarrier[i].srcAccess;
            memoryBarriers[i].dstAccessMask = (VkAccessFlags)memoryBarrier[i].dstAccess;
        }

        VkBufferMemoryBarrier* bufferBarriers = SL_STACK(VkBufferMemoryBarrier, numBufferBarrier);
        for (uint32 i = 0; i < numBufferBarrier; i++)
        {
            bufferBarriers[i] = {};
            bufferBarriers[i].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferBarriers[i].srcAccessMask       = (VkAccessFlags)bufferBarrier[i].srcAccess;
            bufferBarriers[i].dstAccessMask       = (VkAccessFlags)bufferBarrier[i].dstAccess;
            bufferBarriers[i].buffer              = ((VulkanBuffer*)bufferBarrier[i].buffer)->buffer;
            bufferBarriers[i].offset              = bufferBarrier[i].offset;
            bufferBarriers[i].size                = bufferBarrier[i].size;
        }

        VkImageMemoryBarrier* imageBarriers = SL_STACK(VkImageMemoryBarrier, numTextureBarrier);
        for (uint32 i = 0; i < numTextureBarrier; i++)
        {
            VulkanTexture* vktexture = (VulkanTexture*)(textureBarrier[i].texture);
            imageBarriers[i] = {};
            imageBarriers[i].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[i].srcAccessMask                   = (VkAccessFlags)textureBarrier[i].srcAccess;
            imageBarriers[i].dstAccessMask                   = (VkAccessFlags)textureBarrier[i].dstAccess;
            imageBarriers[i].oldLayout                       = (VkImageLayout)textureBarrier[i].oldLayout;
            imageBarriers[i].newLayout                       = (VkImageLayout)textureBarrier[i].newLayout;
            imageBarriers[i].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[i].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            imageBarriers[i].image                           = vktexture->image;
            imageBarriers[i].subresourceRange.aspectMask     = (VkImageAspectFlags)textureBarrier[i].subresources.aspect;
            imageBarriers[i].subresourceRange.baseMipLevel   = textureBarrier[i].subresources.baseMipmap;
            imageBarriers[i].subresourceRange.levelCount     = textureBarrier[i].subresources.mipmapCount;
            imageBarriers[i].subresourceRange.baseArrayLayer = textureBarrier[i].subresources.baseLayer;
            imageBarriers[i].subresourceRange.layerCount     = textureBarrier[i].subresources.layerCount;
        }

        vkCmdPipelineBarrier(
            ((VulkanCommandBuffer*)commanddBuffer)->commandBuffer,
            (VkPipelineStageFlags)srcStage,
            (VkPipelineStageFlags)dstStage,
            (VkDependencyFlags)0,
            numMemoryBarrier,
            memoryBarriers,
            numBufferBarrier,
            bufferBarriers,
            numTextureBarrier,
            imageBarriers
        );
    }

    void VulkanAPI::ClearBuffer(CommandBuffer* commandbuffer, Buffer* buffer, uint64 offset, uint64 size)
    {
        VulkanBuffer* vkbuffer = (VulkanBuffer*)buffer;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdFillBuffer(cmd->commandBuffer, vkbuffer->buffer, offset, size, 0); // 0で埋める
    }

    void VulkanAPI::CopyBuffer(CommandBuffer* commandbuffer, Buffer* srcBuffer, Buffer* dstBuffer, uint32 numRegion, BufferCopyRegion* regions)
    {
        VulkanBuffer* src = (VulkanBuffer*)srcBuffer;
        VulkanBuffer* dst = (VulkanBuffer*)dstBuffer;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdCopyBuffer(cmd->commandBuffer, src->buffer, dst->buffer, numRegion, (VkBufferCopy*)regions);
    }

    void VulkanAPI::CopyTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureCopyRegion* regions)
    {
        VkImageCopy* copyRegion = SL_STACK(VkImageCopy, numRegion);
        for (uint32 i = 0; i < numRegion; i++)
        {
            copyRegion[i] = {};
            copyRegion[i].srcSubresource.aspectMask     = regions[i].srcSubresources.aspect;
            copyRegion[i].srcSubresource.baseArrayLayer = regions[i].srcSubresources.baseLayer;
            copyRegion[i].srcSubresource.layerCount     = regions[i].srcSubresources.layerCount;
            copyRegion[i].srcSubresource.mipLevel       = regions[i].srcSubresources.mipmap;
            copyRegion[i].srcOffset.x                   = regions[i].srcOffset.x;
            copyRegion[i].srcOffset.y                   = regions[i].srcOffset.y;
            copyRegion[i].srcOffset.z                   = regions[i].srcOffset.z;
            copyRegion[i].dstSubresource.aspectMask     = regions[i].dstSubresources.aspect;
            copyRegion[i].dstSubresource.baseArrayLayer = regions[i].dstSubresources.baseLayer;
            copyRegion[i].dstSubresource.layerCount     = regions[i].dstSubresources.layerCount;
            copyRegion[i].dstSubresource.mipLevel       = regions[i].dstSubresources.mipmap;
            copyRegion[i].dstOffset.x                   = regions[i].dstOffset.x;
            copyRegion[i].dstOffset.y                   = regions[i].dstOffset.y;
            copyRegion[i].dstOffset.z                   = regions[i].dstOffset.z;
            copyRegion[i].extent.width                  = regions[i].size.x;
            copyRegion[i].extent.height                 = regions[i].size.y;
            copyRegion[i].extent.depth                  = regions[i].size.z;
        }

        VulkanTexture* src = (VulkanTexture*)srcTexture;
        VulkanTexture* dst = (VulkanTexture*)dstTexture;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdCopyImage(cmd->commandBuffer, src->image, (VkImageLayout)srcTextureLayout, dst->image, (VkImageLayout)dstTextureLayout, numRegion, copyRegion);
    }

    void VulkanAPI::ResolveTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, uint32 srcLayer, uint32 srcMipmap, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 dstLayer, uint32 dstMipmap)
    {
        VulkanTexture* src = (VulkanTexture*)srcTexture;
        VulkanTexture* dst = (VulkanTexture*)dstTexture;

        VkImageResolve resolve = {};
        resolve.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        resolve.srcSubresource.mipLevel       = srcMipmap;
        resolve.srcSubresource.baseArrayLayer = srcLayer;
        resolve.srcSubresource.layerCount     = 1;
        resolve.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        resolve.dstSubresource.mipLevel       = dstMipmap;
        resolve.dstSubresource.baseArrayLayer = dstLayer;
        resolve.dstSubresource.layerCount     = 1;
        resolve.extent.width                  = std::max(1u, src->extent.width  >> srcMipmap);
        resolve.extent.height                 = std::max(1u, src->extent.height >> srcMipmap);
        resolve.extent.depth                  = std::max(1u, src->extent.depth  >> srcMipmap);

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdResolveImage(cmd->commandBuffer, src->image, (VkImageLayout)srcTextureLayout, dst->image, (VkImageLayout)dstTextureLayout, 1, &resolve);
    }

    void VulkanAPI::ClearColorTexture(CommandBuffer* commandbuffer, TextureHandle* texture, TextureLayout textureLayout, const glm::vec4& color, const TextureSubresourceRange& subresources)
    {
        VulkanTexture* vktexture = (VulkanTexture*)texture;

        VkClearColorValue clearColor = {};
        memcpy(&clearColor.float32, &color, sizeof(VkClearColorValue::float32));

        VkImageSubresourceRange vkSubresources = {};
        vkSubresources.aspectMask     = subresources.aspect;
        vkSubresources.baseMipLevel   = subresources.baseMipmap;
        vkSubresources.levelCount     = subresources.mipmapCount;
        vkSubresources.layerCount     = subresources.layerCount;
        vkSubresources.baseArrayLayer = subresources.baseLayer;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdClearColorImage(cmd->commandBuffer, vktexture->image, (VkImageLayout)textureLayout, &clearColor, 1, &vkSubresources);
    }

    void VulkanAPI::CopyBufferToTexture(CommandBuffer* commandbuffer, Buffer* srcBuffer, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, BufferTextureCopyRegion* regions)
    {
        VkBufferImageCopy* copyRegion = SL_STACK(VkBufferImageCopy, numRegion);
        for (uint32 i = 0; i < numRegion; i++)
        {
            copyRegion[i] = {};
            copyRegion[i].bufferOffset                    = regions[i].bufferOffset;
            copyRegion[i].imageExtent.width               = regions[i].textureRegionSize.x;
            copyRegion[i].imageExtent.height              = regions[i].textureRegionSize.y;
            copyRegion[i].imageExtent.depth               = regions[i].textureRegionSize.z;
            copyRegion[i].imageOffset.x                   = regions[i].textureOffset.x;
            copyRegion[i].imageOffset.y                   = regions[i].textureOffset.y;
            copyRegion[i].imageOffset.z                   = regions[i].textureOffset.z;
            copyRegion[i].imageSubresource.aspectMask     = regions[i].textureSubresources.aspect;
            copyRegion[i].imageSubresource.baseArrayLayer = regions[i].textureSubresources.baseLayer;
            copyRegion[i].imageSubresource.layerCount     = regions[i].textureSubresources.layerCount;
            copyRegion[i].imageSubresource.mipLevel       = regions[i].textureSubresources.mipmap;
        }

        VulkanBuffer*  src = (VulkanBuffer*)srcBuffer;
        VulkanTexture* dst = (VulkanTexture*)dstTexture;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdCopyBufferToImage(cmd->commandBuffer, src->buffer, dst->image, (VkImageLayout)dstTextureLayout, numRegion, copyRegion);
    }

    void VulkanAPI::CopyTextureToBuffer(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, Buffer* dstBuffer, uint32 numRegion, BufferTextureCopyRegion* regions)
    {
        VkBufferImageCopy* copyRegion = SL_STACK(VkBufferImageCopy, numRegion);
        for (uint32 i = 0; i < numRegion; i++)
        {
            copyRegion[i] = {};
            copyRegion[i].bufferOffset                    = regions[i].bufferOffset;
            copyRegion[i].imageExtent.width               = regions[i].textureRegionSize.x;
            copyRegion[i].imageExtent.height              = regions[i].textureRegionSize.y;
            copyRegion[i].imageExtent.depth               = regions[i].textureRegionSize.z;
            copyRegion[i].imageOffset.x                   = regions[i].textureOffset.x;
            copyRegion[i].imageOffset.y                   = regions[i].textureOffset.y;
            copyRegion[i].imageOffset.z                   = regions[i].textureOffset.z;
            copyRegion[i].imageSubresource.aspectMask     = regions[i].textureSubresources.aspect;
            copyRegion[i].imageSubresource.baseArrayLayer = regions[i].textureSubresources.baseLayer;
            copyRegion[i].imageSubresource.layerCount     = regions[i].textureSubresources.layerCount;
            copyRegion[i].imageSubresource.mipLevel       = regions[i].textureSubresources.mipmap;
        }

        VulkanTexture* src = (VulkanTexture*)srcTexture;
        VulkanBuffer*  dst = (VulkanBuffer*)dstBuffer;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdCopyImageToBuffer(cmd->commandBuffer, src->image, (VkImageLayout)srcTextureLayout, dst->buffer, numRegion, copyRegion);
    }



    void VulkanAPI::PushConstants(CommandBuffer* commandbuffer, ShaderHandle* shader, uint32 firstIndex, uint32* data, uint32 numData)
    {
        VulkanShader* vkshader = (VulkanShader*)shader;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdPushConstants(cmd->commandBuffer, vkshader->pipelineLayout, vkshader->stageFlags, firstIndex * sizeof(uint32), numData * sizeof(uint32), data);
    }

    void VulkanAPI::BeginRenderPass(CommandBuffer* commandbuffer, RenderPass* renderpass, FramebufferHandle* framebuffer, CommandBufferType commandBufferType, uint32 numclearValues, RenderPassClearValue* clearvalues)
    {
        VulkanFramebuffer* vkframebuffer = (VulkanFramebuffer*)framebuffer;

        VkRenderPassBeginInfo begineInfo = {};
        begineInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begineInfo.renderPass               = (VkRenderPass)((VulkanRenderPass*)(renderpass))->renderpass;
        begineInfo.framebuffer              = vkframebuffer->framebuffer;
        begineInfo.renderArea.offset.x      = vkframebuffer->rect.x;
        begineInfo.renderArea.offset.y      = vkframebuffer->rect.y;
        begineInfo.renderArea.extent.width  = vkframebuffer->rect.width;
        begineInfo.renderArea.extent.height = vkframebuffer->rect.height;
        begineInfo.clearValueCount          = numclearValues;
        begineInfo.pClearValues             = (VkClearValue*)clearvalues;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdBeginRenderPass(cmd->commandBuffer, &begineInfo, commandBufferType == COMMAND_BUFFER_TYPE_PRIMARY? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    }

    void VulkanAPI::EndRenderPass(CommandBuffer* commandbuffer)
    {
        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdEndRenderPass(cmd->commandBuffer);
    }

    void VulkanAPI::NextRenderSubpass(CommandBuffer* commandbuffer, CommandBufferType commandBufferType)
    {
        VkSubpassContents vksubpassContents = commandBufferType == COMMAND_BUFFER_TYPE_PRIMARY? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdNextSubpass(cmd->commandBuffer, vksubpassContents);
    }

    void VulkanAPI::SetViewport(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height)
    {
        // ビューポート Y座標 反転
        // https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/

        VkViewport viewport = {};
        viewport.x        =  (float)x;
        viewport.y        =  (float)height - y;
        viewport.width    =  (float)width;
        viewport.height   = -(float)height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdSetViewport(cmd->commandBuffer, 0, 1, &viewport);
    }

    void VulkanAPI::SetScissor(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height)
    {
        VkRect2D rect = {};
        rect.offset = { (int32)x, (int32)y };
        rect.extent = { width, height};

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdSetScissor(cmd->commandBuffer, 0, 1, &rect);
    }

    void VulkanAPI::ClearAttachments(CommandBuffer* commandbuffer, uint32 numAttachmentClear, AttachmentClear** attachmentClears, uint32 x, uint32 y, uint32 width, uint32 height)
    {
        VkClearAttachment* vkclears = SL_STACK(VkClearAttachment, numAttachmentClear);

        for (uint32 i = 0; i < numAttachmentClear; i++)
        {
            vkclears[i] = {};
            memcpy(&vkclears[i].clearValue, &attachmentClears[i]->value, sizeof(VkClearValue));
            vkclears[i].colorAttachment = attachmentClears[i]->colorAttachment;
            vkclears[i].aspectMask      = attachmentClears[i]->aspect;
        }

        VkClearRect vkrects = {};
        vkrects.rect.offset.x      = x;
        vkrects.rect.offset.y      = y;
        vkrects.rect.extent.width  = width;
        vkrects.rect.extent.height = height;
        vkrects.baseArrayLayer     = 0;
        vkrects.layerCount         = 1;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdClearAttachments(cmd->commandBuffer, numAttachmentClear, vkclears, 1, &vkrects);
    }

    void VulkanAPI::BindPipeline(CommandBuffer* commandbuffer, Pipeline* pipeline)
    {
        VulkanPipeline* vkpipeline = (VulkanPipeline*)pipeline;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdBindPipeline(cmd->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipeline->pipeline);
    }

    void VulkanAPI::BindDescriptorSet(CommandBuffer* commandbuffer, DescriptorSet* descriptorset, uint32 setIndex)
    {
        VulkanDescriptorSet* vkdescriptorset = (VulkanDescriptorSet*)descriptorset;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdBindDescriptorSets(cmd->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkdescriptorset->pipelineLayout, setIndex, 1, &vkdescriptorset->descriptorSet, 0, nullptr);
    }

    void VulkanAPI::Draw(CommandBuffer* commandbuffer, uint32 vertexCount, uint32 instanceCount, uint32 baseVertex, uint32 firstInstance)
    {
        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdDraw(cmd->commandBuffer, vertexCount, instanceCount, firstInstance, firstInstance);
    }

    void VulkanAPI::DrawIndexed(CommandBuffer* commandbuffer, uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance)
    {
        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdDrawIndexed(cmd->commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanAPI::BindVertexBuffers(CommandBuffer* commandbuffer, uint32 bindingCount, Buffer** buffers, uint64* offsets)
    {
        VkBuffer* vkbuffers = SL_STACK(VkBuffer, bindingCount);
        for (uint32 i = 0; i < bindingCount; i++)
        {
            VulkanBuffer* buf = ((VulkanBuffer*)buffers[i]);
            vkbuffers[i] = buf->buffer;
        }

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdBindVertexBuffers(cmd->commandBuffer, 0, bindingCount, vkbuffers, offsets);
    }

    void VulkanAPI::BindIndexBuffer(CommandBuffer* commandbuffer, Buffer* buffer, IndexBufferFormat format, uint64 offset)
    {
        VulkanBuffer* buf = (VulkanBuffer*)buffer;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdBindIndexBuffer(cmd->commandBuffer, buf->buffer, offset, format == INDEX_BUFFER_FORMAT_UINT16? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    }


    //==================================================================================
    // 即時コマンド
    //==================================================================================
    bool VulkanAPI::ImmidiateExcute(CommandQueue* queue, CommandBuffer* commandBuffer, Fence* fence, std::function<bool(CommandBuffer*)>&& func)
    {
        VkCommandBuffer vkcmd   = ((VulkanCommandBuffer*)commandBuffer)->commandBuffer;
        VkQueue         vkqueue = ((VulkanCommandQueue*)queue)->queue;
        VkFence         vkfence = ((VulkanFence*)fence)->fence;

        VkResult vkresult = vkResetFences(device, 1, &vkfence);
        SL_CHECK_VKRESULT(vkresult, false);

        {
            bool result = BeginCommandBuffer(commandBuffer);
            SL_CHECK(!result, false);

            result = func(commandBuffer);
            SL_CHECK(!result, false);

            result = EndCommandBuffer(commandBuffer);
            SL_CHECK(!result, false);
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &vkcmd;

        vkresult = vkQueueSubmit(vkqueue, 1, &submitInfo, vkfence);
        SL_CHECK_VKRESULT(vkresult, false);

        vkresult = vkWaitForFences(device, 1, &vkfence, true, UINT64_MAX);
        SL_CHECK_VKRESULT(vkresult, false);
    }

    //==================================================================================
    // シェーダー
    //==================================================================================
    ShaderHandle* VulkanAPI::CreateShader(const ShaderCompiledData& compiledData)
    {
        VkResult result;
        ShaderReflectionData reflectData = compiledData.reflection;
        const auto& spirvData            = compiledData.shaderBinaries;

        uint32 numDescriptorsets = reflectData.descriptorSets.size();
        uint32 numPushConstants  = reflectData.pushConstantRanges.size();

        std::vector<VkDescriptorSetLayoutBinding>    layoutBindings;
        std::vector<VkDescriptorSetLayout>           layouts(numDescriptorsets);
        std::vector<VkPushConstantRange>             pushConstantRanges(numPushConstants);
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        // デスクリプターセットレイアウト
        for (uint32 setIndex = 0; setIndex < numDescriptorsets; setIndex++)
        {
            const ShaderDescriptorSet& descriptorsets = reflectData.descriptorSets[setIndex];

            // ユニフォーム
            for (const auto& [index, uniform] : descriptorsets.uniformBuffers)
            {
                VkDescriptorSetLayoutBinding& binding = layoutBindings.emplace_back();
                binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                binding.binding            = index;
                binding.descriptorCount    = 1;
                binding.stageFlags         = uniform.stage;
                binding.pImmutableSamplers = nullptr;
            }

            // ストレージ
            for (const auto& [index, storage] : descriptorsets.storageBuffers)
            {
                VkDescriptorSetLayoutBinding& binding = layoutBindings.emplace_back();
                binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                binding.binding            = index;
                binding.descriptorCount    = 1;
                binding.stageFlags         = storage.stage;
                binding.pImmutableSamplers = nullptr;
            }

            // イメージサンプラー
            for (const auto& [index, storage] : descriptorsets.imageSamplers)
            {
                VkDescriptorSetLayoutBinding& binding = layoutBindings.emplace_back();
                binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                binding.binding            = index;
                binding.descriptorCount    = storage.arraySize;
                binding.stageFlags         = storage.stage;
                binding.pImmutableSamplers = nullptr;
            }

            // イメージ
            for (const auto& [index, imageSampler] : descriptorsets.separateTextures)
            {
                VkDescriptorSetLayoutBinding& binding = layoutBindings.emplace_back();
                binding.descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                binding.descriptorCount    = imageSampler.arraySize;
                binding.stageFlags         = imageSampler.stage;
                binding.pImmutableSamplers = nullptr;
                binding.binding            = index;
            }

            // サンプラー
            for (const auto& [index, imageSampler] : descriptorsets.separateSamplers)
            {
                VkDescriptorSetLayoutBinding& binding = layoutBindings.emplace_back();
                binding.descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLER;
                binding.descriptorCount    = imageSampler.arraySize;
                binding.stageFlags         = imageSampler.stage;
                binding.pImmutableSamplers = nullptr;
                binding.binding            = index;
            }

            // ストレージイメージ
            for (auto& [index, imageSampler] : descriptorsets.storageImages)
            {
                VkDescriptorSetLayoutBinding& binding = layoutBindings.emplace_back();
                binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                binding.descriptorCount    = imageSampler.arraySize;
                binding.stageFlags         = imageSampler.stage;
                binding.pImmutableSamplers = nullptr;
                binding.binding            = index;
            }

            // ユニフォーム テクセル
            //for (auto& [index, ] : descriptorsets.)
            //{
            //}

            // ストレージ テクセル
            //for (auto& [index, ] : descriptorsets.)
            //{
            //}

            // インプットアタッチメント
            //for (auto& [index, ] : descriptorsets.)
            //{
            //}

            // デスクリプターセットレイアウト生成
            VkDescriptorSetLayoutCreateInfo descriptorsetLayoutCreateInfo = {};
            descriptorsetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorsetLayoutCreateInfo.pNext        = nullptr;
            descriptorsetLayoutCreateInfo.bindingCount = layoutBindings.size();
            descriptorsetLayoutCreateInfo.pBindings    = layoutBindings.data();

            VkDescriptorSetLayout vkdescriptorsetLayout = nullptr;
            result = vkCreateDescriptorSetLayout(device, &descriptorsetLayoutCreateInfo ,nullptr, &vkdescriptorsetLayout);
            SL_CHECK_VKRESULT(result, nullptr);

            layouts[setIndex] = vkdescriptorsetLayout;
            layoutBindings.clear();
        }

        // プッシュ定数レンジ
        for (uint32 i = 0; i < pushConstantRanges.size(); i++)
        {
            pushConstantRanges[i].stageFlags = reflectData.pushConstantRanges[i].stage;
            pushConstantRanges[i].offset     = reflectData.pushConstantRanges[i].offset;
            pushConstantRanges[i].size       = reflectData.pushConstantRanges[i].size;
        }

        // パイプラインレイアウト
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext                  = nullptr;
        pipelineLayoutCreateInfo.setLayoutCount         = layouts.size();
        pipelineLayoutCreateInfo.pSetLayouts            = layouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutCreateInfo.pPushConstantRanges    = pushConstantRanges.data();

        VkPipelineLayout vkpipelineLayout = nullptr;
        result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &vkpipelineLayout);
        SL_CHECK_VKRESULT(result, nullptr);

        // シェーダーモジュール
        VkShaderStageFlags stageFlags = 0;
        for (const auto& [stage, binary] : spirvData)
        {
            VkShaderModuleCreateInfo moduleCreateInfo = {};
            moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleCreateInfo.codeSize = binary.size() * sizeof(uint32);
            moduleCreateInfo.pCode    = binary.data();

            // シェーダーモジュール生成
            VkShaderModule shaderModule = nullptr;
            result = vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule);
            SL_CHECK_VKRESULT(result, nullptr);

            // シェーダーステージ情報格納
            VkPipelineShaderStageCreateInfo shaderStage = {};
            shaderStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStage.stage  = (VkShaderStageFlagBits)stage;
            shaderStage.module = shaderModule;
            shaderStage.pName  = "main";

            shaderStages.push_back(shaderStage);
            stageFlags |= (VkShaderStageFlagBits)stage;
        }

        // Vulkanデータ生成
        VulkanShader* vkshader = Memory::Allocate<VulkanShader>();
        vkshader->descriptorsetLayouts = layouts;
        vkshader->stageFlags           = stageFlags;
        vkshader->pipelineLayout       = vkpipelineLayout;
        vkshader->stageCreateInfos     = shaderStages;

        return vkshader;
    }

    void VulkanAPI::DestroyShader(ShaderHandle* shader)
    {
        if (shader)
        {
            VulkanShader* vkshader = (VulkanShader*)shader;

            for (uint32 i = 0; i < vkshader->descriptorsetLayouts.size(); i++)
            {
                vkDestroyDescriptorSetLayout(device, vkshader->descriptorsetLayouts[i], nullptr);
            }

            vkDestroyPipelineLayout(device, vkshader->pipelineLayout, nullptr);

            for (uint32 i = 0; i < vkshader->stageCreateInfos.size(); i++)
            {
                vkDestroyShaderModule(device, vkshader->stageCreateInfos[i].module, nullptr);
            }

            Memory::Deallocate(vkshader);
        }
    }

    //==================================================================================
    // デスクリプター
    //==================================================================================
    DescriptorSet* VulkanAPI::CreateDescriptorSet(uint32 numdescriptors, DescriptorInfo* descriptors, ShaderHandle* shader, uint32 setIndex)
    {
        VulkanShader* vkShader = (VulkanShader*)shader;

        // 同一シグネチャ（型と数の一致）のプールを識別するキー ※同一プールは デフォルトで64個まで確保され、超えた場合は別プールが確保される
        DescriptorSetPoolKey key = {};

        // デスクリプタに実データ（イメージ・バッファ）書き込み
        VkWriteDescriptorSet* writes = SL_STACK(VkWriteDescriptorSet, numdescriptors);
        for (uint32 i = 0; i < numdescriptors; i++)
        {
            uint32 numDescriptors = 1;
            const DescriptorInfo& descriptor = descriptors[i];

            writes[i] = {};
            writes[i].sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstBinding = descriptor.binding;

            switch (descriptor.type)
            {
                // ユニフォームバッファ
                case DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                {
                    VkDescriptorBufferInfo* bufferInfo = SL_STACK(VkDescriptorBufferInfo, 1);

                    VulkanBuffer* buffer = (VulkanBuffer*)descriptor.handles[0];
                    *bufferInfo = {};
                    bufferInfo->buffer = buffer->buffer;
                    bufferInfo->range  = buffer->size;

                    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    writes[i].pBufferInfo    = bufferInfo;

                    break;
                }

                // ストレージバッファ
                case DESCRIPTOR_TYPE_STORAGE_BUFFER:
                {
                    VkDescriptorBufferInfo* bufferInfo = SL_STACK(VkDescriptorBufferInfo, 1);

                    VulkanBuffer* buffer = (VulkanBuffer*)descriptor.handles[0];
                    *bufferInfo = {};
                    bufferInfo->buffer = buffer->buffer;
                    bufferInfo->range  = buffer->size;

                    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    writes[i].pBufferInfo    = bufferInfo;

                    break;
                }
                
                // サンプラー
                case DESCRIPTOR_TYPE_SAMPLER:
                {
                    numDescriptors = descriptor.handles.size();
                    VkDescriptorImageInfo* imgInfos = SL_STACK(VkDescriptorImageInfo, numDescriptors);

                    for (uint32 j = 0; j < numDescriptors; j++)
                    {
                        imgInfos[j] = {};
                        imgInfos[j].sampler     = ((VulkanSampler*)(descriptor.handles[j]))->sampler;
                        imgInfos[j].imageView   = nullptr;
                        imgInfos[j].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    }

                    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                    writes[i].pImageInfo     = imgInfos;

                    break;
                }

                // テクスチャ
                case DESCRIPTOR_TYPE_IMAGE:
                {
                    numDescriptors = descriptor.handles.size();
                    VkDescriptorImageInfo* imageInfos = SL_STACK(VkDescriptorImageInfo, numDescriptors);

                    for (uint32 j = 0; j < numDescriptors; j++)
                    {
                        imageInfos[j] = {};
                        imageInfos[j].imageView   = ((VulkanTexture*)(descriptor.handles[j]))->imageView;
                        imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }

                    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    writes[i].pImageInfo     = imageInfos;

                    break;
                }

                // テクスチャ + サンプル
                case DESCRIPTOR_TYPE_IMAGE_SAMPLER:
                {
                    // サンプラーとイメージがセットで1つのデスクリプタとして扱うため
                    numDescriptors = descriptor.handles.size() / 2;
                    VkDescriptorImageInfo* imageInfos = SL_STACK(VkDescriptorImageInfo, numDescriptors);

                    for (uint32 j = 0; j < numDescriptors; j++) 
                    {
                        imageInfos[j] = {};
                        imageInfos[j].sampler     = ((VulkanSampler*)(descriptor.handles[j * 2 + 0]))->sampler;
                        imageInfos[j].imageView   = ((VulkanTexture*)(descriptor.handles[j * 2 + 1]))->imageView;
                        imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }

                    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    writes[i].pImageInfo     = imageInfos;

                    break;
                }

                // ストレージイメージ
                case DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    numDescriptors = descriptor.handles.size();
                    VkDescriptorImageInfo* imageInfos = SL_STACK(VkDescriptorImageInfo, numDescriptors);

                    for (uint32 j = 0; j < numDescriptors; j++)
                    {
                        imageInfos[j] = {};
                        imageInfos[j].imageView   = ((VulkanTexture*)(descriptor.handles[j]))->imageView;
                        imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    }

                    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    writes[i].pImageInfo     = imageInfos;

                    break;
                }

                // テクセルバッファ
                case DESCRIPTOR_TYPE_UNIFORM_TEXTURE_BUFFER:
                {
                    SL_ASSERT(false, "未実装");
                    break;
                }

                // ストレージ テクセルバッファ
                case DESCRIPTOR_TYPE_STORAGE_TEXTURE_BUFFER:
                {
                    SL_ASSERT(false, "未実装");
                    break;
                }

                // インプットアタッチメント
                case DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                {
                    SL_ASSERT(false, "未実装");
                    break;
                }

                default: SL_ASSERT(false);
            }

            // キーに使用デスクリプタ型の個数を加算
            writes[i].descriptorCount = numDescriptors;
            key.descriptorTypeCounts[descriptor.type] += numDescriptors;
        }

        // デスクリプタープール取得 (keyをもとに、生成済みのプールがあれば取得、なければ新規生成)
        VkDescriptorPool vkPool = _FindOrCreateDescriptorPool(key);
        SL_CHECK(!vkPool, nullptr);

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool     = vkPool;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        descriptorSetAllocateInfo.pSetLayouts        = &vkShader->descriptorsetLayouts[setIndex];

        // デスクリプターセット生成
        VkDescriptorSet vkdescriptorset = nullptr;
        VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &vkdescriptorset);
        if (result != VK_SUCCESS)
        {
            _DecrementPoolRefCount(vkPool, key);
            
            SL_LOG_LOCATION_ERROR(VkResultToString(result));
            return nullptr;
        }

        // デスクリプターセット更新
        {
            for (uint32 i = 0; i < numdescriptors; i++)
                writes[i].dstSet = vkdescriptorset;

            vkUpdateDescriptorSets(device, numdescriptors, writes, 0, nullptr);
        }

        VulkanDescriptorSet* descriptorset = Memory::Allocate<VulkanDescriptorSet>();
        descriptorset->descriptorPool = vkPool;
        descriptorset->descriptorSet  = vkdescriptorset;
        descriptorset->pipelineLayout = vkShader->pipelineLayout;
        descriptorset->poolKey        = key;

        return descriptorset;
    }

    void VulkanAPI::DestroyDescriptorSet(DescriptorSet* descriptorset)
    {
        if (descriptorset)
        {
            VulkanDescriptorSet* vkdescriptorset = (VulkanDescriptorSet*)descriptorset;
            vkFreeDescriptorSets(device, vkdescriptorset->descriptorPool, 1, &vkdescriptorset->descriptorSet);

            // 同一キーのデスクリプタプールの参照カウントを減らす(参照カウントが0ならデスクリプタプールを破棄)
            _DecrementPoolRefCount(vkdescriptorset->descriptorPool, vkdescriptorset->poolKey);

            Memory::Deallocate(vkdescriptorset);
        }
    }

    //==================================================================================
    // パイプライン
    //==================================================================================
    Pipeline* VulkanAPI::CreateGraphicsPipeline(ShaderHandle* shader, InputLayout* inputLayout, PipelineInputAssemblyState inputAssemblyState, PipelineRasterizationState rasterizationState, PipelineMultisampleState multisampleState, PipelineDepthStencilState depthstencilState, PipelineColorBlendState blendState, RenderPass* renderpass, uint32 renderSubpass, PipelineDynamicStateFlags dynamicState)
    {
        // ===== 頂点レイアウト =====
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        if (inputLayout)
        {
            VulkanInputLayout* vkformat = (VulkanInputLayout*)inputLayout;
            vertexInputStateCreateInfo = vkformat->createInfo;
        }

        // ===== インプットアセンブリ =====
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
        inputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.topology               = (VkPrimitiveTopology)inputAssemblyState.topology;
        inputAssemblyCreateInfo.primitiveRestartEnable = (inputAssemblyState.topology == PRIMITIVE_TOPOLOGY_TRIANGLE_STRIPS_WITH_RESTART_INDEX);

        // ===== テッセレーション =====
        VkPipelineTessellationStateCreateInfo tessellationCreateInfo = {};
        tessellationCreateInfo.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tessellationCreateInfo.patchControlPoints = rasterizationState.patchControlPoints;

        // ===== ビューポート =====
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
        viewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.scissorCount  = 1;

        // ===== ラスタライゼーション =====
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
        rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.depthClampEnable        = rasterizationState.enable_depth_clamp;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = rasterizationState.discard_primitives;
        rasterizationStateCreateInfo.polygonMode             = rasterizationState.wireframe? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.cullMode                = (PolygonCullMode)rasterizationState.cullMode;
        rasterizationStateCreateInfo.frontFace               = (rasterizationState.frontFace == POLYGON_FRONT_FACE_CLOCKWISE ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE);
        rasterizationStateCreateInfo.depthBiasEnable         = rasterizationState.depthBiasEnabled;
        rasterizationStateCreateInfo.depthBiasConstantFactor = rasterizationState.depthBiasConstantFactor;
        rasterizationStateCreateInfo.depthBiasClamp          = rasterizationState.depthBiasClamp;
        rasterizationStateCreateInfo.depthBiasSlopeFactor    = rasterizationState.depthBiasSlopeFactor;
        rasterizationStateCreateInfo.lineWidth               = rasterizationState.lineWidth;

        // ===== マルチサンプリング =====
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
        multisampleStateCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.rasterizationSamples  = _CheckSupportedSampleCounts(multisampleState.sampleCount);
        multisampleStateCreateInfo.sampleShadingEnable   = multisampleState.enableSampleShading;
        multisampleStateCreateInfo.minSampleShading      = multisampleState.minSampleShading;
        multisampleStateCreateInfo.pSampleMask           = multisampleState.sampleMask.empty()? nullptr : multisampleState.sampleMask.data();
        multisampleStateCreateInfo.alphaToCoverageEnable = multisampleState.enableAlphaToCoverage;
        multisampleStateCreateInfo.alphaToOneEnable      = multisampleState.enableAlphaToOne;

        // ===== デプス・ステンシル =====
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
        depthStencilStateCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.depthTestEnable       = depthstencilState.enableDepthTest;
        depthStencilStateCreateInfo.depthWriteEnable      = depthstencilState.enableDepthWrite;
        depthStencilStateCreateInfo.depthCompareOp        = (VkCompareOp)depthstencilState.depthCompareOp;
        depthStencilStateCreateInfo.depthBoundsTestEnable = depthstencilState.enableDepthRange;
        depthStencilStateCreateInfo.stencilTestEnable     = depthstencilState.enableStencil;
        depthStencilStateCreateInfo.front.failOp          = (VkStencilOp)depthstencilState.frontOp.fail;
        depthStencilStateCreateInfo.front.passOp          = (VkStencilOp)depthstencilState.frontOp.pass;
        depthStencilStateCreateInfo.front.depthFailOp     = (VkStencilOp)depthstencilState.frontOp.depthFail;
        depthStencilStateCreateInfo.front.compareOp       = (VkCompareOp)depthstencilState.frontOp.compare;
        depthStencilStateCreateInfo.front.compareMask     = depthstencilState.frontOp.compareMask;
        depthStencilStateCreateInfo.front.writeMask       = depthstencilState.frontOp.writeMask;
        depthStencilStateCreateInfo.front.reference       = depthstencilState.frontOp.reference;
        depthStencilStateCreateInfo.back.failOp           = (VkStencilOp)depthstencilState.backOp.fail;
        depthStencilStateCreateInfo.back.passOp           = (VkStencilOp)depthstencilState.backOp.pass;
        depthStencilStateCreateInfo.back.depthFailOp      = (VkStencilOp)depthstencilState.backOp.depthFail;
        depthStencilStateCreateInfo.back.compareOp        = (VkCompareOp)depthstencilState.backOp.compare;
        depthStencilStateCreateInfo.back.compareMask      = depthstencilState.backOp.compareMask;
        depthStencilStateCreateInfo.back.writeMask        = depthstencilState.backOp.writeMask;
        depthStencilStateCreateInfo.back.reference        = depthstencilState.backOp.reference;
        depthStencilStateCreateInfo.minDepthBounds        = depthstencilState.depthRangeMin;
        depthStencilStateCreateInfo.maxDepthBounds        = depthstencilState.depthRangeMax;

        // ===== ブレンドステート =====
        uint32 numColorAttachments = blendState.attachments.size();

        VkPipelineColorBlendAttachmentState* attachmentStates = SL_STACK(VkPipelineColorBlendAttachmentState, numColorAttachments);
        for (uint32 i = 0; i < numColorAttachments; i++)
        {
            attachmentStates[i] = {};
            attachmentStates[i].blendEnable         = blendState.attachments[i].enableBlend;
            attachmentStates[i].srcColorBlendFactor = (VkBlendFactor)blendState.attachments[i].srcColorBlendFactor;
            attachmentStates[i].dstColorBlendFactor = (VkBlendFactor)blendState.attachments[i].dstColorBlendFactor;
            attachmentStates[i].colorBlendOp        = (VkBlendOp)blendState.attachments[i].colorBlendOp;
            attachmentStates[i].srcAlphaBlendFactor = (VkBlendFactor)blendState.attachments[i].srcAlphaBlendFactor;
            attachmentStates[i].dstAlphaBlendFactor = (VkBlendFactor)blendState.attachments[i].dstAlphaBlendFactor;
            attachmentStates[i].alphaBlendOp        = (VkBlendOp)blendState.attachments[i].alphaBlendOp;

            if (blendState.attachments[i].write_r) attachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
            if (blendState.attachments[i].write_g) attachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
            if (blendState.attachments[i].write_b) attachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
            if (blendState.attachments[i].write_a) attachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
        }

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
        colorBlendStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable     = blendState.enableLogicOp;
        colorBlendStateCreateInfo.logicOp           = (VkLogicOp)blendState.logicOp;
        colorBlendStateCreateInfo.attachmentCount   = numColorAttachments;
        colorBlendStateCreateInfo.pAttachments      = attachmentStates;
        colorBlendStateCreateInfo.blendConstants[0] = blendState.blendConstant.r;
        colorBlendStateCreateInfo.blendConstants[1] = blendState.blendConstant.g;
        colorBlendStateCreateInfo.blendConstants[2] = blendState.blendConstant.b;
        colorBlendStateCreateInfo.blendConstants[3] = blendState.blendConstant.a;


        // ===== ダイナミックステート =====
        VkDynamicState* dynamicStates = SL_STACK(VkDynamicState, DYNAMIC_STATE_MAX + 2);
        uint32 dynamicStateCount = 0;

        dynamicStates[dynamicStateCount] = VK_DYNAMIC_STATE_VIEWPORT; dynamicStateCount++;
        dynamicStates[dynamicStateCount] = VK_DYNAMIC_STATE_SCISSOR;  dynamicStateCount++;

        if (dynamicState & DYNAMIC_STATE_LINE_WIDTH)
        {
            dynamicStates[dynamicStateCount] = VK_DYNAMIC_STATE_LINE_WIDTH;
            dynamicStateCount++;
        }
        if (dynamicState & DYNAMIC_STATE_DEPTH_BIAS)
        {
            dynamicStates[dynamicStateCount] = VK_DYNAMIC_STATE_DEPTH_BIAS;
            dynamicStateCount++;
        }
        if (dynamicState & DYNAMIC_STATE_BLEND_CONSTANTS)
        {
            dynamicStates[dynamicStateCount] = VK_DYNAMIC_STATE_BLEND_CONSTANTS;
            dynamicStateCount++;
        }
        if (dynamicState & DYNAMIC_STATE_DEPTH_BOUNDS)
        {
            dynamicStates[dynamicStateCount] = VK_DYNAMIC_STATE_DEPTH_BOUNDS;
            dynamicStateCount++;
        }
        if (dynamicState & DYNAMIC_STATE_STENCIL_COMPARE_MASK)
        {
            dynamicStates[dynamicStateCount] = VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
            dynamicStateCount++;
        }
        if (dynamicState & DYNAMIC_STATE_STENCIL_WRITE_MASK)
        {
            dynamicStates[dynamicStateCount] = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
            dynamicStateCount++;
        }
        if (dynamicState & DYNAMIC_STATE_STENCIL_REFERENCE)
        {
            dynamicStates[dynamicStateCount] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
            dynamicStateCount++;
        }

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
        dynamicStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
        dynamicStateCreateInfo.pDynamicStates    = dynamicStates;


        // ===== シェーダーステージ =====
        const VulkanShader* vkshader = (VulkanShader*)shader;
        VkPipelineShaderStageCreateInfo* pipelineStages = SL_STACK(VkPipelineShaderStageCreateInfo, vkshader->stageCreateInfos.size());
        for (uint32 i = 0; i < vkshader->stageCreateInfos.size(); i++)
        {
            pipelineStages[i] = vkshader->stageCreateInfos[i];
        }


        // ===== パイプライン生成 =====
        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext               = nullptr;
        pipelineCreateInfo.stageCount          = vkshader->stageCreateInfos.size();
        pipelineCreateInfo.pStages             = pipelineStages;
        pipelineCreateInfo.pVertexInputState   = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        pipelineCreateInfo.pTessellationState  = &tessellationCreateInfo;
        pipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState   = &multisampleStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState  = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState    = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDynamicState       = &dynamicStateCreateInfo;
        pipelineCreateInfo.layout              = vkshader->pipelineLayout;
        pipelineCreateInfo.renderPass          = (VkRenderPass)((VulkanRenderPass*)(renderpass))->renderpass;
        pipelineCreateInfo.subpass             = renderSubpass;

        VkPipeline vkpipeline = nullptr;
        VkResult result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCreateInfo, nullptr, &vkpipeline);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanPipeline* pipeline = Memory::Allocate<VulkanPipeline>();
        pipeline->pipeline = vkpipeline;

        return pipeline;
    }

    Pipeline* VulkanAPI::CreateComputePipeline(ShaderHandle* shader)
    {
        VulkanShader* vkshader = (VulkanShader*)shader;

        VkComputePipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext  = nullptr;
        pipelineCreateInfo.stage  = vkshader->stageCreateInfos[0];
        pipelineCreateInfo.layout = vkshader->pipelineLayout;

        VkPipeline vkpipeline = nullptr;
        VkResult result = vkCreateComputePipelines(device, nullptr, 1, &pipelineCreateInfo, nullptr, &vkpipeline);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanPipeline* pipeline = Memory::Allocate<VulkanPipeline>();
        pipeline->pipeline = vkpipeline;

        return pipeline;
    }

    void VulkanAPI::DestroyPipeline(Pipeline* pipeline)
    {
        if (pipeline)
        {
            VulkanPipeline* vkpipeline = (VulkanPipeline*)pipeline;
            vkDestroyPipeline(device, vkpipeline->pipeline, nullptr);

            Memory::Deallocate(vkpipeline);
        }
    }
}
