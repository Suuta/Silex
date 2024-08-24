
#pragma once

#include "Rendering/RenderingCore.h"
#include "Rendering/ShaderCompiler.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_mem_alloc.h>


namespace Silex
{
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
    struct VulkanCommandPool : public CommandQueue
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
        uint32        layer       = 0;
    };

    // スワップチェイン
    struct VulkanSwapChain : public SwapChain
    {
        VulkanSurface*    surface    = nullptr;
        VulkanRenderPass* renderpass = nullptr;

        VkSwapchainKHR  swapchain  = nullptr;
        VkFormat        format     = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        VkSemaphore present = nullptr;
        VkSemaphore render  = nullptr;

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
    };

    // テクスチャ
    struct VulkanTexture : public TextureHandle
    {
        VkImage                  image     = nullptr;
        VkImageView              imageView = nullptr;
        std::vector<VkImageView> mipView   = {};

        VkImageViewType          imageType   = {};
        VkFormat                 format      = {};
        VkExtent3D               extent      = {};
        VkImageSubresourceRange  subresource = {};
        uint32                   createFlags = {};

        VmaAllocation     allocationHandle = nullptr;
    };

    // サンプラー
    struct VulkanSampler : public Sampler
    {
        VkSampler sampler = nullptr;
    };

    // シェーダー
    struct VulkanShader : public ShaderHandle
    {
        VkShaderStageFlags                           stageFlags = 0;
        std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
        std::vector<VkDescriptorSetLayout>           descriptorsetLayouts;
        VkPipelineLayout                             pipelineLayout;
        ShaderReflectionData*                        reflection;
    };

    // プール検索キー
    struct DescriptorSetPoolKey
    {
        uint16 descriptorTypeCounts[DESCRIPTOR_TYPE_MAX] = {};
        bool operator<(const DescriptorSetPoolKey& other) const
        {
            return memcmp(descriptorTypeCounts, other.descriptorTypeCounts, sizeof(descriptorTypeCounts)) < 0;
        }
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
}
