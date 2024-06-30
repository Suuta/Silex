
#include "PCH.h"

#include "Core/Window.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/ShaderCompiler.h"


namespace Silex
{
    //======================================
    // シェーダーコンパイラ
    //======================================
    ShaderCompiler shaserCompiler;



    RenderingDevice* RenderingDevice::Get()
    {
        return instance;
    }

    RenderingDevice::RenderingDevice()
    {
        instance = this;
    }

    RenderingDevice::~RenderingDevice()
    {
        

        instance = nullptr;
    }

    bool RenderingDevice::Initialize(RenderingContext* context)
    {
        renderingContext = context;

        // レンダーAPI実装クラスを生成
        api = renderingContext->CreateRendringAPI();
        SL_CHECK(!api->Initialize(), false);
       
        // グラフィックスをサポートするキューファミリを取得
        graphicsQueueFamily = api->QueryQueueFamily(QUEUE_FAMILY_GRAPHICS_BIT, Window::Get()->GetSurface());
        SL_CHECK(graphicsQueueFamily == INVALID_RENDER_ID, false);

        // コマンドキュー生成
        graphicsQueue = api->CreateCommandQueue(graphicsQueueFamily);
        SL_CHECK(!graphicsQueue, false);
      
        // フレームデータ生成
        frameData.resize(2);
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            // コマンドプール生成
            frameData[i].commandPool = api->CreateCommandPool(graphicsQueueFamily, COMMAND_BUFFER_TYPE_PRIMARY);
            SL_CHECK(!frameData[i].commandPool, false);

            // コマンドバッファ生成
            frameData[i].commandBuffer = api->CreateCommandBuffer(frameData[i].commandPool);
            SL_CHECK(!frameData[i].commandBuffer, false);

            // セマフォ生成
            frameData[i].presentSemaphore = api->CreateSemaphore();
            SL_CHECK(!frameData[i].presentSemaphore, false);

            frameData[i].renderSemaphore = api->CreateSemaphore();
            SL_CHECK(!frameData[i].renderSemaphore, false);

            // フェンス生成
            frameData[i].fence = api->CreateFence();
            SL_CHECK(!frameData[i].fence, false);
        }

        //----------------------------------------
        // VERTEX
        //----------------------------------------
        //  (set: 0, bind: 0) uniform SceneData
        //  (set: 1, bind: 0) uniform GLTFMaterialData
        //   push_constant: Constants
        //----------------------------------------
        // FRAGMENT
        //----------------------------------------
        //  (set: 0, bind: 0) uniform       SceneData
        //  (set: 1, bind: 1) sampled_image colorTex
        //  (set: 1, bind: 2) sampled_image metalRoughTex


        //----------------------------------------------
        // (set: 0, bind: 0) uniform SceneData
        //----------------------------------------------
        // (set: 1, bind: 0) uniform       GLTFMaterialData
        // (set: 1, bind: 1) sampled_image colorTex
        // (set: 1, bind: 2) sampled_image metalRoughTex
        //----------------------------------------------
        //  push_constant: constants
        //----------------------------------------------

#if 0
        struct SceneData
        {
            glm::mat4 view;
            glm::mat4 proj;
            glm::mat4 viewproj;
            glm::vec4 ambientColor;
            glm::vec4 sunlightDirection;
            glm::vec4 sunlightColor;
        };

        SceneData sd = {};

        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/mesh.glsl", compiledData);

        ShaderHandle* shader  = api->CreateShader(compiledData);
        Buffer*       uniform = api->CreateBuffer(sizeof(SceneData), BufferUsageBits(BUFFER_USAGE_UNIFORM_BIT | BUFFER_USAGE_TRANSFER_DST_BIT), MEMORY_ALLOCATION_TYPE_CPU);

        DescriptorInfo info = {};
        info.binding = 0;
        info.type    = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        info.buffer  = uniform;

        DescriptorSet* set = api->CreateDescriptorSet(1, &info, shader, 0);

        byte* mapped = api->MapBuffer(uniform);
        memcpy(mapped, &sd, sizeof(SceneData));
        api->UnmapBuffer(uniform);

        api->DestroyDescriptorSet(set);
        api->DestroyShader(shader);
        api->DestroyBuffer(uniform);
#endif

        //======================================================================================================
        // TODO: DescriptorInfo を生成するクラスを実装する ???
        //======================================================================================================
        // デスクリプターセットレイアウトと一対一であるリフレクションデータを使って、デスクリプターセットに必要なイメージやバッファ
        // を受け取って、Infoを生成する。結局のところ、更新するイメージバッファ数がわかっても、実データが用意できるまで更新できないので
        // CreateDescriptorSet関数は、実データの用意と同タイミングで行う様にする
        
        //====================================================================
        // ????? マテリアル生成毎に互換性のあるセットが再生成されるのは仕方ないのか ?????
        //--------------------------------------------------------------------
        // 良くなさそうなら、プールと同様に、セットのハッシュキーで管理すればよさそう？
        //====================================================================

        return true;
    }

    void RenderingDevice::Finalize()
    {
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            api->DestroyCommandPool(frameData[i].commandPool);
            api->DestroySemaphore(frameData[i].presentSemaphore);
            api->DestroySemaphore(frameData[i].renderSemaphore);
            api->DestroyFence(frameData[i].fence);
        }

        api->DestroyCommandQueue(graphicsQueue);
        renderingContext->DestroyRendringAPI(api);
    }


    SwapChain* RenderingDevice::CreateSwapChain(Surface* surface)
    {
        SwapChain* swapchain = nullptr;
        bool result = false;

        swapchain = api->CreateSwapChain(surface);
        SL_CHECK(!swapchain, nullptr);

        result = api->ResizeSwapChain(swapchain, swapchainBufferCount, VSYNC_MODE_DISABLED);
        SL_CHECK(!result, nullptr);

        return swapchain;
    }

    void RenderingDevice::DestoySwapChain(SwapChain* swapchain)
    {
        if (swapchain)
        {
            api->DestroySwapChain(swapchain);
        }
    }
}

