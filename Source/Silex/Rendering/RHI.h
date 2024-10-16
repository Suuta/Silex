
#pragma once

#include "Core/TaskQueue.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/RHIStructures.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;

    // 削除待機リソース
    struct PendingDestroyResourceQueue
    {
        std::vector<BufferHandle*>        buffer;
        std::vector<TextureHandle*>       texture;
        std::vector<TextureViewHandle*>   textureView;
        std::vector<SamplerHandle*>       sampler;
        std::vector<DescriptorSetHandle*> descriptorset;
        std::vector<FramebufferHandle*>   framebuffer;
        std::vector<ShaderHandle*>        shader;
        std::vector<PipelineHandle*>      pipeline;
    };

    // フレームデータ
    struct FrameData
    {
        CommandPoolHandle*           commandPool      = nullptr;
        CommandBufferHandle*         commandBuffer    = nullptr;
        SemaphoreHandle*             presentSemaphore = nullptr;
        SemaphoreHandle*             renderSemaphore  = nullptr;
        FenceHandle*                 fence            = nullptr;
        bool                         waitingSignal    = false;
        PendingDestroyResourceQueue* pendingResources;
    };

    // 即時コマンドデータ
    struct ImmidiateCommandData
    {
        CommandPoolHandle*   commandPool   = nullptr;
        CommandBufferHandle* commandBuffer = nullptr;
        FenceHandle*         fence         = nullptr;
    };




    struct GBufferData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        PipelineHandle*    pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;

        TextureHandle* albedo   = nullptr;
        TextureHandle* normal   = nullptr;
        TextureHandle* emission = nullptr;
        TextureHandle* id       = nullptr;
        TextureHandle* depth    = nullptr;

        TextureViewHandle* albedoView   = nullptr;
        TextureViewHandle* normalView   = nullptr;
        TextureViewHandle* emissionView = nullptr;
        TextureViewHandle* idView       = nullptr;
        TextureViewHandle* depthView    = nullptr;

        BufferHandle* materialUBO[2];
        BufferHandle* transformUBO[2];

        DescriptorSetHandle* transformSet[2];
        DescriptorSetHandle* materialSet[2];
    };

    struct LightingData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        TextureHandle*     color       = nullptr;
        TextureViewHandle* view        = nullptr;

        PipelineHandle* pipeline = nullptr;
        ShaderHandle*   shader   = nullptr;

        BufferHandle*        sceneUBO[2];
        DescriptorSetHandle* set[2];
    };

    struct EnvironmentData
    {
        RenderPassHandle*    pass        = nullptr;
        FramebufferHandle*   framebuffer = nullptr;
        TextureHandle*       depth       = nullptr;
        TextureViewHandle*   view        = nullptr;
        PipelineHandle*      pipeline    = nullptr;
        ShaderHandle*        shader      = nullptr;
        BufferHandle*        ubo[2];
        DescriptorSetHandle* set[2];
    };

    struct ShadowData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        TextureHandle*     depth       = nullptr;
        TextureViewHandle* depthView   = nullptr;
        PipelineHandle*    pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;

        BufferHandle*        transformUBO[2];
        BufferHandle*        lightTransformUBO[2];
        BufferHandle*        cascadeUBO[2];
        DescriptorSetHandle* set[2];
    };

    struct BloomData
    {
        const uint32 numDefaultSampling = 6;
        std::vector<Extent> resolutions = {};

        RenderPassHandle* pass = nullptr;

        std::vector<FramebufferHandle*> samplingFB    = {};
        std::vector<TextureHandle*>     sampling      = {};
        std::vector<TextureViewHandle*> samplingView  = {};
        TextureHandle*                  prefilter     = {};
        TextureViewHandle*              prefilterView = {};
        TextureHandle*                  bloom         = {};
        TextureViewHandle*              bloomView     = {};
        FramebufferHandle*              bloomFB       = {};

        PipelineHandle*      prefilterPipeline = nullptr;
        ShaderHandle*        prefilterShader   = nullptr;
        DescriptorSetHandle* prefilterSet      = nullptr;

        PipelineHandle*                   downSamplingPipeline = nullptr;
        ShaderHandle*                     downSamplingShader   = nullptr;
        std::vector<DescriptorSetHandle*> downSamplingSet      = {};

        PipelineHandle*                   upSamplingPipeline = nullptr;
        ShaderHandle*                     upSamplingShader   = nullptr;
        std::vector<DescriptorSetHandle*> upSamplingSet      = {};

        PipelineHandle*      bloomPipeline = nullptr;
        ShaderHandle*        bloomShader   = nullptr;
        DescriptorSetHandle* bloomSet      = nullptr;
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

        // レンダリングコンテキスト
        RenderingContext* GetContext() const;
        RenderingAPI*     GetAPI()     const;

        // コマンドキュー
        QueueID             GetGraphicsQueueID()      const;
        CommandQueueHandle* GetGraphicsCommandQueue() const;

        // フレームデータ
        const FrameData& GetFrameData()         const;
        uint32           GetCurrentFrameIndex() const;

        // デバイス情報
        const DeviceInfo& GetDeviceInfo() const;

        //===========================================================
        // API
        //===========================================================

        // テクスチャ
        TextureHandle* CreateTextureFromMemory(const uint8* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);
        TextureHandle* CreateTextureFromMemory(const float* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);

        // レンダーテクスチャ
        TextureHandle* CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap = false, TextureUsageFlags additionalFlags = 0);
        TextureHandle* CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap = false, TextureUsageFlags additionalFlags = 0);
        TextureHandle* CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap = false, TextureUsageFlags additionalFlags = 0);
        void           DestroyTexture(TextureHandle* texture);

        // テクスチャビュー
        TextureViewHandle* CreateTextureView(TextureHandle* texture, TextureType type, TextureAspectFlags aspect, uint32 baseArrayLayer = 0, uint32 numArrayLayer = UINT32_MAX, uint32 baseMipLevel = 0, uint32 numMipLevel = UINT32_MAX);
        void               DestroyTextureView(TextureViewHandle* view);

        // バッファ
        BufferHandle* CreateUniformBuffer(void* data, uint64 size);
        BufferHandle* CreateStorageBuffer(void* data, uint64 size);
        BufferHandle* CreateVertexBuffer(void* data, uint64 size);
        BufferHandle* CreateIndexBuffer(void* data, uint64 size);
        void          DestroyBuffer(BufferHandle* buffer);
        bool          UpdateBufferData(BufferHandle* buffer, const void* data, uint32 dataByte);

        // フレームバッファ
        FramebufferHandle* CreateFramebuffer(RenderPassHandle* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height);
        void               DestroyFramebuffer(FramebufferHandle* framebuffer);

        // デスクリプターセット
        DescriptorSetHandle* CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex);
        void                 UpdateDescriptorSet(DescriptorSetHandle* set, DescriptorSetInfo& setInfo);
        void                 DestroyDescriptorSet(DescriptorSetHandle* set);

        // スワップチェイン
        SwapChainHandle* CreateSwapChain(SurfaceHandle* surface, uint32 width, uint32 height, VSyncMode mode);
        bool             ResizeSwapChain(SwapChainHandle* swapchain, uint32 width, uint32 height, VSyncMode mode);
        void             DestoySwapChain(SwapChainHandle* swapchain);
        bool             Present();
        void             BeginSwapChainPass();
        void             EndSwapChainPass();
    
    private:

        BufferHandle* _CreateAndMapBuffer(BufferUsageFlags type, const void* data, uint64 dataSize, void** outMappedPtr);
        BufferHandle* _CreateAndSubmitBufferData(BufferUsageFlags type, const void* data, uint64 dataSize);

        TextureHandle* _CreateTexture(TextureDimension dimension, TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 depth, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags);
        void           _SubmitTextureData(TextureHandle* texture, uint32 width, uint32 height, bool genMipmap, const void* pixelData, uint64 dataSize);
        void           _GenerateMipmaps(CommandBufferHandle* cmd, TextureHandle* texture, uint32 width, uint32 height, uint32 depth, uint32 array, TextureAspectFlags aspect);

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
        CommandQueueHandle* graphicsQueue   = nullptr;

        //===========================================================
        // インスタンス
        //===========================================================
        static inline RHI* instance = nullptr;


    public:

        //===========================================================
        // テストコード
        //===========================================================

        void TEST();
        void Update(class Camera* camera);
        void Render(float dt);
        void RESIZE(uint32 width, uint32 height);
        void UpdateUBO(class Camera* camera);

    public:

        // ビューポートサイズ
        glm::ivec2 sceneFramebufferSize = {};
        glm::ivec2 cameraFramebufferSize = {};

        // Gバッファ
        void PrepareGBuffer(uint32 width, uint32 height);
        void ResizeGBuffer(uint32 width, uint32 height);
        void CleanupGBuffer();
        GBufferData* gbuffer;

        // ライティング
        void PrepareLightingBuffer(uint32 width, uint32 height);
        void ResizeLightingBuffer(uint32 width, uint32 height);
        void CleanupLightingBuffer();
        LightingData lighting;

        // 環境マップ
        void PrepareEnvironmentBuffer(uint32 width, uint32 height);
        void ResizeEnvironmentBuffer(uint32 width, uint32 height);
        void CleanupEnvironmentBuffer();
        EnvironmentData environment;

        // IBL生成
        void PrepareIBL(const char* environmentTexturePath);
        void CleanupIBL();
        void CreateIrradiance();
        void CreatePrefilter();
        void CreateBRDF();

        // シャドウマップ
        void PrepareShadowBuffer();
        void CleanupShadowBuffer();
        void CalculateLightSapceMatrices(glm::vec3 directionalLightDir, Camera* camera, std::array<glm::mat4, 4>& out_result);
        void GetFrustumCornersWorldSpace(const glm::mat4& projview, std::array<glm::vec4, 8>& out_result);
        glm::mat4 GetLightSpaceMatrix(glm::vec3 directionalLightDir, Camera* camera, const float nearPlane, const float farPlane);
        ShadowData shadow;

        // ブルーム
        std::vector<Extent> CalculateBlomSampling(uint32 width, uint32 height);
        void PrepareBloomBuffer(uint32 width, uint32 height);
        void ResizeBloomBuffer(uint32 width, uint32 height);
        void CleanupBloomBuffer();
        BloomData* bloom;

        // オブジェクトID リードバック
        int32 ReadPixelObjectID(uint32 x, uint32 y);
        BufferHandle* pixelIDBuffer = nullptr;
        void*         mappedPixelID = nullptr;

    public:

        // IBL
        FramebufferHandle* IBLProcessFB       = nullptr;
        RenderPassHandle*  IBLProcessPass     = nullptr;
        BufferHandle*      equirectangularUBO = nullptr;

        // キューブマップ変換
        PipelineHandle*      equirectangularPipeline = nullptr;
        ShaderHandle*        equirectangularShader   = nullptr;
        TextureHandle*       cubemapTexture          = nullptr;
        TextureViewHandle*   cubemapTextureView      = nullptr;
        DescriptorSetHandle* equirectangularSet      = nullptr;

        // irradiance
        PipelineHandle*      irradiancePipeline    = nullptr;
        ShaderHandle*        irradianceShader      = nullptr;
        TextureHandle*       irradianceTexture     = nullptr;
        TextureViewHandle*   irradianceTextureView = nullptr;
        DescriptorSetHandle* irradianceSet         = nullptr;

        // prefilter
        PipelineHandle*      prefilterPipeline    = nullptr;
        ShaderHandle*        prefilterShader      = nullptr;
        TextureHandle*       prefilterTexture     = nullptr;
        TextureViewHandle*   prefilterTextureView = nullptr;
        DescriptorSetHandle* prefilterSet         = nullptr;

        // BRDF-LUT
        PipelineHandle*    brdflutPipeline    = nullptr;
        ShaderHandle*      brdflutShader      = nullptr;
        TextureHandle*     brdflutTexture     = nullptr;
        TextureViewHandle* brdflutTextureView = nullptr;

    public:

        // シーンBlit
        FramebufferHandle*   compositeFB          = nullptr;
        TextureHandle*       compositeTexture     = nullptr;
        TextureViewHandle*   compositeTextureView = nullptr;
        RenderPassHandle*    compositePass        = nullptr;
        ShaderHandle*        compositeShader      = nullptr;
        PipelineHandle*      compositePipeline    = nullptr;
        DescriptorSetHandle* compositeSet[2];

    public:

        // グリッド
        ShaderHandle*        gridShader   = nullptr;
        PipelineHandle*      gridPipeline = nullptr;
        DescriptorSetHandle* gridSet;
        BufferHandle*        gridUBO;

    public:

        // ImGui::Image
        DescriptorSetHandle* imageSet[2];

        // swapchain
        FramebufferHandle* currentSwapchainFramebuffer = nullptr;
        TextureViewHandle*       currentSwapchainView        = nullptr;
        RenderPassHandle*        swapchainPass               = nullptr;

    public:

        // サンプラー
        SamplerHandle*   linearSampler = nullptr;
        SamplerHandle*   shadowSampler = nullptr;

        // 頂点レイアウト
        InputLayout defaultLayout;

        // ライト
        glm::vec3 sceneLightDir = { 0.5, 0.7, 0.1 };

    public:

        // テクスチャ
        TextureHandle* defaultTexture     = nullptr;
        TextureViewHandle*   defaultTextureView = nullptr;

        TextureHandle* envTexture     = nullptr;
        TextureViewHandle*   envTextureView = nullptr;
    };
}

