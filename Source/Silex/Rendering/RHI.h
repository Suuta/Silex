
#pragma once

#include "Core/TaskQueue.h"
#include "Rendering/RenderingCore.h"
#include "Rendering/ShaderCompiler.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;

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

    class RHI : public Object
    {
        SL_CLASS(RHI, Object);

    public:

        RHI();
        ~RHI();

        // インスタンス
        static RHI* Get();

        // 初期化
        bool Initialize(RenderingContext* renderingContext);

        // フレーム同期
        bool BeginFrame();
        bool EndFrame();

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

        // デバイス情報
        const DeviceInfo& GetDeviceInfo() const;

        //===========================================================
        // API
        //===========================================================

        // テクスチャ
        TextureHandle* CreateTextureFromMemory(const byte*  pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);
        TextureHandle* CreateTextureFromMemory(const float* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);

        // レンダーテクスチャ
        TextureHandle* CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap = false);
        TextureHandle* CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap = false);
        TextureHandle* CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap = false);
        void           DestroyTexture(TextureHandle* texture);

        // バッファ
        Buffer* CreateUniformBuffer(void* data, uint64 size, void** outMappedAdress);
        Buffer* CreateStorageBuffer(void* data, uint64 size, void** outMappedAdress);
        Buffer* CreateVertexBuffer(void* data, uint64 size);
        Buffer* CreateIndexBuffer(void* data, uint64 size);
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

        //===========================================================
        // ヘルパー
        //===========================================================

        Buffer*        _CreateAndMapBuffer(BufferUsageBits type, const void* data, uint64 dataSize, void** outMappedAdress);
        Buffer*        _CreateAndSubmitBufferData(BufferUsageBits type, const void* data, uint64 dataSize);

        TextureHandle* _CreateTexture(TextureDimension dimension, TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 depth, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags);
        void           _SubmitTextureData(TextureHandle* texture, uint32 width, uint32 height, bool genMipmap, const void* pixelData, uint64 dataSize);
        void           _GenerateMipmaps(CommandBuffer* cmd, TextureHandle* texture, uint32 width, uint32 height, uint32 depth, uint32 array, TextureAspectFlags aspect);

        //===========================================================
        // メンバ
        //===========================================================

        ImmidiateCommandData     immidiateContext = {};
        std::array<FrameData, 2> frameData        = {};
        uint64                   frameIndex       = 0;

        RenderingContext* context = nullptr;
        RenderingAPI*     api     = nullptr;

        QueueFamily   graphicsQueueFamily = RENDER_INVALID_ID;
        CommandQueue* graphicsQueue       = nullptr;

        RenderPassClearValue defaultClearColor        = {};
        RenderPassClearValue defaultClearDepthStencil = {};

        //===========================================================
        // インスタンス
        //===========================================================
        static inline RHI* instance = nullptr;


        //===========================================================
        // テストコード
        //===========================================================
    public:

        void TEST();
        void Update(class Camera* camera);
        void Render(class Camera* camera, float dt);
        void RESIZE(uint32 width, uint32 height);

    private:

        // キューブマップ変換
        Pipeline*          equirectangularPipeline = nullptr;
        FramebufferHandle* equirectangularFB       = nullptr;
        RenderPass*        equirectangularPass     = nullptr;
        ShaderHandle*      equirectangularShader   = nullptr;
        DescriptorSet*     equirectangularSet      = nullptr;
        Buffer*            equirectangularUBO      = nullptr;
        void*              mappedEquirectanguler   = nullptr;
        TextureHandle*     cubemap                 = nullptr;
        Sampler*           cubemapSampler          = nullptr;

        // スカイボックス
        Pipeline*          skyPipeline = nullptr;
        ShaderHandle*      skyShader   = nullptr;

        // シーンBlit
        DescriptorSet*     imageSet     = nullptr;
        FramebufferHandle* imageFB      = nullptr;
        TextureHandle*     imageTexture = nullptr;
        RenderPass*        imagePass    = nullptr;


        // シーンバッファ
        Buffer*            sceneUBO        = nullptr;
        void*              mappedSceneData = nullptr;
        Buffer*            gridUBO         = nullptr;
        void*              mappedGridData  = nullptr;
        Buffer*            lightUBO        = nullptr;
        void*              mappedLightData = nullptr;

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
        DescriptorSet*     lightSet      = nullptr;

        ShaderHandle*      gridShader    = nullptr;
        Pipeline*          gridPipeline  = nullptr;
        DescriptorSet*     gridSet       = nullptr;

        ShaderHandle*      blitShader    = nullptr;
        Pipeline*          blitPipeline  = nullptr;
        DescriptorSet*     blitSet       = nullptr;

        Buffer* vb = nullptr;
        Buffer* ib = nullptr;

        //class Mesh* sphereMesh = nullptr;
        class Mesh* cubeMesh   = nullptr;
        class Mesh* sponzaMesh = nullptr;

        TextureHandle* textureFile = nullptr;
        TextureHandle* envTexture  = nullptr;
    };
}

