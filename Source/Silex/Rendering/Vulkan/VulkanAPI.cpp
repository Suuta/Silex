
#include "PCH.h"
#include "Rendering/Vulkan/VulkanAPI.h"
#include "Rendering/Vulkan/VulkanContext.h"


namespace Silex
{
    //=============================================
    // Vulkan 構造体
    //=============================================

    // コマンドキュー
    struct VulkanCommandQueue : public CommandQueue
    {
        VkQueue queue  = nullptr;
        uint32  family = INVALID_RENDER_ID;
        uint32  index  = INVALID_RENDER_ID;
    };

    // コマンドプール
    struct VulkanCommandPool : public CommandQueue
    {
        VkCommandPool     commandPool = nullptr;
        CommandBufferType type        = COMMAND_BUFFER_TYPE_PRIMARY;
    };

    // コマンドバッファ
    struct VulkanCommandBuffer : public CommandBuffer
    {
        VkCommandBuffer commandBuffer = nullptr;
    };

    // セマフォ
    struct VulkanSemaphore : public Semaphore
    {
        VkSemaphore semaphore = nullptr;
    };

    // フェンス
    struct VulkanFence : public Fence
    {
        VkFence fence = nullptr;
    };

    // レンダーパス
    struct VulkanRenderPass : public RenderPass
    {
        VkRenderPass renderpass = nullptr;
    };

    // フレームバッファ
    struct VulkanFramebuffer : public FramebufferHandle
    {
        VkFramebuffer framebuffer = nullptr;
    };

    // スワップチェイン
    struct VulkanSwapChain : public SwapChain
    {
        VulkanSurface*    surface    = nullptr;
        VulkanRenderPass* renderpass = nullptr;

        VkSwapchainKHR  swapchain  = nullptr;
        VkFormat        format     = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        std::vector<FramebufferHandle*> framebuffers;
        std::vector<VkImage>            images;
        std::vector<VkImageView>        views;

        uint32 imageIndex = 0;
    };

    // バッファ
    struct VulkanBuffer : public Buffer
    {
        VkBuffer      buffer           = nullptr;
        VkBufferView  view             = nullptr;
        uint64        size             = 0;
        VmaAllocation allocationHandle = nullptr;
        uint64        allocationSize   = 0;
    };

    // テクスチャ
    struct VulkanTexture : public TextureHandle
    {
        VkImage     image     = nullptr;
        VkImageView imageView = nullptr;
        VkExtent3D  extent    = {};

        VmaAllocation     allocationHandle = nullptr;
        VmaAllocationInfo allocationInfo   = {};
    };

    // サンプラー
    struct VulkanSampler : public Sampler
    {
        VkSampler sampler = nullptr;
    };

    // 頂点フォーマット
    struct VulkanVertexFormat : public VertexFormat
    {
        std::vector<VkVertexInputBindingDescription>   bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
        VkPipelineVertexInputStateCreateInfo           createInfo = {};
    };

    // シェーダー
    struct VulkanShader : public DescriptorSet
    {
        VkShaderStageFlags                           stageFlags = 0;
        std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
        std::vector<VkDescriptorSetLayout>           descriptorsetLayouts;
        VkPipelineLayout                             pipelineLayout;
    };

    // デスクリプターセット
    static constexpr uint32 MaxDescriptorsetPerPool = 64;

    struct VulkanDescriptorSet : public DescriptorSet
    {
        VkDescriptorSet  descriptorSet  = nullptr;
        VkDescriptorPool descriptorPool = nullptr;

        VulkanAPI::DescriptorSetPoolKey poolKey;
    };

    // パイプライン
    struct VulkanPipeline
    {
        VkPipeline pipeline = nullptr;
    };



    //==================================================================================
    // Vulkan ヘルパー
    //==================================================================================

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
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_SAMPLER_WITH_TEXTURE])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_SAMPLER_WITH_TEXTURE] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_TEXTURE])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_TEXTURE] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_IMAGE])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_IMAGE] * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_TEXTURE_BUFFER] || key.descriptorTypeCounts[DESCRIPTOR_TYPE_SAMPLER_WITH_TEXTURE_BUFFER])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                sizes->descriptorCount = (key.descriptorTypeCounts[DESCRIPTOR_TYPE_TEXTURE_BUFFER] + key.descriptorTypeCounts[DESCRIPTOR_TYPE_SAMPLER_WITH_TEXTURE_BUFFER]) * MaxDescriptorsetPerPool;
                sizes++;
                sizeCount++;
            }
            if (key.descriptorTypeCounts[DESCRIPTOR_TYPE_IMAGE_BUFFER])
            {
                *sizes = {};
                sizes->type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                sizes->descriptorCount = key.descriptorTypeCounts[DESCRIPTOR_TYPE_IMAGE_BUFFER] * MaxDescriptorsetPerPool;
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
    void VulkanAPI::_RecycleDescriptorPool(VkDescriptorPool pool, DescriptorSetPoolKey& poolKey)
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

        // NOTE:===================================================================
        // 以前の実装では、インスタンスの検証レイヤーとデバイス固有の検証レイヤーが区別されていたが
        // 最新の実装では enabledLayerCount / ppEnabledLayerNames フィールドは無視される
        // ただし、古い実装との互換性を保つ必要があれば、設定する必要がある
        // ========================================================================
        //deviceCreateInfo.ppEnabledLayerNames = nullptr;
        //deviceCreateInfo.enabledLayerCount   = 0;

        VkResult result = vkCreateDevice(context->GetPhysicalDevice(), &deviceCreateInfo, nullptr, &device);
        SL_CHECK_VKRESULT(result, false);

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

    //==================================================================================
    // コマンドプール
    //==================================================================================
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
    SwapChain* VulkanAPI::CreateSwapChain(Surface* surface)
    {
        VkResult result;

        VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();
        VkSurfaceKHR     vkSurface      = ((VulkanSurface*)surface)->surface;

        uint32 formatCount = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface, &formatCount, nullptr);
        SL_CHECK_VKRESULT(result, nullptr);

        VkSurfaceFormatKHR* formats = SL_STACK(VkSurfaceFormatKHR, formatCount);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface, &formatCount, formats);
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

        VkRenderPass vkRenderPass = nullptr;
        result = vkCreateRenderPass2(device, &passInfo, nullptr, &vkRenderPass);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanRenderPass* renderpass = Memory::Allocate<VulkanRenderPass>();
        renderpass->renderpass = vkRenderPass;

        VulkanSwapChain* swapchain = Memory::Allocate<VulkanSwapChain>();
        swapchain->surface    = ((VulkanSurface*)surface);
        swapchain->format     = format;
        swapchain->colorspace = colorspace;
        swapchain->renderpass = renderpass;

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
            surface->width  = extent.width;
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

        VkPresentModeKHR* presentModes = SL_STACK(VkPresentModeKHR, presentModeCount);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface->surface, &presentModeCount, presentModes);
        SL_CHECK_VKRESULT(result, false);

        VkPresentModeKHR selectMode = (VkPresentModeKHR)mode;
        bool findRequestMode = false;
        for (uint32 i = 0; i < presentModeCount; i++)
        {
            if (selectMode == presentModes[i])
            {
                findRequestMode = true;
                surface->vsyncMode = (VSyncMode)selectMode;
                break;
            }
        }

        // 見つからない場合は、必ずサポートされている（VK_PRESENT_MODE_FIFO_KHR）モードを選択
        if (!findRequestMode)
        {
            surface->vsyncMode = VSYNC_MODE_ENABLED;
            selectMode         = VK_PRESENT_MODE_FIFO_KHR;

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
        framebufferCreateInfo.width           = surface->width;
        framebufferCreateInfo.height          = surface->height;
        framebufferCreateInfo.layers          = 1;

        for (uint32 i = 0; i < imageCount; i++)
        {
            framebufferCreateInfo.pAttachments = &vkSwapchain->views[i];

            VkFramebuffer framebuffer = nullptr;
            result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer);
            SL_CHECK_VKRESULT(result, false);

            VulkanFramebuffer* vkFramebuffer = Memory::Allocate<VulkanFramebuffer>();
            vkFramebuffer->framebuffer = framebuffer;

            vkSwapchain->framebuffers.push_back(vkFramebuffer);
        }

        return true;
    }

    FramebufferHandle* VulkanAPI::GetSwapChainNextFramebuffer(SwapChain* swapchain, Semaphore* semaphore)
    {
        VulkanSwapChain* vkswapchain = (VulkanSwapChain*)swapchain;
        VulkanSemaphore* vksemaphore = (VulkanSemaphore*)semaphore;

        //=============================================================================
        // TODO: サーフェースのリサイズ対応
        // リサイズ対応が必要なら、ここでリサイズフラグを立てて、スワップチェインにリサイズ要求をする
        // ここでチェックするか、ウィンドウイベントから直接リサイズ呼び出しするか検討する
        //=============================================================================

        VkResult result = AcquireNextImageKHR(device, vkswapchain->swapchain, UINT64_MAX, vksemaphore->semaphore, nullptr, &vkswapchain->imageIndex);
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


    //==================================================================================
    // バッファ
    //==================================================================================
    Buffer* VulkanAPI::CreateBuffer(uint64 size, BufferUsageBits usage, MemoryAllocationType memoryType)
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
                // 書き込み場合は、正しい順序で読み込まれることを保証する
                allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }

            if (!isSrc && isDst)
            {
                // 読み戻しの場合は、ランダムに読み込まれることを許す
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
    Sampler* VulkanAPI::CreateSampler(const SamplerState& state)
    {
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext                   = nullptr;
        createInfo.flags                   = 0;
        createInfo.magFilter               = (VkFilter)state.magFilter;
        createInfo.minFilter               = (VkFilter)state.minFilter;
        createInfo.mipmapMode              = (VkSamplerMipmapMode)state.mipFilter;
        createInfo.addressModeU            = (VkSamplerAddressMode)state.repeatU;
        createInfo.addressModeV            = (VkSamplerAddressMode)state.repeatV;
        createInfo.addressModeW            = (VkSamplerAddressMode)state.repeatW;
        createInfo.mipLodBias              = state.lodBias;
        createInfo.anisotropyEnable        = state.useAnisotropy;
        createInfo.maxAnisotropy           = state.anisotropyMax;
        createInfo.compareEnable           = state.enableCompare;
        createInfo.compareOp               = (VkCompareOp)state.compareOp;
        createInfo.minLod                  = state.minLod;
        createInfo.maxLod                  = state.maxLod;
        createInfo.borderColor             = (VkBorderColor)state.borderColor;
        createInfo.unnormalizedCoordinates = state.unnormalized;

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
    FramebufferHandle* VulkanAPI::CreateFramebuffer(RenderPass* renderpass, TextureHandle* textures, uint32 numTexture, uint32 width, uint32 height)
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
    VertexFormat* VulkanAPI::CreateVertexFormat(uint32 numattributes, VertexAttribute* attributes)
    {
        VulkanVertexFormat* vkVertexFormat = Memory::Allocate<VulkanVertexFormat>();
        vkVertexFormat->attributes.resize(numattributes);
        vkVertexFormat->bindings.resize(numattributes);

        for (uint32 i = 0; i < numattributes; i++)
        {
            vkVertexFormat->bindings[i]           = {};
            vkVertexFormat->bindings[i].binding   = i;
            vkVertexFormat->bindings[i].stride    = attributes[i].stride;
            vkVertexFormat->bindings[i].inputRate = attributes[i].frequency == VERTEX_FREQUENCY_INSTANCE? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
            
            vkVertexFormat->attributes[i]          = {};
            vkVertexFormat->attributes[i].binding  = i;
            vkVertexFormat->attributes[i].location = attributes[i].location;
            vkVertexFormat->attributes[i].format   = (VkFormat)attributes[i].format;
            vkVertexFormat->attributes[i].offset   = attributes[i].offset;
        }

        vkVertexFormat->createInfo = {};
        vkVertexFormat->createInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vkVertexFormat->createInfo.vertexBindingDescriptionCount   = numattributes;
        vkVertexFormat->createInfo.pVertexBindingDescriptions      = vkVertexFormat->bindings.data();
        vkVertexFormat->createInfo.vertexAttributeDescriptionCount = numattributes;
        vkVertexFormat->createInfo.pVertexAttributeDescriptions    = vkVertexFormat->attributes.data();

        return vkVertexFormat;
    }

    void VulkanAPI::DestroyVertexFormat(VertexAttribute* attributes)
    {
        if (attributes)
        {
            VulkanVertexFormat* vkVertexFormat = (VulkanVertexFormat*)attributes;
            Memory::Deallocate(vkVertexFormat);
        }
    }

    //==================================================================================
    // レンダーパス
    //==================================================================================
    RenderPass* VulkanAPI::CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies)
    {
        // アタッチメント
        VkAttachmentDescription2* vkAttachments = SL_STACK(VkAttachmentDescription2, numAttachments);
        for (uint32 i = 0; i < numAttachments; i++)
        {
            vkAttachments[i] = {};
            vkAttachments[i].sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR;
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
        VkSubpassDescription2* vkSubpasses = SL_STACK(VkSubpassDescription2, numSubpasses);
        for (uint32 i = 0; i < numSubpasses; i++)
        {
            // 入力アタッチメント参照
            uint32 numInputAttachmentRef = subpasses[i].inputReferences.size();
            VkAttachmentReference2* inputAttachmentRefs = SL_STACK(VkAttachmentReference2, numInputAttachmentRef);
            for (uint32 j = 0; j < numInputAttachmentRef; j++)
            {
                *inputAttachmentRefs = {};
                inputAttachmentRefs[i].sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                inputAttachmentRefs[i].attachment = subpasses[i].inputReferences[j].attachment;
                inputAttachmentRefs[i].layout     = (VkImageLayout)subpasses[i].inputReferences[j].layout;
                inputAttachmentRefs[i].aspectMask = (VkImageAspectFlags)subpasses[i].inputReferences[j].aspect;
            }

            // カラーアタッチメント参照
            uint32 numColorAttachmentRef = subpasses[i].colorReferences.size();
            VkAttachmentReference2* colorAttachmentRefs = SL_STACK(VkAttachmentReference2, numColorAttachmentRef);
            for (uint32 j = 0; j < numColorAttachmentRef; j++)
            {
                *colorAttachmentRefs = {};
                colorAttachmentRefs[i].sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                colorAttachmentRefs[i].attachment = subpasses[i].colorReferences[j].attachment;
                colorAttachmentRefs[i].layout     = (VkImageLayout)subpasses[i].colorReferences[j].layout;
                colorAttachmentRefs[i].aspectMask = (VkImageAspectFlags)subpasses[i].colorReferences[j].aspect;
            }

            // マルチサンプル解決アタッチメント参照
            VkAttachmentReference2* resolveAttachmentRef = nullptr;
            if (subpasses[i].resolveReferences.attachment != INVALID_RENDER_ID)
            {
                resolveAttachmentRef = SL_STACK(VkAttachmentReference2, 1);

                *resolveAttachmentRef = {};
                resolveAttachmentRef->sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                resolveAttachmentRef->attachment = subpasses[i].resolveReferences.attachment;
                resolveAttachmentRef->layout     = (VkImageLayout)subpasses[i].resolveReferences.layout;
                resolveAttachmentRef->aspectMask = (VkImageAspectFlags)subpasses[i].resolveReferences.aspect;
            }

            // 深度ステンシルアタッチメント参照
            VkAttachmentReference2* depthstencilAttachmentRef = nullptr;
            if (subpasses[i].depthstencilReference.attachment != INVALID_RENDER_ID)
            {
                depthstencilAttachmentRef = SL_STACK(VkAttachmentReference2, 1);

                *depthstencilAttachmentRef = {};
                depthstencilAttachmentRef->sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                depthstencilAttachmentRef->attachment = subpasses[i].depthstencilReference.attachment;
                depthstencilAttachmentRef->layout     = (VkImageLayout)subpasses[i].depthstencilReference.layout;
                depthstencilAttachmentRef->aspectMask = (VkImageAspectFlags)subpasses[i].depthstencilReference.aspect;
            }

            vkSubpasses[i] = {};
            vkSubpasses[i].sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR;
            vkSubpasses[i].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            vkSubpasses[i].viewMask                = 0;
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
        VkSubpassDependency2* vkSubpassDependencies = SL_STACK(VkSubpassDependency2, numSubpassDependencies);
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
        createInfo.attachmentCount         = numAttachments;
        createInfo.pAttachments            = vkAttachments;
        createInfo.subpassCount            = numSubpasses;
        createInfo.pSubpasses              = vkSubpasses;
        createInfo.dependencyCount         = numSubpassDependencies;
        createInfo.pDependencies           = vkSubpassDependencies;
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

            Memory::Deallocate(vkRenderpass);
        }
    }

    //==================================================================================
    // コマンド
    //==================================================================================
    void VulkanAPI::PipelineBarrier(CommandBuffer* commanddBuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrier* memoryBarrier, uint32 numBufferBarrier, BufferBarrier* bufferBarrier, uint32 numTextureBarrier, TextureBarrier* textureBarrier)
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
        vkCmdPushConstants(cmd->commandBuffer, vkshader->pipelineLayout, vkshader->stageFlags, firstIndex * sizeof(uint32), numData * sizeof(uint32_t), data);
    }

    void VulkanAPI::BeginRenderPass(CommandBuffer* commandbuffer, RenderPass* renderpass, FramebufferHandle* framebuffer, CommandBufferType commandBufferType, uint32 numclearValues, RenderPassClearValue* clearvalues, uint32 x, uint32 y, uint32 width, uint32 height)
    {
        VkSubpassContents vksubpassContents = commandBufferType == COMMAND_BUFFER_TYPE_PRIMARY ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

        VkRenderPassBeginInfo begineInfo = {};
        begineInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begineInfo.renderPass               = (VkRenderPass)((VulkanRenderPass*)(renderpass))->renderpass;
        begineInfo.framebuffer              = (VkFramebuffer)((VulkanFramebuffer*)(framebuffer))->framebuffer;
        begineInfo.renderArea.offset.x      = x;
        begineInfo.renderArea.offset.y      = y;
        begineInfo.renderArea.extent.width  = width;
        begineInfo.renderArea.extent.height = height;
        begineInfo.clearValueCount          = numclearValues;
        begineInfo.pClearValues             = (VkClearValue*)clearvalues;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdBeginRenderPass(cmd->commandBuffer, &begineInfo, vksubpassContents);
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
        VkViewport viewport = {};
        viewport.x        = x;
        viewport.y        = y;
        viewport.width    = width;
        viewport.height   = height;
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

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

    void VulkanAPI::BindDescriptorSet(CommandBuffer* commandbuffer, DescriptorSet* descriptorset, ShaderHandle* shader, uint32 setIndex)
    {
        VulkanShader* vkshader = (VulkanShader*)shader;
        VulkanDescriptorSet* vkdescriptorset = (VulkanDescriptorSet*)descriptorset;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdBindDescriptorSets(cmd->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkshader->pipelineLayout, setIndex, 1, &vkdescriptorset->descriptorSet, 0, nullptr);
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

    void VulkanAPI::BindVertexBuffers(CommandBuffer* commandbuffer, uint32 bindingCount, const Buffer** buffers, const uint64* offsets)
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

    void VulkanAPI::SetLineWidth(CommandBuffer* commandbuffer, float width)
    {
        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdSetLineWidth(cmd->commandBuffer, width);
    }


    //==================================================================================
    // シェーダー
    //==================================================================================
#if 1
    std::vector<byte> VulkanAPI::CompileSPIRV(uint32 numSpirv, ShaderStageSPIRVData* spirv, const std::string& shaderName)
    {
        return std::vector<byte>();
    }

    ShaderHandle* VulkanAPI::CreateShader(const std::vector<byte>& p_shader_binary, ShaderDescription& shaderDesc, std::string& name)
    {
        return nullptr;
    }

    void VulkanAPI::DestroyShader(ShaderHandle* shader)
    {
    }
#endif

    //==================================================================================
    // デスクリプター
    //==================================================================================
    DescriptorSet* VulkanAPI::CreateDescriptorSet(Descriptor* descriptors, uint32 numdescriptors, ShaderHandle* shader, uint32 setIndex)
    {
        DescriptorSetPoolKey key = {};

        // key 追加
        VkWriteDescriptorSet* writes = SL_STACK(VkWriteDescriptorSet, numdescriptors);
        for (uint32 i = 0; i < numdescriptors; i++)
        {
            uint32 numDescriptors = 1;
            const Descriptor& descriptor = descriptors[i];

            writes[i] = {};
            writes[i].sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstBinding = descriptor.binding;

            switch (descriptor.type)
            {
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
                case DESCRIPTOR_TYPE_SAMPLER_WITH_TEXTURE:
                {
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
                case DESCRIPTOR_TYPE_TEXTURE:
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
                case DESCRIPTOR_TYPE_IMAGE:
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
                case DESCRIPTOR_TYPE_TEXTURE_BUFFER:
                {
                    numDescriptors = descriptor.handles.size();
                    VkDescriptorBufferInfo* bufferInfo = SL_STACK(VkDescriptorBufferInfo, numDescriptors);
                    VkBufferView* views                = SL_STACK(VkBufferView, numDescriptors);

                    for (uint32 j = 0; j < numDescriptors; j++)
                    {
                        VulkanBuffer* buffer = (VulkanBuffer*)descriptor.handles[j];
                        bufferInfo[j] = {};
                        bufferInfo[j].buffer = buffer->buffer;
                        bufferInfo[j].range  = buffer->size;

                        views[j] = buffer->view;
                    }

                    writes[i].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                    writes[i].pBufferInfo      = bufferInfo;
                    writes[i].pTexelBufferView = views;

                    break;
                }
                case DESCRIPTOR_TYPE_SAMPLER_WITH_TEXTURE_BUFFER:
                {
                    numDescriptors = descriptor.handles.size() / 2;
                    VkDescriptorImageInfo*  imageInfos  = SL_STACK(VkDescriptorImageInfo, numDescriptors);
                    VkDescriptorBufferInfo* bufferInfos = SL_STACK(VkDescriptorBufferInfo, numDescriptors);
                    VkBufferView*           views       = SL_STACK(VkBufferView, numDescriptors);

                    for (uint32 j = 0; j < numDescriptors; j++)
                    {
                        VulkanSampler* sampler = (VulkanSampler*)descriptor.handles[j * 2 + 0];
                        VulkanBuffer* buffer   = (VulkanBuffer*)descriptor.handles[j * 2 + 1];

                        imageInfos[j] = {};
                        imageInfos[j].sampler = sampler->sampler;

                        bufferInfos[j] = {};
                        bufferInfos[j].buffer = buffer->buffer;
                        bufferInfos[j].range  = buffer->size;

                        views[j] = buffer->view;
                    }

                    writes[i].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                    writes[i].pImageInfo       = imageInfos;
                    writes[i].pBufferInfo      = bufferInfos;
                    writes[i].pTexelBufferView = views;

                    break;
                }
                case DESCRIPTOR_TYPE_IMAGE_BUFFER:
                {
                    SL_ASSERT(false, "未実装");
                    break;
                }
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
                case DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                {
                    numDescriptors = descriptor.handles.size();
                    VkDescriptorImageInfo* imageInfos = SL_STACK(VkDescriptorImageInfo, numDescriptors);

                    for (uint32 j = 0; j < numDescriptors; j++)
                    {
                        imageInfos[j] = {};
                        imageInfos[j].imageView   = ((VulkanTexture*)descriptor.handles[j])->imageView;
                        imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }

                    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                    writes[i].pImageInfo     = imageInfos;

                    break;
                }

                default: SL_ASSERT(false);
            }

            writes[i].descriptorCount = numDescriptors;
            key.descriptorTypeCounts[descriptor.type] += numDescriptors;
        }

        // デスクリプタープール取得 (keyをもとに、生成済みのプールがあれば取得、なければ新規生成)
        VkDescriptorPool vkPool = _FindOrCreateDescriptorPool(key);
        SL_CHECK(vkPool, nullptr);

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool     = vkPool;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        descriptorSetAllocateInfo.pSetLayouts        = &((VulkanShader*)shader)->descriptorsetLayouts[setIndex];

        // デスクリプターセット生成
        VkDescriptorSet vkdescriptorset = nullptr;
        VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &vkdescriptorset);
        if (result != VK_SUCCESS)
        {
            _RecycleDescriptorPool(vkPool, key);
            
            SL_LOG_LOCATION_ERROR(VkResultToString(result));
            return nullptr;
        }

        // デスクリプターセット更新
        for (uint32 i = 0; i < numdescriptors; i++)
            writes[i].dstSet = vkdescriptorset;

        vkUpdateDescriptorSets(device, numdescriptors, writes, 0, nullptr);


        VulkanDescriptorSet* descriptorset = Memory::Allocate<VulkanDescriptorSet>();
        descriptorset->descriptorPool = vkPool;
        descriptorset->descriptorSet  = vkdescriptorset;
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
            _RecycleDescriptorPool(vkdescriptorset->descriptorPool, vkdescriptorset->poolKey);

            Memory::Deallocate(vkdescriptorset);
        }
    }

    //==================================================================================
    // パイプライン
    //==================================================================================
    Pipeline* VulkanAPI::CreatePipeline(ShaderHandle* shader, VertexFormat* vertexFormat, PrimitiveTopology primitive, PipelineRasterizationState rasterizationState, PipelineMultisampleState multisampleState, PipelineDepthStencilState depthstencilState, PipelineColorBlendState blendState, int32* colorAttachments, int32 numColorAttachments, PipelineDynamicStateFlags dynamicState, RenderPass* renderpass, uint32 renderSubpass)
    {
        return nullptr;
    }

    void VulkanAPI::DestroyPipeline(Pipeline* pipeline)
    {
    }
}
