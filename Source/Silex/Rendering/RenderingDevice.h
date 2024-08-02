
#pragma once

#include "Core/TaskQueue.h"
#include "Rendering/RenderingCore.h"
#include "Rendering/ShaderCompiler.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;
    class Mesh;

    // 削除待機リソース
    struct PendingDestroyResourceQueue
    {
        std::vector<Buffer*>            buffer;
        std::vector<TextureHandle*>     texture;
        std::vector<Sampler*>           sampler;
        std::vector<DescriptorSet*>     descriptorset;
        std::vector<FramebufferHandle*> framebuffer;
        std::vector<ShaderHandle*>      shader;
        std::vector<Pipeline*>          pipeline;
    };

    // フレームデータ
    struct FrameData
    {
        CommandPool*                commandPool      = nullptr;
        CommandBuffer*              commandBuffer    = nullptr;
        Semaphore*                  presentSemaphore = nullptr;
        Semaphore*                  renderSemaphore  = nullptr;
        Fence*                      fence            = nullptr;
        bool                        waitingSignal    = false;
        PendingDestroyResourceQueue pendingResources;
    };

    // 即時コマンドデータ
    struct ImmidiateCommandData
    {
        CommandPool*   commandPool   = nullptr;
        CommandBuffer* commandBuffer = nullptr;
        Fence*         fence         = nullptr;
    };

    class RenderingDevice : public Object
    {
        SL_CLASS(RenderingDevice, Object);

    public:

        RenderingDevice();
        ~RenderingDevice();

        static RenderingDevice* Get();

    public:

        bool Initialize(RenderingContext* renderingContext);
        bool Begin();
        bool End();

        // レンダリング環境
        RenderingContext* GetContext() const;
        RenderingAPI*     GetAPI()     const;

        // コマンドキュー
        QueueFamily   GetGraphicsQueueFamily()  const;
        CommandQueue* GetGraphicsCommandQueue() const;

        // フレームデータ
        FrameData&       GetFrameData();
        const FrameData& GetFrameData() const;

        // 保留中リソース削除
        void DestroyPendingResources(uint32 frame);

    public:

        // デバイス情報
        const DeviceInfo& GetDeviceInfo() const;

        // テクスチャ
        TextureHandle* CreateTextureFromMemory(const byte* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);

        // レンダーテクスチャ
        TextureHandle* CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap);
        TextureHandle* CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap);
        TextureHandle* CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap);
        void           DestroyTexture(TextureHandle* texture);

        // バッファ
        Buffer* CreateUniformBuffer(void* data, uint64 dataByte);
        Buffer* CreateStorageBuffer(void* data, uint64 dataByte);
        Buffer* CreateVertexBuffer(void* data, uint64 dataByte);
        Buffer* CreateIndexBuffer(void* data, uint64 dataByte);
        void    DestroyBuffer(Buffer* buffer);

        // フレームバッファ
        FramebufferHandle* CreateFramebuffer(RenderPass* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height);
        void               DestroyFramebuffer(FramebufferHandle* framebuffer);

        // デスクリプターセット
        DescriptorSet* CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex, const DescriptorSetInfo& setInfo);
        void           DestroyDescriptorSet(DescriptorSet* set);

        // スワップチェイン
        SwapChain* CreateSwapChain(Surface* surface, uint32 width, uint32 height, VSyncMode mode);
        bool       ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, VSyncMode mode);
        void       DestoySwapChain(SwapChain* swapchain);
        bool       Present();
    
    private:

        // バッファヘルパー
        Buffer* _CreateBuffer();

        // テクスチャヘルパー
        void           _GenerateMipmaps(CommandBuffer* cmd, TextureHandle* texture, uint32 width, uint32 height, uint32 depth, uint32 array, TextureAspectFlags aspect);
        TextureHandle* _CreateTexture(TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 depth, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags);

    public:

        void TEST();
        void DOCK_SPACE(class Camera* camera);
        void DRAW(class Camera* camera);
        void RESIZE(uint32 width, uint32 height);

    private:

        Buffer*            sceneUBO             = nullptr;
        void*              mappedSceneData      = nullptr;
        Buffer*            gridUBO              = nullptr;
        void*              mappedGridData       = nullptr;

        // Scene
        glm::ivec2         sceneFramebufferSize = {};
        TextureHandle*     sceneColorTexture    = nullptr;
        TextureHandle*     sceneDepthTexture    = nullptr;
        Sampler*           sceneSampler         = nullptr;

        FramebufferHandle* sceneFramebuffer     = nullptr;
        FramebufferHandle* swapchainFramebuffer = nullptr;

        RenderPass*        scenePass     = nullptr;
        RenderPass*        swapchainPass = nullptr;

        Pipeline*          pipeline      = nullptr;
        ShaderHandle*      shader        = nullptr;
        DescriptorSet*     descriptorSet = nullptr;
        DescriptorSet*     textureSet    = nullptr;

        ShaderHandle*      gridShader    = nullptr;
        Pipeline*          gridPipeline  = nullptr;
        DescriptorSet*     gridSet       = nullptr;

        ShaderHandle*      blitShader    = nullptr;
        Pipeline*          blitPipeline  = nullptr;
        DescriptorSet*     blitSet       = nullptr;

        // グリッド


        Buffer* vb = nullptr;
        Buffer* ib = nullptr;

        Mesh* cubeMesh   = nullptr;
        Mesh* sphereMesh = nullptr;

        TextureHandle* textureFile = nullptr;

    private:

        ImmidiateCommandData     immidiateContext = {};
        std::array<FrameData, 2> frameData        = {};
        uint64                   frameIndex       = 0;

        RenderingContext* context = nullptr;
        RenderingAPI*     api     = nullptr;

        QueueFamily   graphicsQueueFamily = RENDER_INVALID_ID;
        CommandQueue* graphicsQueue       = nullptr;

        RenderPassClearValue defaultClearColor        = {};
        RenderPassClearValue defaultClearDepthStencil = {};

    private:

        static inline RenderingDevice* instance = nullptr;
    };
}

