
#pragma once

#include "Rendering/RenderingCore.h"
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
        uint32  family = INVALID_RENDER_ID;
        uint32  index  = INVALID_RENDER_ID;
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
        VkRenderPass renderpass = nullptr;
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
        VkBuffer      buffer = nullptr;
        VkBufferView  view = nullptr;
        uint64        size = 0;
        VmaAllocation allocationHandle = nullptr;
        uint64        allocationSize = 0;
    };

    // テクスチャ
    struct VulkanTexture : public TextureHandle
    {
        VkImage     image = nullptr;
        VkImageView imageView = nullptr;
        VkExtent3D  extent = {};

        VmaAllocation     allocationHandle = nullptr;
        VmaAllocationInfo allocationInfo = {};
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
    struct VulkanShader : public ShaderHandle
    {
        VkShaderStageFlags                           stageFlags = 0;
        std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
        std::vector<VkDescriptorSetLayout>           descriptorsetLayouts;
        VkPipelineLayout                             pipelineLayout;
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
        VkDescriptorSet  descriptorSet = nullptr;
        VkDescriptorPool descriptorPool = nullptr;

        DescriptorSetPoolKey poolKey;
    };

    // パイプライン
    struct VulkanPipeline : public Pipeline
    {
        VkPipeline pipeline = nullptr;
    };
}
