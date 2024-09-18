
#include "PCH.h"

#include "Rendering/ShaderCompiler.h"
#include "Rendering/Vulkan/VulkanAPI.h"
#include "Rendering/Vulkan/VulkanContext.h"
#include "Rendering/RenderingUtility.h"
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

    // スワップチェイン生成 共通処理
    VulkanSwapChain* VulkanAPI::_CreateSwapChain_Internal(VulkanSwapChain* swapchain, const SwapChainCapability& cap)
    {
        // スワップチェイン生成
        VkSwapchainCreateInfoKHR swapCreateInfo = {};
        swapCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapCreateInfo.surface          = swapchain->surface->surface;
        swapCreateInfo.minImageCount    = cap.minImageCount;
        swapCreateInfo.imageFormat      = cap.format;
        swapCreateInfo.imageColorSpace  = cap.colorspace;
        swapCreateInfo.imageExtent      = cap.extent;
        swapCreateInfo.imageArrayLayers = 1;
        swapCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapCreateInfo.preTransform     = cap.transform;
        swapCreateInfo.compositeAlpha   = cap.compositeAlpha;
        swapCreateInfo.presentMode      = cap.presentMode;
        swapCreateInfo.clipped          = true;

        VkResult result = CreateSwapchainKHR(device, &swapCreateInfo, nullptr, &swapchain->swapchain);
        SL_CHECK_VKRESULT(result, nullptr);

        // イメージ取得
        uint32 imageCount = 0;
        result = GetSwapchainImagesKHR(device, swapchain->swapchain, &imageCount, nullptr);
        SL_CHECK_VKRESULT(result, nullptr);

        VkImage* vkimg = SL_STACK(VkImage, imageCount);
        result = GetSwapchainImagesKHR(device, swapchain->swapchain, &imageCount, vkimg);
        SL_CHECK_VKRESULT(result, nullptr);

        // イメージビュー生成
        VkImageView* vkview = SL_STACK(VkImageView, imageCount);
        for (uint32 i = 0; i < imageCount; i++)
        {
            VulkanTexture* vktex = slnew(VulkanTexture);
            vktex->createFlags      = 0;
            vktex->image            = vkimg[i];
            vktex->format           = swapCreateInfo.imageFormat;
            vktex->usageflags       = swapCreateInfo.imageUsage;
            vktex->extent           = VkExtent3D(swapCreateInfo.imageExtent.width, swapCreateInfo.imageExtent.height, 1);
            vktex->arrayLayers      = 1;
            vktex->mipLevels        = 1;
            vktex->allocationHandle = nullptr;

            swapchain->textures.push_back(vktex);

            TextureViewInfo viewinfo = {};
            viewinfo.type                      = TEXTURE_TYPE_2D;
            viewinfo.subresource.aspect        = TEXTURE_ASPECT_COLOR_BIT;
            viewinfo.subresource.baseLayer     = 0;
            viewinfo.subresource.layerCount    = 1;
            viewinfo.subresource.baseMipLevel  = 0;
            viewinfo.subresource.mipLevelCount = 1;

            TextureView* vkview = CreateTextureView(vktex, viewinfo);
            SL_CHECK(!vkview, nullptr);

            swapchain->views.push_back(vkview);
        }

        // フレームバッファ
        for (uint32 i = 0; i < imageCount; i++)
        {
            FramebufferHandle* fb = CreateFramebuffer(swapchain->renderpass, 1, &swapchain->textures[i], cap.extent.width, cap.extent.height);
            SL_CHECK(!fb, nullptr);
            
            swapchain->framebuffers.push_back(fb);
        }

        return swapchain;
    }

    // スワップチェイン破棄 共通処理
    void VulkanAPI::_DestroySwapChain_Internal(VulkanSwapChain* swapchain)
    {
        if (swapchain)
        {
            // フレームバッファ破棄
            for (uint32 i = 0; i < swapchain->framebuffers.size(); i++)
            {
                DestroyFramebuffer(swapchain->framebuffers[i]);
            }

            // イメージビュー破棄
            for (uint32 i = 0; i < swapchain->views.size(); i++)
            {
                DestroyTextureView(swapchain->views[i]);
            }

            // イメージ破棄
            for (uint32 i = 0; i < swapchain->textures.size(); i++)
            {
                //-------------------------------------------------
                // VkImage 自体は swapchain が管理しているので破棄しない
                //-------------------------------------------------
                VulkanTexture* vktex = (VulkanTexture*)swapchain->textures[i];
                sldelete(vktex);
            }

            // スワップチェイン破棄
            DestroySwapchainKHR(device, swapchain->swapchain, nullptr);

            swapchain->framebuffers.clear();
            swapchain->views.clear();
            swapchain->textures.clear();
        }
    }

    // スワップチェインパス生成
    VulkanRenderPass* VulkanAPI::_CreateSwapChainRenderPass(VkFormat format)
    {
        VkAttachmentDescription attachment = {};
        attachment.format         = format;
        attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // 描画 前 処理
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
        passInfo.dependencyCount = 1;
        passInfo.pDependencies   = &dependency;

        VkRenderPass vkRenderPass = nullptr;
        VkResult result = vkCreateRenderPass(device, &passInfo, nullptr, &vkRenderPass);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanRenderPass* renderpass = slnew(VulkanRenderPass);
        renderpass->renderpass = vkRenderPass;

        return renderpass;
    }

    // スワップチェイン仕様のクエリ
    bool VulkanAPI::_QuerySwapChainCapability(VkSurfaceKHR surface, uint32 width, uint32 height, uint32 requestFramebufferCount, VkPresentModeKHR mode, SwapChainCapability& out_cap)
    {
        VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();

        //================================================================================
        // フォーマット・色空間
        //================================================================================
        uint32 formatCount = 0;
        VkResult result = context->GetExtensionFunctions().GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        SL_CHECK_VKRESULT(result, false);

        VkSurfaceFormatKHR* formats = SL_STACK(VkSurfaceFormatKHR, formatCount);
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);
        SL_CHECK_VKRESULT(result, false);

        VkFormat        format     = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        // BGRA / RGBA https://www.reddit.com/r/vulkan/comments/p3iy0o/why_use_bgra_instead_of_rgba/
        if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        {
            // VK_FORMAT_UNDEFINED が1つだけ含まれている場合、サーフェスには優先フォーマットがない
            format = VK_FORMAT_B8G8R8A8_UNORM;
            colorspace = formats[0].colorSpace;
        }
        else
        {
            // BGRA8_UNORM がサポートされていればそれが推奨される
            const VkFormat firstChoice = VK_FORMAT_B8G8R8A8_UNORM;
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


        //================================================================================
        // サイズ
        //================================================================================
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
        SL_CHECK_VKRESULT(result, false);

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

        //================================================================================
        // フレームバッファ数決定（現状: 3）
        //================================================================================
        uint32 requestImageCount = std::clamp(requestFramebufferCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);

        //================================================================================
        // トランスフォーム情報（回転・反転）
        //================================================================================
        VkSurfaceTransformFlagBitsKHR surfaceTransformBits;
        if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            surfaceTransformBits = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            surfaceTransformBits = surfaceCapabilities.currentTransform;
        }

        //================================================================================
        // アルファモードが有効なら（現状: 使用しない）
        //================================================================================
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

        //================================================================================
        // プレゼントモード
        //================================================================================
        uint32 presentModeCount = 0;
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        SL_CHECK_VKRESULT(result, false);

        VkPresentModeKHR* presentModes = SL_STACK(VkPresentModeKHR, presentModeCount);
        result = context->GetExtensionFunctions().GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);
        SL_CHECK_VKRESULT(result, false);

        VkPresentModeKHR selectMode = mode;
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

        out_cap.format         = format;
        out_cap.colorspace     = colorspace;
        out_cap.compositeAlpha = compositeAlpha;
        out_cap.extent         = extent;
        out_cap.minImageCount  = requestImageCount;
        out_cap.presentMode    = selectMode;
        out_cap.transform      = surfaceTransformBits;

        return true;
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


        //==========================================================================
        // デバイス機能
        //--------------------------------------------------------------------------
        // 拡張機能がコア機能に昇格しているバージョン移行であれば、Feature を有効にするだけで良い。
        // pNextチェインを使って VkPhysicalDeviceVulkan "version" Features を渡すことで、
        // 1.1以降の機能を指定する
        // 
        // 逆にコア機能に昇格していないバージョンで拡張機能を使用する場合は、Extension で有効にする
        // 必要がある。また、VkPhysicalDevice "ExtensionName" Features を指定することで
        // 追加の拡張機能オプションが指定できる
        //==========================================================================

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
    CommandQueue* VulkanAPI::CreateCommandQueue(QueueID id, uint32 indexInFamily)
    {
        // queueIndex は キューファミリ内に複数キューが存在する場合のインデックスを指定する
        VkQueue vkQueue = nullptr;
        vkGetDeviceQueue(device, id, indexInFamily, &vkQueue);

        VulkanCommandQueue* queue = slnew(VulkanCommandQueue);
        queue->family = id;
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
            sldelete(vkqueue);
        }
    }

    QueueID VulkanAPI::QueryQueueID(QueueFamilyFlags queueFlag, Surface* surface) const
    {
        QueueID familyIndex = RENDER_INVALID_ID;

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

    bool VulkanAPI::SubmitQueue(CommandQueue* queue, CommandBuffer* commandbuffer, Fence* fence, Semaphore* present, Semaphore* render)
    {
        VulkanCommandQueue*  vkqueue          = (VulkanCommandQueue*)queue;
        VulkanCommandBuffer* vkcommandBuffer  = (VulkanCommandBuffer*)commandbuffer;
        VulkanFence*         vkfence          = (VulkanFence*)fence;

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo = {};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext                = nullptr;
        submitInfo.pWaitDstStageMask    = &waitStage;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &vkcommandBuffer->commandBuffer;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &((VulkanSemaphore*)present)->semaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &((VulkanSemaphore*)render)->semaphore;

        VkResult result = vkQueueSubmit(vkqueue->queue, 1, &submitInfo, vkfence->fence);
        SL_CHECK_VKRESULT(result, false);

        return false;
    }



    //==================================================================================
    // コマンドプール
    //==================================================================================
    CommandPool* VulkanAPI::CreateCommandPool(QueueID id, CommandBufferType type)
    {
        uint32 familyIndex = id;

        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; 
        commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // (必須) コマンドバッファがリセットできるようにする
        commandPoolInfo.queueFamilyIndex = familyIndex;

        VkCommandPool vkCommandPool = nullptr;
        VkResult result = vkCreateCommandPool(device, &commandPoolInfo, nullptr, &vkCommandPool);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanCommandPool* commandPool = slnew(VulkanCommandPool);
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

            sldelete(vkpool);
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

        VulkanCommandBuffer* cmdBuffer = slnew(VulkanCommandBuffer);
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
            sldelete(vkcommandBuffer);
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

        VulkanSemaphore* semaphore = slnew(VulkanSemaphore);
        semaphore->semaphore = vkSemaphore;

        return semaphore;
    }

    void VulkanAPI::DestroySemaphore(Semaphore* semaphore)
    {
        if (semaphore)
        {
            VulkanSemaphore* vkSemaphore = (VulkanSemaphore*)semaphore;
            vkDestroySemaphore(device, vkSemaphore->semaphore, nullptr);

            sldelete(vkSemaphore);
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
        createInfo.flags = 0;                                      //VK_FENCE_CREATE_SIGNALED_BIT;  // シグナル状態で生成
        createInfo.pNext = nullptr;

        VkResult result = vkCreateFence(device, &createInfo, nullptr, &vkfence);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanFence* fence = slnew(VulkanFence);
        fence->fence = vkfence;

        return fence;
    }

    void VulkanAPI::DestroyFence(Fence* fence)
    {
        if (fence)
        {
            VulkanFence* vkfence = (VulkanFence*)fence;
            vkDestroyFence(device, vkfence->fence, nullptr);

            sldelete(vkfence);
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

        SwapChainCapability queryResult;
        SL_CHECK(!_QuerySwapChainCapability(((VulkanSurface*)surface)->surface, width, height, requestFramebufferCount, (VkPresentModeKHR)mode, queryResult), nullptr);

        VulkanSwapChain* swapchain = slnew(VulkanSwapChain);
        swapchain->renderpass = _CreateSwapChainRenderPass(queryResult.format);
        swapchain->surface    = (VulkanSurface*)surface;
        swapchain->format     = queryResult.format;
        swapchain->colorspace = queryResult.colorspace;

        _CreateSwapChain_Internal(swapchain, queryResult);

        return swapchain;
    }

    bool VulkanAPI::ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode)
    {
        if (width == 0 || height == 0)
        {
            // リサイズは行わないが、処理を行わずに成功したことを意味する
            return true;
        }

        SL_LOG_TRACE("Resize Swapchain: {}, {}", width, height);
        VulkanSwapChain* vkSwapchain = (VulkanSwapChain*)swapchain;
        VulkanSurface*   vkSurface   = vkSwapchain->surface;

        // GPU待機
        VkResult result = vkDeviceWaitIdle(device);
        SL_CHECK_VKRESULT(result, false);

        // スワップチェイン仕様クエリ
        SwapChainCapability queryResult;
        SL_CHECK(!_QuerySwapChainCapability(vkSurface->surface, width, height, requestFramebufferCount, (VkPresentModeKHR)mode, queryResult), false);
        
        // スワップチェイン破棄
        _DestroySwapChain_Internal(vkSwapchain);

        // スワップチェイン生成
        _CreateSwapChain_Internal(vkSwapchain, queryResult);

        return true;
    }

    std::pair<FramebufferHandle*, TextureView*> VulkanAPI::GetCurrentBackBuffer(SwapChain* swapchain, Semaphore* present)
    {
        VulkanSwapChain* vkswapchain = (VulkanSwapChain*)swapchain;

        VkResult result = AcquireNextImageKHR(device, vkswapchain->swapchain, UINT64_MAX, ((VulkanSemaphore*)present)->semaphore, nullptr, &vkswapchain->imageIndex);
        SL_CHECK_VKRESULT(result, std::make_pair(nullptr, nullptr));

        FramebufferHandle* fb = vkswapchain->framebuffers[vkswapchain->imageIndex];
        TextureView*       tv = vkswapchain->views[vkswapchain->imageIndex];

        return {fb, tv};
    }

    bool VulkanAPI::Present(CommandQueue* queue, SwapChain* swapchain, Semaphore* render)
    {
        VulkanSwapChain*     vkswapchain      = (VulkanSwapChain*)swapchain;
        VulkanCommandQueue*  vkqueue          = (VulkanCommandQueue*)queue;

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = &((VulkanSemaphore*)render)->semaphore;
        presentInfo.swapchainCount      = 1;
        presentInfo.pSwapchains         = &vkswapchain->swapchain;
        presentInfo.pImageIndices       = &vkswapchain->imageIndex;

        VkResult result = vkQueuePresentKHR(vkqueue->queue, &presentInfo);
        SL_CHECK_VKRESULT(result, false);

        return true;
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

            // GPU待機
            VkResult result = vkDeviceWaitIdle(device);
            SL_CHECK_VKRESULT(result, SL_DONT_USE);

            // レンダーパス破棄
            vkDestroyRenderPass(device, vkSwapchain->renderpass->renderpass, nullptr);
            sldelete(vkSwapchain->renderpass);

            // スワップチェイン破棄
            _DestroySwapChain_Internal(vkSwapchain);
            sldelete(vkSwapchain);
        }
    }


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

        VulkanBuffer* buffer = slnew(VulkanBuffer);
        buffer->allocationHandle = allocation;
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
            sldelete(vkbuffer);
        }
    }

    void* VulkanAPI::MapBuffer(Buffer* buffer)
    {
        VulkanBuffer* vkbuffer = (VulkanBuffer*)buffer;

        void* mappedPtr = nullptr;
        VkResult result = vmaMapMemory(allocator, vkbuffer->allocationHandle, &mappedPtr);
        SL_CHECK_VKRESULT(result, nullptr);

        return mappedPtr;
    }

    void VulkanAPI::UnmapBuffer(Buffer* buffer)
    {
        VulkanBuffer* vkbuffer = (VulkanBuffer*)buffer;
        vmaUnmapMemory(allocator, vkbuffer->allocationHandle);
    }


    //==================================================================================
    // テクスチャ
    //==================================================================================
    TextureHandle* VulkanAPI::CreateTexture(const TextureInfo& info)
    {
        VkResult result;

        bool isCube          = info.type == TEXTURE_TYPE_CUBE || info.type == TEXTURE_TYPE_CUBE_ARRAY;
        bool isInCpuMemory   = info.usageBits & TEXTURE_USAGE_CPU_READ_BIT;
        bool isDepthStencil  = info.usageBits & TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        auto sampleCountBits = _CheckSupportedSampleCounts(info.samples);

        // キューブマップを生成する場合に指定する。
        // また、キューブマップでは layer を 6にする必要がある (キューブ配列の場合 => 6 * 配列数)
        uint32 flags = isCube? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

        // ==================== イメージ生成 ====================
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.flags         = flags;
        imageCreateInfo.imageType     = (VkImageType)info.dimension;
        imageCreateInfo.format        = (VkFormat)info.format;
        imageCreateInfo.mipLevels     = info.mipLevels;
        imageCreateInfo.arrayLayers   = info.array;
        imageCreateInfo.samples       = sampleCountBits;
        imageCreateInfo.extent.width  = info.width;
        imageCreateInfo.extent.height = info.height;
        imageCreateInfo.extent.depth  = info.depth;
        imageCreateInfo.tiling        = isInCpuMemory? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage         = (VkImageUsageFlagBits)info.usageBits;
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

        VulkanTexture* texture = slnew(VulkanTexture);
        texture->allocationHandle = allocation;
        texture->image            = vkimage;
        texture->format           = (VkFormat)info.format;
        texture->extent           = imageCreateInfo.extent;
        texture->createFlags      = imageCreateInfo.flags;
        texture->usageflags       = imageCreateInfo.usage;
        texture->mipLevels        = imageCreateInfo.mipLevels;
        texture->arrayLayers      = imageCreateInfo.arrayLayers;

        return texture;
    }

    void VulkanAPI::DestroyTexture(TextureHandle* texture)
    {
        if (texture)
        {
            VulkanTexture* vktexture = (VulkanTexture*)texture;
            vmaDestroyImage(allocator, vktexture->image, vktexture->allocationHandle);

            sldelete(vktexture);
        }
    }


    //==================================================================================
    // テクスチャビュー
    //==================================================================================
    TextureView* VulkanAPI::CreateTextureView(TextureHandle* texture, const TextureViewInfo& info)
    {
        VulkanTexture* vktex = (VulkanTexture*)texture;
        bool isDepthStencil = vktex->usageflags & TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        //-------------------------------------------------------------------------------------
        // VK_REMAINING_ARRAY_LAYERS や VK_REMAINING_MIP_LEVELS による自動指定が反映されないので
        //-------------------------------------------------------------------------------------
        uint32 numLayerCount = info.subresource.layerCount    == UINT32_MAX? vktex->arrayLayers : info.subresource.layerCount;
        uint32 numMipCount   = info.subresource.mipLevelCount == UINT32_MAX? vktex->mipLevels   : info.subresource.mipLevelCount;

        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.pNext                           = nullptr;
        viewCreateInfo.flags                           = 0;
        viewCreateInfo.image                           = vktex->image;
        viewCreateInfo.viewType                        = (VkImageViewType)info.type;
        viewCreateInfo.format                          = (VkFormat)vktex->format;
        viewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
        viewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
        viewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
        viewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
        viewCreateInfo.subresourceRange.aspectMask     = isDepthStencil? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseArrayLayer = info.subresource.baseLayer;
        viewCreateInfo.subresourceRange.layerCount     = numLayerCount;
        viewCreateInfo.subresourceRange.baseMipLevel   = info.subresource.baseMipLevel;
        viewCreateInfo.subresourceRange.levelCount     = numMipCount;

        VkImageView vkview = nullptr;
        VkResult result = vkCreateImageView(device, &viewCreateInfo, nullptr, &vkview);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanTextureView* texview = slnew(VulkanTextureView);
        texview->view        = vkview;
        texview->subresource = viewCreateInfo.subresourceRange;

        return texview;
    }

    void VulkanAPI::DestroyTextureView(TextureView* view)
    {
        if (view)
        {
            VulkanTextureView* vkview = (VulkanTextureView*)view;
            vkDestroyImageView(device, vkview->view, nullptr);

            sldelete(vkview);
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

        VulkanSampler* sampler = slnew(VulkanSampler);
        sampler->sampler = vksampler;

        return sampler;
    }

    void VulkanAPI::DestroySampler(Sampler* sampler)
    {
        if (sampler)
        {
            VulkanSampler* vksampler = (VulkanSampler * )sampler;
            vkDestroySampler(device, vksampler->sampler, nullptr);

            sldelete(vksampler);
        }
    }

    //==================================================================================
    // フレームバッファ
    //==================================================================================
    FramebufferHandle* VulkanAPI::CreateFramebuffer(RenderPass* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height)
    {
        VulkanTexture** tex = (VulkanTexture**)textures;

        //----------------------------------------------------------------------------------------
        // VK_KHR_imageless_framebuffer (vulkan 1.2)
        // 生成時にアタッチメントのイメージビューを指定せず、レンダーパス開始時まで遅延できる（フレームバッファ汎化）
        // mipmap毎にフレームバッファが必要だったが、フレームバッファの生成が1つになる
        // https://docs.vulkan.org/guide/latest/extensions/VK_KHR_imageless_framebuffer.html
        //----------------------------------------------------------------------------------------
        
        // アタッチメントイメージ情報
        VkFramebufferAttachmentImageInfo* attachments = SL_STACK(VkFramebufferAttachmentImageInfo, numTexture);
        for (uint32 i = 0; i < numTexture; i++)
        {
            attachments[i].sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
            attachments[i].pNext           = nullptr;
            attachments[i].flags           = tex[i]->createFlags;
            attachments[i].usage           = tex[i]->usageflags;
            attachments[i].width           = width;
            attachments[i].height          = height;
            attachments[i].layerCount      = tex[i]->arrayLayers;
            attachments[i].viewFormatCount = 1;                   // VK_KHR_image_format_list (vulkan 1.2)
            attachments[i].pViewFormats    = &tex[i]->format;     // VK_KHR_image_format_list (vulkan 1.2)
        }

        // アタッチメント情報
        VkFramebufferAttachmentsCreateInfo framebufferAttachments = {};
        framebufferAttachments.sType                    = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
        framebufferAttachments.pNext                    = nullptr;
        framebufferAttachments.attachmentImageInfoCount = numTexture;
        framebufferAttachments.pAttachmentImageInfos    = attachments;

        // フレームバッファ生成情報
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.flags           = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
        createInfo.pNext           = &framebufferAttachments;
        createInfo.renderPass      = ((VulkanRenderPass*)renderpass)->renderpass;
        createInfo.attachmentCount = numTexture;
        createInfo.width           = width;
        createInfo.height          = height;
        createInfo.layers          = tex[0]->arrayLayers;
        createInfo.pAttachments    = nullptr; // VK_KHR_imageless_framebuffer? nullptr : imageView;

        VkFramebuffer vkfb = nullptr;
        VkResult result = vkCreateFramebuffer(device, &createInfo, nullptr, &vkfb);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanFramebuffer* framebuffer = slnew(VulkanFramebuffer);
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

            sldelete(vkfb);
        }
    }

    //==================================================================================
    // レンダーパス
    //==================================================================================
    RenderPass* VulkanAPI::CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies, uint32 numClearValue, RenderPassClearValue* clearValue)
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
                inputAttachmentRefs[j].attachment = subpasses[i].inputReferences[j].attachment;
                inputAttachmentRefs[j].layout     = (VkImageLayout)subpasses[i].inputReferences[j].layout;
            }

            // カラーアタッチメント参照
            uint32 numColorAttachmentRef = subpasses[i].colorReferences.size();
            VkAttachmentReference* colorAttachmentRefs = SL_STACK(VkAttachmentReference, numColorAttachmentRef);
            for (uint32 j = 0; j < numColorAttachmentRef; j++)
            {
                colorAttachmentRefs[j].attachment = subpasses[i].colorReferences[j].attachment;
                colorAttachmentRefs[j].layout     = (VkImageLayout)subpasses[i].colorReferences[j].layout;
            }

            // マルチサンプル解決アタッチメント参照
            VkAttachmentReference* resolveAttachmentRef = nullptr;
            if (subpasses[i].resolveReferences.attachment != RENDER_INVALID_ID)
            {
                resolveAttachmentRef = SL_STACK(VkAttachmentReference, 1);

                resolveAttachmentRef->attachment = subpasses[i].resolveReferences.attachment;
                resolveAttachmentRef->layout     = (VkImageLayout)subpasses[i].resolveReferences.layout;
            }

            // 深度ステンシルアタッチメント参照
            VkAttachmentReference* depthstencilAttachmentRef = nullptr;
            if (subpasses[i].depthstencilReference.attachment != RENDER_INVALID_ID)
            {
                depthstencilAttachmentRef = SL_STACK(VkAttachmentReference, 1);

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

        VulkanRenderPass* renderpass = slnew(VulkanRenderPass);
        renderpass->renderpass = vkRenderPass;
        renderpass->clearValue.resize(numClearValue);

        for (uint32 i = 0; i < numClearValue; i++)
        {
            std::memcpy(&renderpass->clearValue[i], &clearValue[i], sizeof(VkClearValue));
        }


        return renderpass;
    }

    void VulkanAPI::DestroyRenderPass(RenderPass* renderpass)
    {
        if (renderpass)
        {
            VulkanRenderPass* vkRenderpass = (VulkanRenderPass*)renderpass;
            vkDestroyRenderPass(device, vkRenderpass->renderpass, nullptr);

            sldelete(vkRenderpass);
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
            imageBarriers[i].subresourceRange.baseMipLevel   = textureBarrier[i].subresources.baseMipLevel;
            imageBarriers[i].subresourceRange.levelCount     = textureBarrier[i].subresources.mipLevelCount;
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
            copyRegion[i].srcSubresource.mipLevel       = regions[i].srcSubresources.mipLevel;
            copyRegion[i].srcOffset.x                   = regions[i].srcOffset.width;
            copyRegion[i].srcOffset.y                   = regions[i].srcOffset.height;
            copyRegion[i].srcOffset.z                   = regions[i].srcOffset.depth;
            copyRegion[i].dstSubresource.aspectMask     = regions[i].dstSubresources.aspect;
            copyRegion[i].dstSubresource.baseArrayLayer = regions[i].dstSubresources.baseLayer;
            copyRegion[i].dstSubresource.layerCount     = regions[i].dstSubresources.layerCount;
            copyRegion[i].dstSubresource.mipLevel       = regions[i].dstSubresources.mipLevel;
            copyRegion[i].dstOffset.x                   = regions[i].dstOffset.width;
            copyRegion[i].dstOffset.y                   = regions[i].dstOffset.height;
            copyRegion[i].dstOffset.z                   = regions[i].dstOffset.depth;
            copyRegion[i].extent.width                  = regions[i].size.width;
            copyRegion[i].extent.height                 = regions[i].size.height;
            copyRegion[i].extent.depth                  = regions[i].size.depth;
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
        vkSubresources.baseMipLevel   = subresources.baseMipLevel;
        vkSubresources.levelCount     = subresources.mipLevelCount;
        vkSubresources.layerCount     = subresources.layerCount;
        vkSubresources.baseArrayLayer = subresources.baseLayer;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdClearColorImage(cmd->commandBuffer, vktexture->image, (VkImageLayout)textureLayout, &clearColor, 1, &vkSubresources);
    }

    void VulkanAPI::CopyBufferToTexture(CommandBuffer* commandbuffer, Buffer* srcBuffer, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, BufferTextureCopyRegion* regions)
    {
        //[ERROR] Validation Error: [ VUID-VkImageSubresourceLayers-layerCount-09243 ] 
        // Object 0: handle = 0x1a5e4aa82c0, type = VK_OBJECT_TYPE_COMMAND_BUFFER; | MessageID = 0xd8445716 | vkCmdCopyBufferToImage():
        // pRegions[0].imageSubresource.layerCount is VK_REMAINING_ARRAY_LAYERS. The Vulkan spec states:
        // If the maintenance5 feature is not enabled, layerCount must not be VK_REMAINING_ARRAY_LAYERS

        // imageSubresource.layerCount に VK_REMAINING_ARRAY_LAYERS を指定できないようです（maintenance5 を有効にする必要がある）
        // 必ずレイヤー数を指定しないといけないっぽい

        VkBufferImageCopy* copyRegion = SL_STACK(VkBufferImageCopy, numRegion);
        for (uint32 i = 0; i < numRegion; i++)
        {
            copyRegion[i] = {};
            copyRegion[i].bufferOffset                    = regions[i].bufferOffset;
            copyRegion[i].imageExtent.width               = regions[i].textureRegionSize.width;
            copyRegion[i].imageExtent.height              = regions[i].textureRegionSize.height;
            copyRegion[i].imageExtent.depth               = regions[i].textureRegionSize.depth;
            copyRegion[i].imageOffset.x                   = regions[i].textureOffset.width;
            copyRegion[i].imageOffset.y                   = regions[i].textureOffset.height;
            copyRegion[i].imageOffset.z                   = regions[i].textureOffset.depth;
            copyRegion[i].imageSubresource.aspectMask     = regions[i].textureSubresources.aspect;
            copyRegion[i].imageSubresource.baseArrayLayer = regions[i].textureSubresources.baseLayer;
            copyRegion[i].imageSubresource.layerCount     = regions[i].textureSubresources.layerCount;
            copyRegion[i].imageSubresource.mipLevel       = regions[i].textureSubresources.mipLevel;
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
            copyRegion[i].imageExtent.width               = regions[i].textureRegionSize.width;
            copyRegion[i].imageExtent.height              = regions[i].textureRegionSize.height;
            copyRegion[i].imageExtent.depth               = regions[i].textureRegionSize.depth;
            copyRegion[i].imageOffset.x                   = regions[i].textureOffset.width;
            copyRegion[i].imageOffset.y                   = regions[i].textureOffset.height;
            copyRegion[i].imageOffset.z                   = regions[i].textureOffset.depth;
            copyRegion[i].imageSubresource.aspectMask     = regions[i].textureSubresources.aspect;
            copyRegion[i].imageSubresource.baseArrayLayer = regions[i].textureSubresources.baseLayer;
            copyRegion[i].imageSubresource.layerCount     = regions[i].textureSubresources.layerCount;
            copyRegion[i].imageSubresource.mipLevel       = regions[i].textureSubresources.mipLevel;
        }

        VulkanTexture* src = (VulkanTexture*)srcTexture;
        VulkanBuffer*  dst = (VulkanBuffer*)dstBuffer;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdCopyImageToBuffer(cmd->commandBuffer, src->image, (VkImageLayout)srcTextureLayout, dst->buffer, numRegion, copyRegion);
    }

    void VulkanAPI::BlitTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureBlitRegion* regions, SamplerFilter filter)
    {
        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        VulkanTexture* srctex    = (VulkanTexture*)srcTexture;
        VulkanTexture* dsttex    = (VulkanTexture*)dstTexture;

        VkImageBlit* blitRegions = SL_STACK(VkImageBlit, numRegion);
        for (uint32 i = 0; i < numRegion; i++)
        {
            blitRegions[i].srcOffsets[0].x               = regions[i].srcOffset[0].width;
            blitRegions[i].srcOffsets[0].y               = regions[i].srcOffset[0].height;
            blitRegions[i].srcOffsets[0].z               = regions[i].srcOffset[0].depth;
            blitRegions[i].srcOffsets[1].x               = regions[i].srcOffset[1].width;
            blitRegions[i].srcOffsets[1].y               = regions[i].srcOffset[1].height;
            blitRegions[i].srcOffsets[1].z               = regions[i].srcOffset[1].depth;
            blitRegions[i].srcSubresource.aspectMask     = regions[i].srcSubresources.aspect;
            blitRegions[i].srcSubresource.mipLevel       = regions[i].srcSubresources.mipLevel;
            blitRegions[i].srcSubresource.baseArrayLayer = regions[i].srcSubresources.baseLayer;
            blitRegions[i].srcSubresource.layerCount     = regions[i].srcSubresources.layerCount;

            blitRegions[i].dstOffsets[0].x               = regions[i].dstOffset[0].width;
            blitRegions[i].dstOffsets[0].y               = regions[i].dstOffset[0].height;
            blitRegions[i].dstOffsets[0].z               = regions[i].dstOffset[0].depth;
            blitRegions[i].dstOffsets[1].x               = regions[i].dstOffset[1].width;
            blitRegions[i].dstOffsets[1].y               = regions[i].dstOffset[1].height;
            blitRegions[i].dstOffsets[1].z               = regions[i].dstOffset[1].depth;
            blitRegions[i].dstSubresource.aspectMask     = regions[i].dstSubresources.aspect;
            blitRegions[i].dstSubresource.mipLevel       = regions[i].dstSubresources.mipLevel;
            blitRegions[i].dstSubresource.baseArrayLayer = regions[i].dstSubresources.baseLayer;
            blitRegions[i].dstSubresource.layerCount     = regions[i].dstSubresources.layerCount;
        }

        vkCmdBlitImage(cmd->commandBuffer, srctex->image, (VkImageLayout)srcTextureLayout, dsttex->image, (VkImageLayout)dstTextureLayout, numRegion, blitRegions, (VkFilter)filter);
    }

    void VulkanAPI::PushConstants(CommandBuffer* commandbuffer, ShaderHandle* shader, const void* data, uint32 numData, uint32 offsetIndex)
    {
        VulkanShader*   vkshader = (VulkanShader*)shader;
        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;

        // UniformBuffer と同様に プッシュ定数もステージ間で共有する
        vkCmdPushConstants(cmd->commandBuffer, vkshader->pipelineLayout, VK_SHADER_STAGE_ALL, offsetIndex * sizeof(uint32), numData * sizeof(uint32), data);
    
        // ID3D12GraphicsCommandList* list;
        // list->SetGraphicsRoot32BitConstants(0, numData, data, offsetIndex);
        // list->SetGraphicsRoot32BitConstant(0, *(uint32*)data, offsetIndex);
    }

    void VulkanAPI::BeginRenderPass(CommandBuffer* commandbuffer, RenderPass* renderpass, FramebufferHandle* framebuffer, uint32 numView, TextureView** views, CommandBufferType commandBufferType)
    {
        VulkanFramebuffer*  vkframebuffer = (VulkanFramebuffer*)framebuffer;
        VulkanTextureView** vkviews       = (VulkanTextureView**)views;

        VkImageView* imageViews = SL_STACK(VkImageView, numView);
        for (uint32 i = 0; i < numView; i++)
        {
            imageViews[i] = vkviews[i]->view;
        }

        VkRenderPassAttachmentBeginInfo attachmentInfo = {};
        attachmentInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
        attachmentInfo.pNext           = nullptr;
        attachmentInfo.attachmentCount = numView;
        attachmentInfo.pAttachments    = imageViews;

        VkRenderPassBeginInfo begineInfo = {};
        begineInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begineInfo.pNext                    = &attachmentInfo;
        begineInfo.renderPass               = (VkRenderPass)((VulkanRenderPass*)(renderpass))->renderpass;
        begineInfo.framebuffer              = vkframebuffer->framebuffer;
        begineInfo.renderArea.offset.x      = vkframebuffer->rect.x;
        begineInfo.renderArea.offset.y      = vkframebuffer->rect.y;
        begineInfo.renderArea.extent.width  = vkframebuffer->rect.width;
        begineInfo.renderArea.extent.height = vkframebuffer->rect.height;
        begineInfo.clearValueCount          = ((VulkanRenderPass*)(renderpass))->clearValue.size();
        begineInfo.pClearValues             = (VkClearValue*)((VulkanRenderPass*)(renderpass))->clearValue.data();

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

#if SL_RENDERER_INVERT_Y_AXIS
        viewport.x        =  (float)x;
        viewport.y        =  (float)height - y;
        viewport.width    =  (float)width;
        viewport.height   = -(float)height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
#else
        viewport.x        = (float)x;
        viewport.y        = (float)y;
        viewport.width    = (float)width;
        viewport.height   = (float)height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
#endif
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

    void VulkanAPI::BindVertexBuffer(CommandBuffer* commandbuffer, Buffer* buffer, uint64 offset)
    {
        uint64 offsets[]  = { offset };
        VkBuffer vkbuffer = ((VulkanBuffer*)buffer)->buffer;

        VulkanCommandBuffer* cmd = (VulkanCommandBuffer*)commandbuffer;
        vkCmdBindVertexBuffers(cmd->commandBuffer, 0, 1, &vkbuffer, offsets);
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
    bool VulkanAPI::ImmidiateCommands(CommandQueue* queue, CommandBuffer* commandBuffer, Fence* fence, std::function<void(CommandBuffer*)>&& func)
    {
        VkCommandBuffer vkcmd   = ((VulkanCommandBuffer*)commandBuffer)->commandBuffer;
        VkQueue         vkqueue = ((VulkanCommandQueue*)queue)->queue;
        VkFence         vkfence = ((VulkanFence*)fence)->fence;

        VkResult vkresult = vkResetFences(device, 1, &vkfence);
        SL_CHECK_VKRESULT(vkresult, false);

        vkresult = vkResetCommandBuffer(vkcmd, 0);
        SL_CHECK_VKRESULT(vkresult, false);

        {
            bool result = BeginCommandBuffer(commandBuffer);
            SL_CHECK(!result, false);

            func(commandBuffer);

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

        return true;
    }

    bool VulkanAPI::WaitDevice()
    {
        VkResult vkresult = vkDeviceWaitIdle(device);
        SL_CHECK_VKRESULT(vkresult, false);

        return true;
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

#if 0
            // デスクリプタが使用されていなければ更新できるようにする
            VkDescriptorBindingFlags flags = VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;

            // デスクリプタが使用されていなければ更新できるようにするフラグを指定する構造体 (pNext に渡す)
            VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {};
            flagsInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            flagsInfo.pNext         = nullptr;
            flagsInfo.bindingCount  = layoutBindings.size();
            flagsInfo.pBindingFlags = &flags;
#endif

            // デスクリプターセットレイアウト生成
            VkDescriptorSetLayoutCreateInfo descriptorsetLayoutCreateInfo = {};
            descriptorsetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorsetLayoutCreateInfo.flags        = 0;
            descriptorsetLayoutCreateInfo.pNext        = nullptr; // &flagsInfo;
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

        ShaderReflectionData* reflectionData = slnew(ShaderReflectionData);
        *reflectionData = compiledData.reflection;

        // Vulkanデータ生成
        VulkanShader* vkshader = slnew(VulkanShader);
        vkshader->descriptorsetLayouts = layouts;
        vkshader->pipelineLayout       = vkpipelineLayout;
        vkshader->stageInfos           = shaderStages;
        vkshader->reflection           = reflectionData;

        return vkshader;
    }

    void VulkanAPI::DestroyShader(ShaderHandle* shader)
    {
        if (shader)
        {
            VulkanShader* vkshader = (VulkanShader*)shader;
            sldelete(vkshader->reflection);

            for (uint32 i = 0; i < vkshader->descriptorsetLayouts.size(); i++)
            {
                vkDestroyDescriptorSetLayout(device, vkshader->descriptorsetLayouts[i], nullptr);
            }

            vkDestroyPipelineLayout(device, vkshader->pipelineLayout, nullptr);

            for (uint32 i = 0; i < vkshader->stageInfos.size(); i++)
            {
                vkDestroyShaderModule(device, vkshader->stageInfos[i].module, nullptr);
            }

            sldelete(vkshader);
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
        std::vector<VkWriteDescriptorSet> writes(numdescriptors);

        for (uint32 i = 0; i < numdescriptors; i++)
        {
            uint32 numHandles = 1;
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

                    VulkanBuffer* buffer = (VulkanBuffer*)descriptor.handles[0].buffer;
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

                    VulkanBuffer* buffer = (VulkanBuffer*)descriptor.handles[0].buffer;
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
                    numHandles = descriptor.handles.size();
                    VkDescriptorImageInfo* imgInfos = SL_STACK(VkDescriptorImageInfo, numHandles);

                    for (uint32 j = 0; j < numHandles; j++)
                    {
                        VulkanSampler* sampler = (VulkanSampler*)descriptor.handles[j].sampler;

                        imgInfos[j] = {};
                        imgInfos[j].sampler     = sampler->sampler;
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
                    numHandles = descriptor.handles.size();
                    VkDescriptorImageInfo* imageInfos = SL_STACK(VkDescriptorImageInfo, numHandles);

                    for (uint32 j = 0; j < numHandles; j++)
                    {
                        VulkanTextureView* view = (VulkanTextureView*)descriptor.handles[j].imageView;
                        bool isDepth = view->subresource.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT;

                        imageInfos[j] = {};
                        imageInfos[j].imageView   = view->view;
                        imageInfos[j].imageLayout = isDepth? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }

                    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    writes[i].pImageInfo     = imageInfos;

                    break;
                }

                // テクスチャ + サンプル
                case DESCRIPTOR_TYPE_IMAGE_SAMPLER:
                {
                    // サンプラーとイメージがセットで1つのデスクリプタとして扱うため
                    numHandles = descriptor.handles.size();
                    VkDescriptorImageInfo* imageInfos = SL_STACK(VkDescriptorImageInfo, numHandles);

                    for (uint32 j = 0; j < numHandles; j++)
                    {
                        VulkanSampler*     sampler = (VulkanSampler*)descriptor.handles[j].sampler;
                        VulkanTextureView* view    = (VulkanTextureView*)descriptor.handles[j].imageView;
                        bool isDepth = view->subresource.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT;

                        imageInfos[j] = {};
                        imageInfos[j].sampler     = sampler->sampler;
                        imageInfos[j].imageView   = view->view;
                        imageInfos[j].imageLayout = isDepth? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }

                    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    writes[i].pImageInfo     = imageInfos;

                    break;
                }

                // ストレージイメージ
                case DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    numHandles = descriptor.handles.size();
                    VkDescriptorImageInfo* imageInfos = SL_STACK(VkDescriptorImageInfo, numHandles);

                    for (uint32 j = 0; j < numHandles; j++)
                    {
                        VulkanTextureView* view = (VulkanTextureView*)descriptor.handles[j].imageView;

                        imageInfos[j] = {};
                        imageInfos[j].imageView   = view->view;
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
            writes[i].descriptorCount = numHandles;
            key.descriptorTypeCounts[descriptor.type] += numHandles;
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

            vkUpdateDescriptorSets(device, numdescriptors, writes.data(), 0, nullptr);
        }

        VulkanDescriptorSet* descriptorset = slnew(VulkanDescriptorSet);
        descriptorset->descriptorPool = vkPool;
        descriptorset->descriptorSet  = vkdescriptorset;
        descriptorset->pipelineLayout = vkShader->pipelineLayout;
        descriptorset->poolKey        = key;
        descriptorset->writes         = writes;

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

            sldelete(vkdescriptorset);
        }
    }


    //==================================================================================
    // パイプライン
    //==================================================================================
    Pipeline* VulkanAPI::CreateGraphicsPipeline(ShaderHandle* shader, PipelineStateInfo* info, RenderPass* renderpass, uint32 renderSubpass, PipelineDynamicStateFlags dynamicState)
    {
        // ===== 頂点レイアウト =====
        VkPipelineVertexInputStateCreateInfo           vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        std::vector<VkVertexInputBindingDescription>   bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;

        if (info->inputLayout.layouts)
        {
            InputLayout* layouts      = info->inputLayout.layouts;
            uint32       numLayout    = info->inputLayout.numLayout;
            uint32       attribeIndex = 0;
            uint32       attribeSize  = 0;

            for (uint32 i = 0; i < numLayout; i++)
                attribeSize += layouts[i].attributes.size();

            attributes.resize(attribeSize);
            bindings.resize(numLayout);

            for (uint32 i = 0; i < numLayout; i++)
            {
                bindings[i] = {};
                bindings[i].binding   = layouts[i].binding;
                bindings[i].stride    = layouts[i].stride;
                bindings[i].inputRate = layouts[i].frequency == VERTEX_FREQUENCY_INSTANCE? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

                uint32 attributeSize = layouts[i].attributes.size();

                for (uint32 j = 0; j < attributeSize; j++)
                {
                    attributes[attribeIndex + j] = {};
                    attributes[attribeIndex + j].binding  = layouts[i].binding;
                    attributes[attribeIndex + j].format   = (VkFormat)layouts[i].attributes[j].format;
                    attributes[attribeIndex + j].location = layouts[i].attributes[j].location;
                    attributes[attribeIndex + j].offset   = layouts[i].attributes[j].offset;
                }

                attribeIndex += attributeSize;
            }

            vertexInputStateCreateInfo.vertexBindingDescriptionCount   = bindings.size();
            vertexInputStateCreateInfo.pVertexBindingDescriptions      = bindings.data();
            vertexInputStateCreateInfo.vertexAttributeDescriptionCount = attribeSize;
            vertexInputStateCreateInfo.pVertexAttributeDescriptions    = attributes.data();
        }


        // ===== インプットアセンブリ =====
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
        inputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.topology               = (VkPrimitiveTopology)info->inputAssembly.topology;
        inputAssemblyCreateInfo.primitiveRestartEnable = info->inputAssembly.primitiveRestartEnable;

        // ===== テッセレーション =====
        VkPipelineTessellationStateCreateInfo tessellationCreateInfo = {};
        tessellationCreateInfo.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tessellationCreateInfo.patchControlPoints = info->rasterize.patchControlPoints;

        // ===== ビューポート =====
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
        viewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.scissorCount  = 1;

        // ===== ラスタライゼーション =====
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
        rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.depthClampEnable        = info->rasterize.enableDepthClamp;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = info->rasterize.discardPrimitives;
        rasterizationStateCreateInfo.polygonMode             = info->rasterize.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.cullMode                = (PolygonCullMode)info->rasterize.cullMode;
        rasterizationStateCreateInfo.frontFace               = (info->rasterize.frontFace == POLYGON_FRONT_FACE_CLOCKWISE? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE);
        rasterizationStateCreateInfo.depthBiasEnable         = info->rasterize.depthBiasEnabled;
        rasterizationStateCreateInfo.depthBiasConstantFactor = info->rasterize.depthBiasConstantFactor;
        rasterizationStateCreateInfo.depthBiasClamp          = info->rasterize.depthBiasClamp;
        rasterizationStateCreateInfo.depthBiasSlopeFactor    = info->rasterize.depthBiasSlopeFactor;
        rasterizationStateCreateInfo.lineWidth               = info->rasterize.lineWidth;

        // ===== マルチサンプリング =====
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
        multisampleStateCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.rasterizationSamples  = _CheckSupportedSampleCounts(info->multisample.sampleCount);
        multisampleStateCreateInfo.sampleShadingEnable   = info->multisample.enableSampleShading;
        multisampleStateCreateInfo.minSampleShading      = info->multisample.minSampleShading;
        multisampleStateCreateInfo.pSampleMask           = info->multisample.sampleMask.empty()? nullptr : info->multisample.sampleMask.data();
        multisampleStateCreateInfo.alphaToCoverageEnable = info->multisample.enableAlphaToCoverage;
        multisampleStateCreateInfo.alphaToOneEnable      = info->multisample.enableAlphaToOne;

        // ===== デプス・ステンシル =====
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
        depthStencilStateCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.depthTestEnable       = info->depthStencil.enableDepthTest;
        depthStencilStateCreateInfo.depthWriteEnable      = info->depthStencil.enableDepthWrite;
        depthStencilStateCreateInfo.depthCompareOp        = (VkCompareOp)info->depthStencil.depthCompareOp;
        depthStencilStateCreateInfo.depthBoundsTestEnable = info->depthStencil.enableDepthRange;
        depthStencilStateCreateInfo.stencilTestEnable     = info->depthStencil.enableStencil;
        depthStencilStateCreateInfo.front.failOp          = (VkStencilOp)info->depthStencil.frontOp.fail;
        depthStencilStateCreateInfo.front.passOp          = (VkStencilOp)info->depthStencil.frontOp.pass;
        depthStencilStateCreateInfo.front.depthFailOp     = (VkStencilOp)info->depthStencil.frontOp.depthFail;
        depthStencilStateCreateInfo.front.compareOp       = (VkCompareOp)info->depthStencil.frontOp.compare;
        depthStencilStateCreateInfo.front.compareMask     = info->depthStencil.frontOp.compareMask;
        depthStencilStateCreateInfo.front.writeMask       = info->depthStencil.frontOp.writeMask;
        depthStencilStateCreateInfo.front.reference       = info->depthStencil.frontOp.reference;
        depthStencilStateCreateInfo.back.failOp           = (VkStencilOp)info->depthStencil.backOp.fail;
        depthStencilStateCreateInfo.back.passOp           = (VkStencilOp)info->depthStencil.backOp.pass;
        depthStencilStateCreateInfo.back.depthFailOp      = (VkStencilOp)info->depthStencil.backOp.depthFail;
        depthStencilStateCreateInfo.back.compareOp        = (VkCompareOp)info->depthStencil.backOp.compare;
        depthStencilStateCreateInfo.back.compareMask      = info->depthStencil.backOp.compareMask;
        depthStencilStateCreateInfo.back.writeMask        = info->depthStencil.backOp.writeMask;
        depthStencilStateCreateInfo.back.reference        = info->depthStencil.backOp.reference;
        depthStencilStateCreateInfo.minDepthBounds        = info->depthStencil.depthRangeMin;
        depthStencilStateCreateInfo.maxDepthBounds        = info->depthStencil.depthRangeMax;

        // ===== ブレンドステート =====
        uint32 numColorAttachments = info->blend.attachments.size();

        VkPipelineColorBlendAttachmentState* attachmentStates = SL_STACK(VkPipelineColorBlendAttachmentState, numColorAttachments);
        for (uint32 i = 0; i < numColorAttachments; i++)
        {
            attachmentStates[i] = {};
            attachmentStates[i].blendEnable         = info->blend.attachments[i].enableBlend;
            attachmentStates[i].srcColorBlendFactor = (VkBlendFactor)info->blend.attachments[i].srcColorBlendFactor;
            attachmentStates[i].dstColorBlendFactor = (VkBlendFactor)info->blend.attachments[i].dstColorBlendFactor;
            attachmentStates[i].colorBlendOp        = (VkBlendOp)    info->blend.attachments[i].colorBlendOp;
            attachmentStates[i].srcAlphaBlendFactor = (VkBlendFactor)info->blend.attachments[i].srcAlphaBlendFactor;
            attachmentStates[i].dstAlphaBlendFactor = (VkBlendFactor)info->blend.attachments[i].dstAlphaBlendFactor;
            attachmentStates[i].alphaBlendOp        = (VkBlendOp)    info->blend.attachments[i].alphaBlendOp;

            if (info->blend.attachments[i].write_r) attachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
            if (info->blend.attachments[i].write_g) attachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
            if (info->blend.attachments[i].write_b) attachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
            if (info->blend.attachments[i].write_a) attachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
        }

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
        colorBlendStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable     = info->blend.enableLogicOp;
        colorBlendStateCreateInfo.logicOp           = (VkLogicOp)info->blend.logicOp;
        colorBlendStateCreateInfo.attachmentCount   = numColorAttachments;
        colorBlendStateCreateInfo.pAttachments      = attachmentStates;
        colorBlendStateCreateInfo.blendConstants[0] = info->blend.blendConstant.r;
        colorBlendStateCreateInfo.blendConstants[1] = info->blend.blendConstant.g;
        colorBlendStateCreateInfo.blendConstants[2] = info->blend.blendConstant.b;
        colorBlendStateCreateInfo.blendConstants[3] = info->blend.blendConstant.a;


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
        VkPipelineShaderStageCreateInfo* pipelineStages = SL_STACK(VkPipelineShaderStageCreateInfo, vkshader->stageInfos.size());
        for (uint32 i = 0; i < vkshader->stageInfos.size(); i++)
        {
            pipelineStages[i] = vkshader->stageInfos[i];
        }


        // ===== パイプライン生成 =====
        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext               = nullptr;
        pipelineCreateInfo.stageCount          = vkshader->stageInfos.size();
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

        VulkanPipeline* pipeline = slnew(VulkanPipeline);
        pipeline->pipeline = vkpipeline;

        return pipeline;
    }

    Pipeline* VulkanAPI::CreateComputePipeline(ShaderHandle* shader)
    {
        VulkanShader* vkshader = (VulkanShader*)shader;

        VkComputePipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext  = nullptr;
        pipelineCreateInfo.stage  = vkshader->stageInfos[0];
        pipelineCreateInfo.layout = vkshader->pipelineLayout;

        VkPipeline vkpipeline = nullptr;
        VkResult result = vkCreateComputePipelines(device, nullptr, 1, &pipelineCreateInfo, nullptr, &vkpipeline);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanPipeline* pipeline = slnew(VulkanPipeline);
        pipeline->pipeline = vkpipeline;

        return pipeline;
    }

    void VulkanAPI::DestroyPipeline(Pipeline* pipeline)
    {
        if (pipeline)
        {
            VulkanPipeline* vkpipeline = (VulkanPipeline*)pipeline;
            vkDestroyPipeline(device, vkpipeline->pipeline, nullptr);

            sldelete(vkpipeline);
        }
    }
}
