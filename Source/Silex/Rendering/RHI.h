
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
        std::vector<TextureView*>       textureView;
        std::vector<Sampler*>           sampler;
        std::vector<DescriptorSet*>     descriptorset;
        std::vector<FramebufferHandle*> framebuffer;
        std::vector<ShaderHandle*>      shader;
        std::vector<Pipeline*>          pipeline;
    };


    // フレームデータ
    struct FrameData
    {
        CommandPool*                 commandPool      = nullptr;
        CommandBuffer*               commandBuffer    = nullptr;
        Semaphore*                   presentSemaphore = nullptr;
        Semaphore*                   renderSemaphore  = nullptr;
        Fence*                       fence            = nullptr;
        bool                         waitingSignal    = false;
        PendingDestroyResourceQueue* pendingResources;
    };


    // 即時コマンドデータ
    struct ImmidiateCommandData
    {
        CommandPool*   commandPool   = nullptr;
        CommandBuffer* commandBuffer = nullptr;
        Fence*         fence         = nullptr;
    };


    // Gバッファ
    struct GBuffer
    {
        RenderPass*        pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        Pipeline*          pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;

        TextureHandle* albedo   = nullptr;
        TextureHandle* normal   = nullptr;
        TextureHandle* emission = nullptr;
        TextureHandle* id       = nullptr;
        TextureHandle* depth    = nullptr;

        TextureView* albedoView   = nullptr;
        TextureView* normalView   = nullptr;
        TextureView* emissionView = nullptr;
        TextureView* idView       = nullptr;
        TextureView* depthView    = nullptr;

        Buffer*        materialUBO     = nullptr;
        void*          mappedMaterial  = nullptr;
        Buffer*        transformUBO    = nullptr;
        void*          mappedTransfrom = nullptr;

        DescriptorSet* transformSet = nullptr;
        DescriptorSet* materialSet  = nullptr;
    };

    struct LightingBuffer
    {
        RenderPass*        pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        TextureHandle*     color       = nullptr;
        TextureView*       view        = nullptr;

        Pipeline*     pipeline = nullptr;
        ShaderHandle* shader   = nullptr;

        Buffer*        sceneUBO    = nullptr;
        void*          mappedScene = nullptr;
        DescriptorSet* set         = nullptr;
    };

    struct EnvironmentBuffer
    {
        RenderPass*        pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        Pipeline*          pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;
        Buffer*            ubo         = nullptr;
        void*              mapped      = nullptr;
        DescriptorSet*     set         = nullptr;
    };

    struct CompositBuffer
    {

    };


    // レンダーAPI抽象化
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

        //===========================================================
        // Getter
        //===========================================================

        // レンダリング環境
        RenderingContext* GetContext() const;
        RenderingAPI*     GetAPI()     const;

        // コマンドキュー
        QueueID       GetGraphicsQueueID()  const;
        CommandQueue* GetGraphicsCommandQueue() const;

        // フレームデータ
        FrameData&       GetFrameData();
        const FrameData& GetFrameData() const;

        // デバイス情報
        const DeviceInfo& GetDeviceInfo() const;

        //===========================================================
        // API
        //===========================================================

        // テクスチャ
        TextureHandle* CreateTextureFromMemory(const uint8* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);
        TextureHandle* CreateTextureFromMemory(const float* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);

        // レンダーテクスチャ
        TextureHandle* CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap = false);
        TextureHandle* CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap = false);
        TextureHandle* CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap = false);
        void           DestroyTexture(TextureHandle* texture);

        // テクスチャビュー
        TextureView* CreateTextureView(TextureHandle* texture, TextureType type, TextureAspectFlags aspect, uint32 baseArrayLayer = 0, uint32 numArrayLayer = UINT32_MAX, uint32 baseMipLevel = 0, uint32 numMipLevel = UINT32_MAX);
        void         DestroyTextureView(TextureView* view);

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
        DescriptorSet* CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex, DescriptorSetInfo& setInfo);
        void           DestroyDescriptorSet(DescriptorSet* set);

        // スワップチェイン
        SwapChain* CreateSwapChain(Surface* surface, uint32 width, uint32 height, VSyncMode mode);
        bool       ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, VSyncMode mode);
        void       DestoySwapChain(SwapChain* swapchain);
        bool       Present();
        void       BeginSwapChainPass();
        void       EndSwapChainPass();
    
    private:

        //===========================================================
        // ヘルパー
        //===========================================================
        Buffer*        _CreateAndMapBuffer(BufferUsageBits type, const void* data, uint64 dataSize, void** outMappedAdress);
        Buffer*        _CreateAndSubmitBufferData(BufferUsageBits type, const void* data, uint64 dataSize);

        TextureHandle* _CreateTexture(TextureDimension dimension, TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 depth, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags);
        void           _SubmitTextureData(TextureHandle* texture, uint32 width, uint32 height, bool genMipmap, const void* pixelData, uint64 dataSize);
        void           _GenerateMipmaps(CommandBuffer* cmd, TextureHandle* texture, uint32 width, uint32 height, uint32 depth, uint32 array, TextureAspectFlags aspect);

        void _DestroyPendingResources(uint32 frame);

        //===========================================================
        // メンバ
        //===========================================================
        ImmidiateCommandData     immidiateContext = {};
        std::array<FrameData, 2> frameData        = {};
        uint64                   frameIndex       = 0;

        RenderingContext* context = nullptr;
        RenderingAPI*     api     = nullptr;

        QueueID       graphicsQueueID = RENDER_INVALID_ID;
        CommandQueue* graphicsQueue   = nullptr;

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

    public:

        // Gバッファ
        void PrepareGBuffer(uint32 width, uint32 height);
        void ResizeGBuffer(uint32 width, uint32 height);
        void CleanupGBuffer();
        GBuffer gbuffer;

        // ライティング
        void PrepareLightingBuffer(uint32 width, uint32 height);
        void ResizeLightingBuffer(uint32 width, uint32 height);
        void CleanupLightingBuffer();
        LightingBuffer lighting;

        // 環境マップ
        void PrepareEnvironmentBuffer(uint32 width, uint32 height);
        void ResizeEnvironmentBuffer(uint32 width, uint32 height);
        void CleanupEnvironmentBuffer();
        EnvironmentBuffer environment;

        // IBL生成
        void PrepareIBL(const char* environmentTexturePath);
        void CleanupIBL();

        void CreateIrradiance();
        void CreatePrefilter();
        void CreateBRDF();

    public:

        // IBL
        FramebufferHandle* IBLProcessFB          = nullptr;
        RenderPass*        IBLProcessPass        = nullptr;
        Buffer*            equirectangularUBO    = nullptr;
        void*              mappedEquirectanguler = nullptr;

        // キューブマップ変換
        Pipeline*      equirectangularPipeline = nullptr;
        ShaderHandle*  equirectangularShader   = nullptr;
        TextureHandle* cubemapTexture          = nullptr;
        TextureView*   cubemapTextureView      = nullptr;
        DescriptorSet* equirectangularSet      = nullptr;

        // irradiance
        Pipeline*      irradiancePipeline    = nullptr;
        ShaderHandle*  irradianceShader      = nullptr;
        TextureHandle* irradianceTexture     = nullptr;
        TextureView*   irradianceTextureView = nullptr;
        DescriptorSet* irradianceSet         = nullptr;

        // prefilter
        Pipeline*      prefilterPipeline    = nullptr;
        ShaderHandle*  prefilterShader      = nullptr;
        TextureHandle* prefilterTexture     = nullptr;
        TextureView*   prefilterTextureView = nullptr;
        DescriptorSet* prefilterSet         = nullptr;

        // BRDF-LUT
        Pipeline*      brdflutPipeline    = nullptr;
        ShaderHandle*  brdflutShader      = nullptr;
        TextureHandle* brdflutTexture     = nullptr;
        TextureView*   brdflutTextureView = nullptr;

    public:

        // シーンBlit
        FramebufferHandle* compositFB          = nullptr;
        TextureHandle*     compositTexture     = nullptr;
        TextureView*       compositTextureView = nullptr;
        RenderPass*        compositPass        = nullptr;
        ShaderHandle*      compositShader      = nullptr;
        Pipeline*          compositPipeline    = nullptr;
        DescriptorSet*     compositSet         = nullptr;

    public:

        // グリッド
        Buffer*        gridUBO        = nullptr;
        void*          mappedGridData = nullptr;
        ShaderHandle*  gridShader     = nullptr;
        Pipeline*      gridPipeline   = nullptr;
        DescriptorSet* gridSet        = nullptr;

    public:

        // ImGui::Image
        DescriptorSet* imageSet = nullptr;

        // swapchain
        FramebufferHandle* currentSwapchainFramebuffer = nullptr;
        TextureView*       currentSwapchainView        = nullptr;
        RenderPass*        swapchainPass               = nullptr;

    public:

        // Scene
        glm::ivec2 sceneFramebufferSize = {};
        Sampler*   sampler              = nullptr;

        // 頂点レイアウト
        InputLayout defaultLayout;

    public:

        // テクスチャ
        TextureHandle* defaultTexture     = nullptr;
        TextureView*   defaultTextureView = nullptr;

        TextureHandle* envTexture     = nullptr;
        TextureView*   envTextureView = nullptr;
    };
}

