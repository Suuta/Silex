
#pragma once

#include "Rendering/RenderingCore.h"
#include "Rendering/ShaderCompiler.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_mem_alloc.h>


namespace Silex
{
    // プール検索キー
    struct DescriptorSetPoolKey
    {
        uint16 descriptorTypeCounts[DESCRIPTOR_TYPE_MAX] = {};
        bool operator<(const DescriptorSetPoolKey& other) const
        {
            return memcmp(descriptorTypeCounts, other.descriptorTypeCounts, sizeof(descriptorTypeCounts)) < 0;
        }
    };

    // スワップチェイン情報クエリ
    struct SwapChainCapability
    {
        uint32                        minImageCount;
        VkFormat                      format;
        VkColorSpaceKHR               colorspace;
        VkExtent2D                    extent;
        VkSurfaceTransformFlagBitsKHR transform;
        VkCompositeAlphaFlagBitsKHR   compositeAlpha;
        VkPresentModeKHR              presentMode;
    };

    //=============================================
    // Vulkan 構造体
    //=============================================

    // サーフェース
    struct VulkanSurface : public Surface
    {
        VkSurfaceKHR surface = nullptr;
    };

    // コマンドキュー
    struct VulkanCommandQueue : public CommandQueue
    {
        VkQueue queue  = nullptr;
        uint32  family = RENDER_INVALID_ID;
        uint32  index  = RENDER_INVALID_ID;
    };

    // コマンドプール
    struct VulkanCommandPool : public CommandPool
    {
        VkCommandPool     commandPool = nullptr;
        CommandBufferType type = COMMAND_BUFFER_TYPE_PRIMARY;
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
        VkRenderPass              renderpass = nullptr;
        std::vector<VkClearValue> clearValue = {};
    };

    // フレームバッファ
    struct VulkanFramebuffer : public FramebufferHandle
    {
        VkFramebuffer framebuffer = nullptr;
        Rect          rect        = {};
    };

    // スワップチェイン
    struct VulkanSwapChain : public SwapChain
    { 
        VkSwapchainKHR  swapchain  = nullptr;
        VkColorSpaceKHR colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkFormat        format     = VK_FORMAT_UNDEFINED;
        uint32          imageIndex = 0;

        std::vector<FramebufferHandle*> framebuffers = {};
        std::vector<TextureHandle*>     textures     = {};
        std::vector<TextureView*>       views        = {};
        VulkanSurface*                  surface      = nullptr;
        VulkanRenderPass*               renderpass   = nullptr;

        VkSemaphore present = nullptr;
        VkSemaphore render  = nullptr;
    };

    // バッファ
    struct VulkanBuffer : public Buffer
    {
        VkBuffer      buffer           = nullptr;
        VkBufferView  view             = nullptr;
        uint64        size             = 0;
        VmaAllocation allocationHandle = nullptr;

        bool  mapped  = false;
        void* pointer = nullptr;
    };

    // テクスチャ
    struct VulkanTexture : public TextureHandle
    {
        VkImage            image       = nullptr;
        VkFormat           format      = {};
        VkImageUsageFlags  usageflags  = {};
        VkImageCreateFlags createFlags = {};
        VkExtent3D         extent      = {};
        uint32             arrayLayers = 1;
        uint32             mipLevels   = 1;

        VmaAllocation allocationHandle = nullptr;
    };

    // テクスチャビュー
    struct VulkanTextureView : public TextureView
    {
        VkImageSubresourceRange subresource = {};
        VkImageView             view        = nullptr;
    };

    // サンプラー
    struct VulkanSampler : public Sampler
    {
        VkSampler sampler = nullptr;
    };

    // シェーダー
    struct VulkanShader : public ShaderHandle
    {
        std::vector<VkPipelineShaderStageCreateInfo> stageInfos           = {};
        std::vector<VkDescriptorSetLayout>           descriptorsetLayouts = {};
        VkPipelineLayout                             pipelineLayout       = nullptr;
        ShaderReflectionData*                        reflection           = nullptr;
    };

    // デスクリプターセット
    struct VulkanDescriptorSet : public DescriptorSet
    {
        VkDescriptorSet  descriptorSet  = nullptr;
        VkDescriptorPool descriptorPool = nullptr;
        VkPipelineLayout pipelineLayout = nullptr;

        std::vector<VkWriteDescriptorSet> writes;

        DescriptorSetPoolKey poolKey;
    };

    // パイプライン
    struct VulkanPipeline : public Pipeline
    {
        VkPipeline pipeline = nullptr;
    };



    //=============================================
    // Vulkan 型キャスト
    //---------------------------------------------
    // 抽象型からAPI型への 型安全キャスト
    //=============================================
    template<class T> struct VulkanTypeTraits {};

    template<> struct VulkanTypeTraits<Buffer>            { using Internal = VulkanBuffer;        };
    template<> struct VulkanTypeTraits<TextureHandle>     { using Internal = VulkanTexture;       };
    template<> struct VulkanTypeTraits<TextureView>       { using Internal = VulkanTextureView;   };
    template<> struct VulkanTypeTraits<Sampler>           { using Internal = VulkanSampler;       };
    template<> struct VulkanTypeTraits<ShaderHandle>      { using Internal = VulkanShader;        };
    template<> struct VulkanTypeTraits<FramebufferHandle> { using Internal = VulkanFramebuffer;   };
    template<> struct VulkanTypeTraits<CommandQueue>      { using Internal = VulkanCommandQueue;  };
    template<> struct VulkanTypeTraits<CommandBuffer>     { using Internal = VulkanCommandBuffer; };
    template<> struct VulkanTypeTraits<CommandPool>       { using Internal = VulkanCommandPool;   };
    template<> struct VulkanTypeTraits<Fence>             { using Internal = VulkanFence;         };
    template<> struct VulkanTypeTraits<Semaphore>         { using Internal = VulkanSemaphore;     };
    template<> struct VulkanTypeTraits<DescriptorSet>     { using Internal = VulkanDescriptorSet; };
    template<> struct VulkanTypeTraits<Pipeline>          { using Internal = VulkanPipeline;      };
    template<> struct VulkanTypeTraits<RenderPass>        { using Internal = VulkanRenderPass;    };
    template<> struct VulkanTypeTraits<Surface>           { using Internal = VulkanSurface;       };
    template<> struct VulkanTypeTraits<SwapChain>         { using Internal = VulkanSwapChain;     };


    template<typename T>
    SL_FORCEINLINE typename VulkanTypeTraits<T>::Internal* VulkanCast(T* type)
    {
        return static_cast<typename VulkanTypeTraits<T>::Internal*>(type);
    }

    template<typename T>
    SL_FORCEINLINE typename VulkanTypeTraits<T>::Internal* VulkanCast(const T* type)
    {
        return static_cast<const typename VulkanTypeTraits<T>::Internal*>(type);
    }

    template<typename T>
    SL_FORCEINLINE typename VulkanTypeTraits<T>::Internal** VulkanCast(T** type)
    {
        return reinterpret_cast<typename VulkanTypeTraits<T>::Internal**>(type);
    }

    template<typename T>
    SL_FORCEINLINE typename VulkanTypeTraits<T>::Internal** VulkanCast(const T** type)
    {
        return reinterpret_cast<const typename VulkanTypeTraits<T>::Internal**>(type);
    }
}
