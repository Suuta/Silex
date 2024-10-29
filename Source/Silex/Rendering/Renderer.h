
#pragma once

#include "Scene/Camera.h"
#include "Rendering/ShaderCompiler.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;

    class Buffer;
    class IndexBuffer;
    class VertexBuffer;
    class UniformBuffer;
    class StorageBuffer;
    class Sampler;
    class Texture;
    class Texture2D;
    class Texture2DArray;
    class TextureCube;
    class TextureView;
    class DescriptorSet;
    class CommandQueue;
    class CommandBuffer;
    class CommandPool;
    class Fence;
    class Semaphore;
    class Surface;
    class Shader;
    class SwapChain;
    class RenderPass;
    class Framebuffer;
    class Pipeline;


    struct GBufferData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        PipelineHandle*    pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;

        Texture2D* albedo   = nullptr;
        Texture2D* normal   = nullptr;
        Texture2D* emission = nullptr;
        Texture2D* id       = nullptr;
        Texture2D* depth    = nullptr;

        TextureViewHandle* albedoView   = nullptr;
        TextureViewHandle* normalView   = nullptr;
        TextureViewHandle* emissionView = nullptr;
        TextureViewHandle* idView       = nullptr;
        TextureViewHandle* depthView    = nullptr;

        UniformBuffer* materialUBO;
        UniformBuffer* transformUBO;

        DescriptorSet* transformSet;
        DescriptorSet* materialSet;
    };

    struct LightingData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        Texture2D*         color       = nullptr;
        TextureViewHandle* view        = nullptr;

        PipelineHandle* pipeline = nullptr;
        ShaderHandle*   shader   = nullptr;

        UniformBuffer* sceneUBO;
        DescriptorSet* set;
    };

    struct EnvironmentData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        TextureHandle*     depth       = nullptr;
        TextureViewHandle* view        = nullptr;
        PipelineHandle*    pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;

        UniformBuffer*     ubo;
        DescriptorSet*     set;
    };

    struct ShadowData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        Texture2DArray*    depth       = nullptr;
        TextureViewHandle* depthView   = nullptr;
        PipelineHandle*    pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;

        UniformBuffer* transformUBO;
        UniformBuffer* lightTransformUBO;
        UniformBuffer* cascadeUBO;
        DescriptorSet* set;
    };

    struct BloomData
    {
        const uint32 numDefaultSampling = 6;
        std::vector<Extent> resolutions = {};

        RenderPassHandle* pass = nullptr;

        std::vector<FramebufferHandle*> samplingFB    = {};
        std::vector<Texture2D*>         sampling      = {};
        std::vector<TextureViewHandle*> samplingView  = {};
        Texture2D*                      prefilter     = {};
        TextureViewHandle*              prefilterView = {};
        Texture2D*                      bloom         = {};
        TextureViewHandle*              bloomView     = {};
        FramebufferHandle*              bloomFB       = {};

        PipelineHandle* prefilterPipeline = nullptr;
        ShaderHandle*   prefilterShader   = nullptr;
        DescriptorSet*  prefilterSet      = nullptr;

        PipelineHandle*             downSamplingPipeline = nullptr;
        ShaderHandle*               downSamplingShader   = nullptr;
        std::vector<DescriptorSet*> downSamplingSet      = {};

        PipelineHandle*             upSamplingPipeline = nullptr;
        ShaderHandle*               upSamplingShader   = nullptr;
        std::vector<DescriptorSet*> upSamplingSet      = {};

        PipelineHandle* bloomPipeline = nullptr;
        ShaderHandle*   bloomShader   = nullptr;
        DescriptorSet*  bloomSet      = nullptr;
    };





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


    // レンダーAPI抽象化
    class Renderer : public Class
    {
        SL_CLASS(Renderer, Class);

    public:

        Renderer();
        ~Renderer();

        // インスタンス
        static Renderer* Get();

        // 初期化
        bool Initialize(RenderingContext* renderingContext, uint32 framesInFlight = 2, uint32 numSwapchainBuffer = 3);

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
        const FrameData& GetFrameData()          const;
        uint32           GetCurrentFrameIndex()  const;
        uint32           GetFrameCountInFlight() const;

        // デバイス情報
        const DeviceInfo& GetDeviceInfo() const;

        //===========================================================
        // API
        //===========================================================

        // テクスチャ
        Texture2D* CreateTextureFromMemory(const uint8* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);
        Texture2D* CreateTextureFromMemory(const float* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);

        // レンダーテクスチャ
        TextureHandle*  CreateTexture(const TextureInfo& info);
        Texture2D*      CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap = false, TextureUsageFlags additionalFlags = 0);
        Texture2DArray* CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap = false, TextureUsageFlags additionalFlags = 0);
        TextureCube*    CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap = false, TextureUsageFlags additionalFlags = 0);
        void            DestroyTexture(Texture* texture);

        // テクスチャビュー
        TextureViewHandle* CreateTextureView(TextureHandle* texture, TextureType type, TextureAspectFlags aspect, uint32 baseArrayLayer = 0, uint32 numArrayLayer = UINT32_MAX, uint32 baseMipLevel = 0, uint32 numMipLevel = UINT32_MAX);
        void               DestroyTextureView(TextureViewHandle* view);

        // バッファ
        Buffer*        CreateBuffer(void* data, uint64 size);
        UniformBuffer* CreateUniformBuffer(void* data, uint64 size);
        BufferHandle*  CreateStorageBuffer(void* data, uint64 size);
        VertexBuffer*  CreateVertexBuffer(void* data, uint64 size);
        IndexBuffer*   CreateIndexBuffer(void* data, uint64 size);
        void           DestroyBuffer(Buffer* buffer);
        bool           UpdateBufferData(BufferHandle* buffer, const void* data, uint32 dataByte);

        // フレームバッファ
        FramebufferHandle* CreateFramebuffer(RenderPassHandle* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height);
        void               DestroyFramebuffer(FramebufferHandle* framebuffer);

        // デスクリプターセット
        DescriptorSet* CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex);
        void           DestroyDescriptorSet(DescriptorSet* set);
        void           UpdateDescriptorSet(DescriptorSetHandle* set, DescriptorSetInfo& setInfo);

        // スワップチェイン
        SwapChainHandle* CreateSwapChain(SurfaceHandle* surface, uint32 width, uint32 height, VSyncMode mode);
        bool             ResizeSwapChain(SwapChainHandle* swapchain, uint32 width, uint32 height, VSyncMode mode);
        void             DestoySwapChain(SwapChainHandle* swapchain);
        bool             Present();
        void             BeginSwapChainPass();
        void             EndSwapChainPass();

        // 即時コマンド
        void ImmidiateExcute(std::function<void(CommandBufferHandle*)>&& func);
    
    private:

        BufferHandle* _CreateAndMapBuffer(BufferUsageFlags type, const void* data, uint64 dataSize, void** outMappedPtr);
        BufferHandle* _CreateAndSubmitBufferData(BufferUsageFlags type, const void* data, uint64 dataSize);

        TextureHandle* _CreateTexture(TextureDimension dimension, TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 depth, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags);
        void           _SubmitTextureData(TextureHandle* texture, uint32 width, uint32 height, bool genMipmap, const void* pixelData, uint64 dataSize);
        void           _GenerateMipmaps(CommandBufferHandle* cmd, TextureHandle* texture, uint32 width, uint32 height, uint32 depth, uint32 array, TextureAspectFlags aspect);

        // フレームデータ
        ImmidiateCommandData   immidiateContext = {};
        std::vector<FrameData> frameData        = {};
        uint64                 frameIndex       = 0;

        // 固有APIレイヤー
        RenderingContext* context = nullptr;
        RenderingAPI*     api     = nullptr;

        // キュー
        QueueID             graphicsQueueID = RENDER_INVALID_ID;
        CommandQueueHandle* graphicsQueue   = nullptr;

        // 定数
        uint32 numSwapchainFrameBuffer = 3;
        uint32 numFramesInFlight       = 2;

        // リソース解放処理
        void _DestroyPendingResources(uint32 frame);

        //===========================================================
        // インスタンス
        //===========================================================
        static inline Renderer* instance = nullptr;




    public:

        //===========================================================
        //===========================================================
        // テストコード
        //===========================================================
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
        Buffer* pixelIDBuffer = nullptr;

    public:

        // IBL
        FramebufferHandle* IBLProcessFB       = nullptr;
        RenderPassHandle*  IBLProcessPass     = nullptr;
        UniformBuffer*     equirectangularUBO = nullptr;

        // キューブマップ変換
        PipelineHandle*    equirectangularPipeline = nullptr;
        ShaderHandle*      equirectangularShader   = nullptr;
        TextureCube*       cubemapTexture          = nullptr;
        TextureViewHandle* cubemapTextureView      = nullptr;
        DescriptorSet*     equirectangularSet      = nullptr;

        // irradiance
        PipelineHandle*    irradiancePipeline    = nullptr;
        ShaderHandle*      irradianceShader      = nullptr;
        TextureCube*       irradianceTexture     = nullptr;
        TextureViewHandle* irradianceTextureView = nullptr;
        DescriptorSet*     irradianceSet         = nullptr;

        // prefilter
        PipelineHandle*    prefilterPipeline    = nullptr;
        ShaderHandle*      prefilterShader      = nullptr;
        TextureCube*       prefilterTexture     = nullptr;
        TextureViewHandle* prefilterTextureView = nullptr;
        DescriptorSet*     prefilterSet         = nullptr;

        // BRDF-LUT
        PipelineHandle*    brdflutPipeline    = nullptr;
        ShaderHandle*      brdflutShader      = nullptr;
        Texture2D*         brdflutTexture     = nullptr;
        TextureViewHandle* brdflutTextureView = nullptr;

    public:

        // シーンBlit
        FramebufferHandle* compositeFB          = nullptr;
        Texture2D*         compositeTexture     = nullptr;
        TextureViewHandle* compositeTextureView = nullptr;
        RenderPassHandle*  compositePass        = nullptr;
        ShaderHandle*      compositeShader      = nullptr;
        PipelineHandle*    compositePipeline    = nullptr;
        DescriptorSet*     compositeSet;

    public:

        // グリッド
        ShaderHandle*   gridShader   = nullptr;
        PipelineHandle* gridPipeline = nullptr;
        DescriptorSet*  gridSet;
        UniformBuffer*  gridUBO;

    public:

        // ImGui::Image
        DescriptorSet* imageSet;

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
        Texture2D*         defaultTexture     = nullptr;
        TextureViewHandle* defaultTextureView = nullptr;

        Texture2D*         envTexture     = nullptr;
        TextureViewHandle* envTextureView = nullptr;
    };
}

