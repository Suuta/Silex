
#pragma once

#include "Rendering/RenderingAPI.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_mem_alloc.h>


//----------------------------------------------------
// memo
//----------------------------------------------------
// アセットは Ref<TextureAsset> 形式で保持?
// 
// レンダーオブジェクトをメンバーに内包する形で表現
// class TextureAsset : public Asset
// { 
//     Texture* texture;
// }
// 
// アセットマネージャの対象は ○○Asset から行うようにし
// Texture 自体は、レンダラー側で管理できるようにする
//----------------------------------------------------

namespace Silex
{
    class  VulkanContext;
    struct VulkanSurface;

    // デバイス拡張機能関数
    struct DeviceExtensionFunctions
    {
        PFN_vkCreateSwapchainKHR    vkCreateSwapchainKHR    = nullptr;
        PFN_vkDestroySwapchainKHR   vkDestroySwapchainKHR   = nullptr;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
        PFN_vkAcquireNextImageKHR   vkAcquireNextImageKHR   = nullptr;
        PFN_vkQueuePresentKHR       vkQueuePresentKHR       = nullptr;
    };

    //=============================================
    // Vulkan API 実装
    //=============================================
    class VulkanAPI : public RenderingAPI
    {
        SL_CLASS(VulkanAPI, RenderingAPI)

    public:

        VulkanAPI(VulkanContext* context);
        ~VulkanAPI();

        bool Initialize() override;

        // コマンドキュー
        CommandQueue* CreateCommandQueue(QueueFamily family, uint32 indexInFamily = 0) override;
        void DestroyCommandQueue(CommandQueue* queue) override;
        QueueFamily QueryQueueFamily(uint32 queueFlag, Surface* surface = nullptr) const override;

        // コマンドプール
        CommandPool* CreateCommandPool(QueueFamily family, CommandBufferType type) override;
        void DestroyCommandPool(CommandPool* pool) override;

        // コマンドバッファ
        CommandBuffer* CreateCommandBuffer(CommandPool* pool) override;
        void DestroyCommandBuffer(CommandBuffer* commandBuffer) override;
        bool BeginCommandBuffer(CommandBuffer* commandBuffer) override;
        bool EndCommandBuffer(CommandBuffer* commandBuffer) override;

        // セマフォ
        Semaphore* CreateSemaphore() override;
        void DestroySemaphore(Semaphore* semaphore) override;

        // フェンス
        Fence* CreateFence() override;
        void DestroyFence(Fence* fence) override;
        bool WaitFence(Fence* fence) override;

        // スワップチェイン
        SwapChain* CreateSwapChain(Surface* surface) override;
        bool ResizeSwapChain(SwapChain* swapchain, uint32 requestFramebufferCount, VSyncMode mode) override;
        FramebufferHandle* GetSwapChainNextFramebuffer(SwapChain* swapchain, Semaphore* semaphore) override;
        RenderPass* GetSwapChainRenderPass(SwapChain* swapchain) override;
        RenderingFormat GetSwapChainFormat(SwapChain* swapchain) override;
        void DestroySwapChain(SwapChain* swapchain) override;

        // バッファ
        Buffer* CreateBuffer(uint64 size, BufferUsageBits usage, MemoryAllocationType memoryType) override;
        void DestroyBuffer(Buffer* buffer) override;
        byte* MapBuffer(Buffer* buffer) override;
        void UnmapBuffer(Buffer* buffer) override;

        // テクスチャ
        TextureHandle* CreateTexture(const TextureFormat& format) override;
        void DestroyTexture(TextureHandle* texture) override;

        // サンプラ
        Sampler* CreateSampler(const SamplerState& state) override;
        void DestroySampler(Sampler* sampler) override;

        // フレームバッファ
        FramebufferHandle* CreateFramebuffer(RenderPass* renderpass, TextureHandle* textures, uint32 numTexture, uint32 width, uint32 height) override;
        void DestroyFramebuffer(FramebufferHandle* framebuffer) override;

        // 頂点フォーマット
        VertexFormat* CreateVertexFormat(uint32 numattributes, VertexAttribute* attributes) override;
        void DestroyVertexFormat(VertexAttribute* attributes) override;

        // レンダーパス
        RenderPass* CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies) override;
        void DestroyRenderPass(RenderPass* renderpass) override;

        // バリア
        void PipelineBarrier(CommandBuffer* commanddBuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrier* memoryBarrier, uint32 numBufferBarrier, BufferBarrier* bufferBarrier, uint32 numTextureBarrier, TextureBarrier* textureBarrier) override;

        // シェーダー
        virtual std::vector<byte> CompileSPIRV(uint32 numSpirv, ShaderStageSPIRVData* spirv, const std::string& shaderName) override;
        virtual ShaderHandle* CreateShader(const std::vector<byte>& p_shader_binary, ShaderDescription& shaderDesc, std::string& name) override;
        virtual void DestroyShader(ShaderHandle* shader) override;

        // デスクリプターセット
        DescriptorSet* CreateDescriptorSet() override;
        void DestroyDescriptorSet() override;

        // パイプライン
        virtual Pipeline* CreatePipeline(
            ShaderHandle*              shader,
            VertexFormat*              vertexFormat,
            PrimitiveTopology          primitive,
            PipelineRasterizationState rasterizationState,
            PipelineMultisampleState   multisampleState,
            PipelineDepthStencilState  depthstencilState,
            PipelineColorBlendState    blendState,
            int32*                     colorAttachments,
            int32                      numColorAttachments,
            PipelineDynamicStateFlags  dynamicState,
            RenderPass*                renderpass,
            uint32                     renderSubpass
        ) override;

        void DestroyPipeline(Pipeline* pipeline) override;

    private:

        // デバイス拡張機能関数
        DeviceExtensionFunctions extensions;

        // レンダリングコンテキスト
        VulkanContext* context = nullptr;

        // 論理デバイス
        VkDevice device = nullptr;

        // VMAアロケータ (VulkanMemoryAllocator: VkImage/VkBuffer の生成にともなうメモリ管理を代行)
        VmaAllocator allocator = nullptr;
    };
}
