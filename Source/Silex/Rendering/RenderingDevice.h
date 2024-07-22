
#pragma once

#include "Core/TaskQueue.h"
#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;

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
        bool Present();


        SwapChain* CreateSwapChain(Surface* surface, uint32 width, uint32 height, VSyncMode mode);
        bool       ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, VSyncMode mode);
        void       DestoySwapChain(SwapChain* swapchain);

    public:

        RenderingContext* GetContext() const;
        RenderingAPI*     GetAPI()     const;

        QueueFamily   GetGraphicsQueueFamily()  const;
        CommandQueue* GetGraphicsCommandQueue() const;

        FrameData&       GetFrameData();
        const FrameData& GetFrameData() const;


        template<typename Func>
        void AddResourceFreeQueue(const char* taskName, Func&& fn)
        {
            frameData[frameIndex].resourceQueue.Enqueue(taskName, Traits::Forward<Func>(fn));
        }

    public:

        void TEST();
        void DOCK_SPACE(class Camera* camera);
        void DRAW(class Camera* camera);
        void RESIZE(uint32 width, uint32 height);

    public:

        void*              mappedSceneData      = nullptr;
        Buffer*            ubo                  = nullptr;

        // Scene
        glm::ivec2         sceneFramebufferSize = {};
        FramebufferHandle* sceneFramebuffer     = nullptr;
        TextureHandle*     sceneTexture         = nullptr;
        Sampler*           sceneSampler         = nullptr;

        // Swapchain
        FramebufferHandle* swapchainFramebuffer = nullptr;



        // Scene パス
        RenderPass*        scenePass     = nullptr;
        Pipeline*          pipeline      = nullptr;
        ShaderHandle*      shader        = nullptr;
        DescriptorSet*     descriptorSet = nullptr;
        Buffer*            buffer        = nullptr;

        // Swapchain Blit パス
        RenderPass*        swapchainPass = nullptr;
        Pipeline*          blitPipeline  = nullptr;
        ShaderHandle*      blitShader    = nullptr;
        DescriptorSet*     blitSet       = nullptr;


    private:

        std::array<FrameData, 2> frameData  = {};
        uint64                   frameIndex = 0;

        RenderingContext* context = nullptr;
        RenderingAPI*     api     = nullptr;

        QueueFamily   graphicsQueueFamily = INVALID_RENDER_ID;
        CommandQueue* graphicsQueue       = nullptr;

        RenderPassClearValue defaultClearColor = {};

    private:

        static inline RenderingDevice* instance = nullptr;
    };
}

