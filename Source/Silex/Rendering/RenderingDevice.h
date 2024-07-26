
#pragma once

#include "Core/TaskQueue.h"
#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;
    class Mesh;


    // TODO: エディターの設定項目として扱える形にする
    const uint32 swapchainBufferCount = 3;

    // フレームデータ
    struct FrameData
    {
        CommandPool*   commandPool      = nullptr;
        CommandBuffer* commandBuffer    = nullptr;
        Semaphore*     presentSemaphore = nullptr;
        Semaphore*     renderSemaphore  = nullptr;
        Fence*         fence            = nullptr;

        TaskQueue resourceQueue;
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

        // リソース解放キュー
        template<typename Func>
        void AddResourceFreeQueue(const char* taskName, Func&& fn)
        {
            frameData[frameIndex].resourceQueue.Enqueue(taskName, Traits::Forward<Func>(fn));
        }

    public:

        // デバイス情報
        const DeviceInfo& GetDeviceInfo() const;

        // テクスチャ
        TextureHandle* LoadTextureFromFile(const byte* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap);

        // レンダーテクスチャ
        TextureHandle* CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap);
        TextureHandle* CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap);
        TextureHandle* CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap);

        // バッファ
        Buffer* CreateVertexBuffer(void* data, uint64 dataByte);
        Buffer* CreateIndexBuffer(void* data, uint64 dataByte);
        void    DestroyBuffer(Buffer* buffer);

        // スワップチェイン
        SwapChain* CreateSwapChain(Surface* surface, uint32 width, uint32 height, VSyncMode mode);
        bool       ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, VSyncMode mode);
        void       DestoySwapChain(SwapChain* swapchain);
        bool       Present();
    
    private:

        void           GenerateMipmaps(CommandBuffer* cmd, TextureHandle* texture, uint32 width, uint32 height);
        TextureHandle* CreateTexture(TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 array, uint32 depth, bool genMipmap, TextureUsageFlags additionalFlags);

    public:

        void TEST();
        void DOCK_SPACE(class Camera* camera);
        void DRAW(class Camera* camera);
        void RESIZE(uint32 width, uint32 height);

    private:

        void*              mappedSceneData      = nullptr;
        Buffer*            ubo                  = nullptr;

        // Scene
        glm::ivec2         sceneFramebufferSize = {};
        FramebufferHandle* sceneFramebuffer     = nullptr;
        TextureHandle*     sceneColorTexture    = nullptr;
        TextureHandle*     sceneDepthTexture    = nullptr;
        Sampler*           sceneSampler         = nullptr;

        // Swapchain
        FramebufferHandle* swapchainFramebuffer = nullptr;

        // Scene パス
        RenderPass*        scenePass     = nullptr;
        Pipeline*          pipeline      = nullptr;
        ShaderHandle*      shader        = nullptr;
        DescriptorSet*     descriptorSet = nullptr;
        DescriptorSet*     textureSet    = nullptr;

        // Swapchain Blit パス
        RenderPass*        swapchainPass = nullptr;
        Pipeline*          blitPipeline  = nullptr;
        ShaderHandle*      blitShader    = nullptr;
        DescriptorSet*     blitSet       = nullptr;


        Buffer* vb = nullptr;
        Buffer* ib = nullptr;

        Mesh* cubeMesh   = nullptr;
        Mesh* sphereMesh = nullptr;

    private:

        ImmidiateCommandData     immidiateContext = {};
        std::array<FrameData, 2> frameData        = {};
        uint64                   frameIndex       = 0;

        RenderingContext* context = nullptr;
        RenderingAPI*     api     = nullptr;

        QueueFamily   graphicsQueueFamily = INVALID_RENDER_ID;
        CommandQueue* graphicsQueue       = nullptr;

        RenderPassClearValue defaultClearColor        = {};
        RenderPassClearValue defaultClearDepthStencil = {};

    private:

        static inline RenderingDevice* instance = nullptr;
    };
}

