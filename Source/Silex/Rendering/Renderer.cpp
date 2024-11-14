
#include "PCH.h"

#include "Core/Window.h"
#include "Core/Engine.h"
#include "Asset/TextureReader.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/RenderingUtility.h"
#include "Rendering/Mesh.h"

#include <imgui/imgui_internal.h>
#include <imgui/imgui.h>
#include <cmath>


namespace Silex
{
    Mesh* cubeMesh   = nullptr;
    Mesh* sponzaMesh = nullptr;

    // シャドウマップ
    static const uint32  shadowMapResolution = 2048;
    std::array<float, 4> shadowCascadeLevels = { 10.0f, 40.0f, 100.0f, 200.0f };
    int32 debugSleep = 0;

    namespace Test
    {
        struct PrifilterParam
        {
            float roughness;
        };

        struct Transform
        {
            glm::mat4 world      = glm::mat4(1.0f);
            glm::mat4 view       = glm::mat4(1.0f);
            glm::mat4 projection = glm::mat4(1.0f);
        };

        struct ShadowTransformData
        {
            glm::mat4 world = glm::mat4(1.0f);
        };

        struct LightSpaceTransformData
        {
            glm::mat4 cascade[4];
        };

        struct CascadeData
        {
            glm::vec4 cascadePlaneDistances[4];
        };

        struct GridData
        {
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec4 pos;
        };

        struct Light
        {
            glm::vec4 directionalLight;
            glm::vec4 cameraPos;
        };

        struct EquirectangularData
        {
            glm::mat4 view[6];
            glm::mat4 projection;
        };

        struct MaterialUBO
        {
            glm::vec3 albedo;
            float     metallic;
            glm::vec3 emission;
            float     roughness;
            glm::vec2 textureTiling;
        };

        struct SceneUBO
        {
            glm::vec4 lightDir;
            glm::vec4 lightColor;
            glm::vec4 cameraPosition;
            glm::mat4 view;
            glm::mat4 invViewProjection;
        };

        struct EnvironmentUBO
        {
            glm::mat4 view;
            glm::mat4 projection;
        };

        struct CompositUBO
        {
            float exposure;
        };


        struct BloomPrefilterData
        {
            float threshold;
        };

        struct BloomDownSamplingData
        {
            glm::vec2 sourceResolution;
        };

        struct BloomUpSamplingData
        {
            float filterRadius;
        };

        struct BloomData
        {
            float intencity;
        };
    }

    //======================================
    // 定数
    //======================================
    // TODO: エディターの設定項目として扱える形にする
    const uint32 swapchainBufferCount = 3;




    Renderer* Renderer::Get()
    {
        return instance;
    }

    Renderer::Renderer()
    {
        instance = this;
    }

    Renderer::~Renderer()
    {
        api->WaitDevice();

        sldelete(cubeMesh);
        sldelete(sponzaMesh);

        CleanupIBL();
        CleanupShadowBuffer();
        CleanupGBuffer();
        CleanupLightingBuffer();
        CleanupEnvironmentBuffer();
        CleanupBloomBuffer();

        sldelete(gbuffer);
        sldelete(bloom);

        DestroyBuffer(gridUBO);
        DestroyBuffer(pixelIDBuffer);

        DestroyTexture(defaultTexture);
        DestroyTexture(compositeTexture);

        DestroyTextureView(defaultTextureView);
        DestroyTextureView(compositeTextureView);
        api->DestroyShader(gridShader);
        api->DestroyShader(compositeShader);

        DestroyDescriptorSet(gridSet);
        DestroyDescriptorSet(compositeSet);
        DestroyDescriptorSet(imageSet);

        api->DestroyPipeline(gridPipeline);
        api->DestroyPipeline(compositePipeline);
        api->DestroyRenderPass(compositePass);
        api->DestroyFramebuffer(compositeFB);
        DestroySampler(linearSampler);
        DestroySampler(shadowSampler);

        for (uint32 i = 0; i < frameData.size(); i++)
        {
            _DestroyPendingResources(i);

            api->DestroyCommandBuffer(frameData[i].commandBuffer);
            api->DestroyCommandPool(frameData[i].commandPool);
            api->DestroySemaphore(frameData[i].presentSemaphore);
            api->DestroySemaphore(frameData[i].renderSemaphore);
            api->DestroyFence(frameData[i].fence);

            sldelete(frameData[i].pendingResources);
        }

        api->DestroyCommandBuffer(immidiateContext.commandBuffer);
        api->DestroyCommandPool(immidiateContext.commandPool);
        api->DestroyFence(immidiateContext.fence);
        api->DestroyCommandQueue(graphicsQueue);

        context->DestroyRendringAPI(api);

        instance = nullptr;
    }

    bool Renderer::Initialize(RenderingContext* renderingContext, uint32 framesInFlight, uint32 numSwapchainBuffer)
    {
        context                 = renderingContext;
        numFramesInFlight       = framesInFlight;
        numSwapchainFrameBuffer = numSwapchainBuffer;

        // レンダーAPI実装クラスを生成
        api = context->CreateRendringAPI();
        SL_CHECK(!api->Initialize(), false);
       
        // グラフィックスをサポートするキューファミリを取得
        graphicsQueueID = api->QueryQueueID(QUEUE_FAMILY_GRAPHICS_BIT, Window::Get()->GetSurface());
        SL_CHECK(graphicsQueueID == RENDER_INVALID_ID, false);

        // コマンドキュー生成
        graphicsQueue = api->CreateCommandQueue(graphicsQueueID);
        SL_CHECK(!graphicsQueue, false);
      
        // フレームデータ生成
        frameData.resize(numFramesInFlight);
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            frameData[i].pendingResources = slnew(PendingDestroyResourceQueue);

            // コマンドプール生成
            frameData[i].commandPool = api->CreateCommandPool(graphicsQueueID);
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

        // 即時コマンドデータ
        immidiateContext.commandPool = api->CreateCommandPool(graphicsQueueID);
        SL_CHECK(!immidiateContext.commandPool, false);

        immidiateContext.commandBuffer = api->CreateCommandBuffer(immidiateContext.commandPool);
        SL_CHECK(!immidiateContext.commandBuffer, false);

        immidiateContext.fence = api->CreateFence();
        SL_CHECK(!immidiateContext.fence, false);

        return true;
    }

    bool Renderer::BeginFrame()
    {
        bool result = false;
        FrameData& frame = frameData[frameIndex];

        // GPU 待機
        if (frameData[frameIndex].waitingSignal)
        {
            SL_SCOPE_PROFILE("Renderer::BeginFrame")

            result = api->WaitFence(frame.fence);
            SL_CHECK(!result, false);

            frameData[frameIndex].waitingSignal = false;
        }

        // 削除キュー実行
        _DestroyPendingResources(frameIndex);

        // 描画先スワップチェインバッファを取得
        auto [fb, view] = api->GetCurrentBackBuffer(Window::Get()->GetSwapChain(), frame.presentSemaphore);
        currentSwapchainFramebuffer = fb;
        currentSwapchainView        = view;

        return true;
    }

    bool Renderer::EndFrame()
    {
        bool result = false;

        FrameData& frame = frameData[frameIndex];
        result = api->EndCommandBuffer(frame.commandBuffer);

        result = api->SubmitQueue(graphicsQueue, frame.commandBuffer, frame.fence, frame.presentSemaphore, frame.renderSemaphore);
        frameData[frameIndex].waitingSignal = true;

        return result;
    }

    void Renderer::TEST()
    {
        auto size = Window::Get()->GetSize();
        swapchainPass = api->GetSwapChainRenderPass(Window::Get()->GetSwapChain());

        cubeMesh   = MeshFactory::Cube();
        sponzaMesh = MeshFactory::Sponza();

        defaultLayout.Binding(0);
        defaultLayout.Attribute(0, VERTEX_BUFFER_FORMAT_R32G32B32);
        defaultLayout.Attribute(1, VERTEX_BUFFER_FORMAT_R32G32B32);
        defaultLayout.Attribute(2, VERTEX_BUFFER_FORMAT_R32G32);
        defaultLayout.Attribute(3, VERTEX_BUFFER_FORMAT_R32G32B32);
        defaultLayout.Attribute(4, VERTEX_BUFFER_FORMAT_R32G32B32);

        linearSampler = CreateSampler(SAMPLER_FILTER_LINEAR, SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE);
        shadowSampler = CreateSampler(SAMPLER_FILTER_LINEAR, SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE, true, COMPARE_OP_LESS_OR_EQUAL);

        TextureReader reader;
        byte* pixels;

        pixels = reader.Read("Assets/Textures/default.png");
        defaultTexture     = CreateTextureFromMemory(pixels, reader.data.byteSize, reader.data.width, reader.data.height, true);
        defaultTextureView = CreateTextureView(defaultTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        reader.Unload(pixels);

        // ID リードバック
        pixelIDBuffer = CreateBuffer(nullptr, sizeof(int32));

        // IBL
        PrepareIBL("Assets/Textures/cloud.png");

        // シャドウマップ
        PrepareShadowBuffer();

        // Gバッファ
        gbuffer = slnew(GBufferData);
        PrepareGBuffer(size.x, size.y);

        // ライティングバッファ
        PrepareLightingBuffer(size.x, size.y);

        // 環境マップ
        PrepareEnvironmentBuffer(size.x, size.y);

        // ブルーム
        bloom = slnew(BloomData);
        PrepareBloomBuffer(size.x, size.y);

        {
            // グリッド
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .Depth(true, false)
                .Blend(true, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Grid.glsl", compiledData);
            gridShader   = api->CreateShader(compiledData);
            gridPipeline = api->CreateGraphicsPipeline(gridShader, &pipelineInfo, environment.pass);
            gridUBO      = CreateUniformBuffer(nullptr, sizeof(Test::GridData));

            gridSet = CreateDescriptorSet(gridShader, 0);
            gridSet->SetResource(0, gridUBO);
            gridSet->Flush();
        }

        {
            // コンポジット
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);

            RenderPassClearValue clear = {};
            clear.SetFloat(0, 0, 0, 1);

            compositePass    = api->CreateRenderPass(1, &color, 1, &subpass, 0, nullptr, 1, &clear);
            compositeTexture = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, size.x, size.y);

            auto hcomposite = compositeTexture->GetHandle();
            compositeTextureView = CreateTextureView(compositeTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
            compositeFB          = CreateFramebuffer(compositePass, 1, &hcomposite, size.x, size.y);

            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .Blend(false, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Composit.glsl", compiledData);
            compositeShader   = api->CreateShader(compiledData);
            compositePipeline = api->CreateGraphicsPipeline(compositeShader, &pipelineInfo, compositePass);

            compositeSet = CreateDescriptorSet(compositeShader, 0);
            compositeSet->SetResource(0, bloom->bloomView, linearSampler);
            compositeSet->Flush();
        }

        // ImGui ビューポート用 デスクリプターセット
        imageSet = CreateDescriptorSet(compositeShader, 0);
        imageSet->SetResource(0, compositeTextureView, linearSampler);
        imageSet->Flush();
    }

    void Renderer::PrepareIBL(const char* environmentTexturePath)
    {
        TextureReader reader;
        byte*         pixels;

        pixels = reader.Read(environmentTexturePath);
        envTexture     = CreateTextureFromMemory(pixels, reader.data.byteSize, reader.data.width, reader.data.height, true);
        envTextureView = CreateTextureView(envTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);

        // 環境マップ変換 レンダーパス
        Attachment color = {};
        color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
        color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        color.loadOp        = ATTACHMENT_LOAD_OP_DONT_CARE;
        color.storeOp       = ATTACHMENT_STORE_OP_STORE;
        color.samples       = TEXTURE_SAMPLES_1;
        color.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

        AttachmentReference colorRef = {};
        colorRef.attachment = 0;
        colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        Subpass subpass = {};
        subpass.colorReferences.push_back(colorRef);

        RenderPassClearValue clear;
        clear.SetFloat(0, 0, 0, 1);
        IBLProcessPass = api->CreateRenderPass(1, &color, 1, &subpass, 0, nullptr, 1, &clear);

        // シェーダー
        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/IBL/EquirectangularToCubeMap.glsl", compiledData);
        equirectangularShader = api->CreateShader(compiledData);

        // キューブマップ
        const uint32 envResolution = 2048;
        cubemapTexture     = CreateTextureCube(RENDERING_FORMAT_R8G8B8A8_UNORM, envResolution, envResolution, true);
        cubemapTextureView = CreateTextureView(cubemapTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT);

        // 一時ビュー (キューブマップがミップレベルをもつため、コピー操作時に単一ミップを対象としたビューが別途必要)
        TextureView* captureView = CreateTextureView(cubemapTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT, 0, 6, 0, 1);

        // キューブマップ UBO
        Test::EquirectangularData data;
        data.projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1.0f);
        data.view[0]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f));
        data.view[1]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f));
        data.view[2]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3( 0.0f,  0.0f, -1.0f));
        data.view[3]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3( 0.0f,  0.0f, -1.0f));
        data.view[4]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3( 0.0f, -1.0f,  0.0f));
        data.view[5]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3( 0.0f, -1.0f,  0.0f));
        equirectangularUBO = CreateUniformBuffer(&data, sizeof(Test::EquirectangularData));

        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .InputLayout(1, &defaultLayout)
            .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        equirectangularPipeline = api->CreateGraphicsPipeline(equirectangularShader, &pipelineInfo, IBLProcessPass);

        // デスクリプタ
        equirectangularSet = CreateDescriptorSet(equirectangularShader, 0);
        equirectangularSet->SetResource(0, equirectangularUBO);
        equirectangularSet->SetResource(1, envTextureView, linearSampler);
        equirectangularSet->Flush();

        auto hcubemap = cubemapTexture->GetHandle();
        IBLProcessFB = CreateFramebuffer(IBLProcessPass, 1, &hcubemap, envResolution, envResolution);

        // キューブマップ書き込み
        ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            // イメージレイアウトを read_only に移行させる
            TextureBarrierInfo info = {};
            info.texture      = cubemapTexture->GetHandle();
            info.subresources = {};
            info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout    = TEXTURE_LAYOUT_UNDEFINED;
            info.newLayout    = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            //-------------------------------------------------------------------------------------------------------------------------
            // ↑このレイアウト移行を実行しない場合、shader_read_only に移行していなので、リソースの利用時に バリデーションエラーが発生する
            // レンダーパスとして mip[0] のみを使用した場合はその例外となる状態が発生した。（mip[0]を書き込みに使用し、残りのレベルは Blitでコピー）
            //-------------------------------------------------------------------------------------------------------------------------
            // 移行しない場合、イメージのすべてのサブリソースが undefine 状態であり、その状態で書き込み後のレイアウト遷移では shader_read_only を期待している
            // ので、すべてのミップレベルでレイアウトに起因するバリデーションエラーが発生するはずだが、mip[0] のみ エラーが発生しなかった。
            // また、レイアウトを mip[1~N] のように mip[0] を除いて移行したとしても、エラーは発生しなかった。
            // 
            // つまり、mip[0] が暗黙的に shader_read_only に移行しているのか、どこかで移行しているのを見逃しているかもしれないし
            // ドライバーが暗黙的に移行させているのかもしれない。（AMDなら検証できるのかも？）
            //-------------------------------------------------------------------------------------------------------------------------


            MeshSource* ms = cubeMesh->GetMeshSource();

            api->Cmd_SetViewport(cmd, 0, 0, envResolution, envResolution);
            api->Cmd_SetScissor(cmd,  0, 0, envResolution, envResolution);

            auto* view = captureView->GetHandle();
            api->Cmd_BeginRenderPass(cmd, IBLProcessPass, IBLProcessFB, 1, &view);
            api->Cmd_BindPipeline(cmd, equirectangularPipeline);
            api->Cmd_BindDescriptorSet(cmd, equirectangularSet->GetHandle(frameIndex), 0);
            api->Cmd_BindVertexBuffer(cmd, ms->GetVertexBuffer()->GetHandle(), 0);
            api->Cmd_BindIndexBuffer(cmd, ms->GetIndexBuffer()->GetHandle(), INDEX_BUFFER_FORMAT_UINT32, 0);
            api->Cmd_DrawIndexed(cmd, ms->GetIndexCount(), 1, 0, 0, 0);
            api->Cmd_EndRenderPass(cmd);

            info = {};
            info.texture      = cubemapTexture->GetHandle();
            info.subresources = {};
            info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout    = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info.newLayout    = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
            
            _GenerateMipmaps(cmd, cubemapTexture->GetHandle(), envResolution, envResolution, 1, 6, TEXTURE_ASPECT_COLOR_BIT);
            
            info.oldLayout = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
        });

        // キャプチャ用テンポラリビュー破棄
        DestroyTextureView(captureView);

        CreateIrradiance();
        CreatePrefilter();
        CreateBRDF();
    }

    void Renderer::CreateIrradiance()
    {
        const uint32 irradianceResolution = 32;

        // シェーダー
        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/IBL/Irradiance.glsl", compiledData);
        irradianceShader = api->CreateShader(compiledData);

        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .InputLayout(1, &defaultLayout)
            .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        irradiancePipeline = api->CreateGraphicsPipeline(irradianceShader, &pipelineInfo, IBLProcessPass);

        // キューブマップ
        irradianceTexture     = CreateTextureCube(RENDERING_FORMAT_R8G8B8A8_UNORM, irradianceResolution, irradianceResolution, false);
        irradianceTextureView = CreateTextureView(irradianceTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT);

        // バッファを再利用するため、リサイズ
        api->DestroyFramebuffer(IBLProcessFB);
        auto hirradiance = irradianceTexture->GetHandle();
        IBLProcessFB = api->CreateFramebuffer(IBLProcessPass, 1, &hirradiance, irradianceResolution, irradianceResolution);

        // デスクリプタ
        irradianceSet = CreateDescriptorSet(irradianceShader, 0);
        irradianceSet->SetResource(0, equirectangularUBO               );
        irradianceSet->SetResource(1, cubemapTextureView, linearSampler);
        irradianceSet->Flush();

        // キューブマップ書き込み
        ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            MeshSource* ms = cubeMesh->GetMeshSource();

            api->Cmd_SetViewport(cmd, 0, 0, irradianceResolution, irradianceResolution);
            api->Cmd_SetScissor(cmd,  0, 0, irradianceResolution, irradianceResolution);

            auto* view = irradianceTextureView->GetHandle();
            api->Cmd_BeginRenderPass(cmd, IBLProcessPass, IBLProcessFB, 1, &view);
            api->Cmd_BindPipeline(cmd, irradiancePipeline);
            api->Cmd_BindDescriptorSet(cmd, irradianceSet->GetHandle(frameIndex), 0);
            api->Cmd_BindVertexBuffer(cmd, ms->GetVertexBuffer()->GetHandle(), 0);
            api->Cmd_BindIndexBuffer(cmd, ms->GetIndexBuffer()->GetHandle(), INDEX_BUFFER_FORMAT_UINT32, 0);
            api->Cmd_DrawIndexed(cmd, ms->GetIndexCount(), 1, 0, 0, 0);
            api->Cmd_EndRenderPass(cmd);
        });
    }

    void Renderer::CreatePrefilter()
    {
        const uint32 prefilterResolution = 256;
        const uint32 prefilterMipCount   = 5;
        const auto   miplevels           = RenderingUtility::CalculateMipmap(prefilterResolution, prefilterResolution);

        // シェーダー
        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/IBL/Prefilter.glsl", compiledData);
        prefilterShader = api->CreateShader(compiledData);

        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .InputLayout(1, &defaultLayout)
            .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        prefilterPipeline = api->CreateGraphicsPipeline(prefilterShader, &pipelineInfo, IBLProcessPass);

        // キューブマップ
        prefilterTexture = CreateTextureCube(RENDERING_FORMAT_R8G8B8A8_UNORM, prefilterResolution, prefilterResolution, true);
        
        // 使用するミップレベル分だけ (5個[0 ~ 4])
        prefilterTextureView = CreateTextureView(prefilterTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT, 0, 6, 0, prefilterMipCount);
        auto hprefilter = prefilterTexture->GetHandle();

        // 生成用ビュー（ミップレベル分用意）
        std::array<TextureView*, prefilterMipCount> prefilterViews;
        for (uint32 i = 0; i < prefilterMipCount; i++)
        {
            prefilterViews[i] = CreateTextureView(prefilterTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT, 0, 6, i, 1);
        }

        // 生成用フレームバッファ（ミップレベル分用意）
        std::array<FramebufferHandle*, prefilterMipCount> prefilterFBs;
        for (uint32 i = 0; i < prefilterMipCount; i++)
        {
            prefilterFBs[i] = api->CreateFramebuffer(IBLProcessPass, 1, &hprefilter, miplevels[i].width, miplevels[i].height);
        }

        // デスクリプタ
        prefilterSet = CreateDescriptorSet(prefilterShader, 0);
        prefilterSet->SetResource(0, equirectangularUBO);
        prefilterSet->SetResource(1, cubemapTextureView, linearSampler);
        prefilterSet->Flush();

        // キューブマップ書き込み
        ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            MeshSource* ms = cubeMesh->GetMeshSource();
            api->Cmd_BindVertexBuffer(cmd, ms->GetVertexBuffer()->GetHandle(), 0);
            api->Cmd_BindIndexBuffer(cmd, ms->GetIndexBuffer()->GetHandle(), INDEX_BUFFER_FORMAT_UINT32, 0);

            for (uint32 i = 0; i < prefilterMipCount; i++)
            {
                api->Cmd_SetViewport(cmd, 0, 0, miplevels[i].width, miplevels[i].height);
                api->Cmd_SetScissor(cmd,  0, 0, miplevels[i].width, miplevels[i].height);

                auto* view = prefilterViews[i]->GetHandle();
                api->Cmd_BeginRenderPass(cmd, IBLProcessPass, prefilterFBs[i], 1, &view);
                api->Cmd_BindPipeline(cmd, prefilterPipeline);
                api->Cmd_BindDescriptorSet(cmd, prefilterSet->GetHandle(frameIndex), 0);

                //-------------------------------------------------------
                // firstInstance でミップレベルを指定（シェーダー内でラフネス計算）
                //-------------------------------------------------------
                api->Cmd_DrawIndexed(cmd, ms->GetIndexCount(), 1, 0, 0, i);
                api->Cmd_EndRenderPass(cmd);
            }

            //==========================================================================================
            // ミップ 0~4 は 'SHADER_READ_ONLY' (移行済み) なので、oldLayout は 'UNDEFINED' ではないが
            // 現状は 再度 0~ALL に対して 'UNDEFINED' -> 'SHADER_READ_ONLY' を実行しても 検証エラーが出ない
            // Vulkan の仕様なのか、NVIDIA だから問題ないのかは 現状不明
            //==========================================================================================
            //TextureBarrierInfo info = {};
            //info.texture      = prefilterTexture;
            //info.subresources = {};
            //info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            //info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            //info.oldLayout    = TEXTURE_LAYOUT_UNDEFINED;
            //info.newLayout    = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            //api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
        });

        // テンポラリオブジェクト破棄
        for (uint32 i = 0; i < prefilterMipCount; i++)
        {
            api->DestroyFramebuffer(prefilterFBs[i]);
            DestroyTextureView(prefilterViews[i]);
        }
    }

    void Renderer::CreateBRDF()
    {
        const uint32 brdfResolution = 512;

        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/IBL/BRDF.glsl", compiledData);
        brdflutShader = api->CreateShader(compiledData);

        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        brdflutPipeline = api->CreateGraphicsPipeline(brdflutShader, &pipelineInfo, IBLProcessPass);
        brdflutTexture  = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, brdfResolution, brdfResolution, false);

        auto hbrdf = brdflutTexture->GetHandle();
        brdflutTextureView = CreateTextureView(brdflutTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);

        // バッファを再利用するため、リサイズ
        api->DestroyFramebuffer(IBLProcessFB);
        IBLProcessFB = api->CreateFramebuffer(IBLProcessPass, 1, &hbrdf, brdfResolution, brdfResolution);

        // キューブマップ書き込み
        ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            MeshSource* ms = cubeMesh->GetMeshSource();

            api->Cmd_SetViewport(cmd, 0, 0, brdfResolution, brdfResolution);
            api->Cmd_SetScissor(cmd,  0, 0, brdfResolution, brdfResolution);

            auto* view = brdflutTextureView->GetHandle();
            api->Cmd_BeginRenderPass(cmd, IBLProcessPass, IBLProcessFB, 1, &view);
            api->Cmd_BindPipeline(cmd, brdflutPipeline);
            api->Cmd_Draw(cmd, 3, 1, 0, 0);
            api->Cmd_EndRenderPass(cmd);
        });
    }


    void Renderer::PrepareShadowBuffer()
    {
        Attachment depth = {};
        depth.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
        depth.finalLayout   = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        depth.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp       = ATTACHMENT_STORE_OP_STORE;
        depth.samples       = TEXTURE_SAMPLES_1;
        depth.format        = RENDERING_FORMAT_D32_SFLOAT;

        AttachmentReference depthRef = {};
        depthRef.attachment = 0;
        depthRef.layout     = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        Subpass subpass = {};
        subpass.depthstencilReference = depthRef;

        SubpassDependency dep = {};
        dep.srcSubpass = RENDER_AUTO_ID;
        dep.dstSubpass = 0;
        dep.srcStages  = PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstStages  = PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccess  = BARRIER_ACCESS_SHADER_READ_BIT;
        dep.dstAccess  = BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        RenderPassClearValue clear;
        clear.SetDepthStencil(1.0f, 0);
        shadow.pass  = api->CreateRenderPass(1, &depth, 1, &subpass, 1, &dep, 1, &clear);
        shadow.depth = CreateTexture2DArray(RENDERING_FORMAT_D32_SFLOAT, shadowMapResolution, shadowMapResolution, 4);
        
        auto hdepth = shadow.depth->GetHandle();
        shadow.depthView   = CreateTextureView(shadow.depth, TEXTURE_TYPE_2D_ARRAY, TEXTURE_ASPECT_DEPTH_BIT);
        shadow.framebuffer = CreateFramebuffer(shadow.pass, 1, &hdepth, shadowMapResolution, shadowMapResolution);
        
        shadow.transformUBO      = CreateUniformBuffer(nullptr, sizeof(Test::ShadowTransformData));
        shadow.lightTransformUBO = CreateUniformBuffer(nullptr, sizeof(Test::LightSpaceTransformData));
        shadow.cascadeUBO        = CreateUniformBuffer(nullptr, sizeof(Test::CascadeData));

        // パイプライン
        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .InputLayout(1, &defaultLayout)
            .Depth(true, true)
            .RasterizerDepthBias(true, 1.0, 2.0)
            .Blend(false, 1)
            .Value();

        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/DirectionalLight.glsl", compiledData);
        shadow.shader   = api->CreateShader(compiledData);
        shadow.pipeline = api->CreateGraphicsPipeline(shadow.shader, &pipelineInfo, shadow.pass);

        // デスクリプター
        shadow.set = CreateDescriptorSet(shadow.shader, 0);
        shadow.set->SetResource(0, shadow.transformUBO     );
        shadow.set->SetResource(1, shadow.lightTransformUBO);
        shadow.set->Flush();
    }

    void Renderer::CleanupShadowBuffer()
    {
        DestroyBuffer(shadow.transformUBO);
        DestroyBuffer(shadow.lightTransformUBO);
        DestroyBuffer(shadow.cascadeUBO);
        DestroyTexture(shadow.depth);

        DestroyTextureView(shadow.depthView);

        api->DestroyShader(shadow.shader);
        api->DestroyPipeline(shadow.pipeline);
        DestroyDescriptorSet(shadow.set);
        api->DestroyRenderPass(shadow.pass);
        api->DestroyFramebuffer(shadow.framebuffer);
    }

    void Renderer::CalculateLightSapceMatrices(glm::vec3 directionalLightDir, Camera* camera, std::array<glm::mat4, 4>& out_result)
    {
        auto nearP  = camera->GetNearPlane();
        auto farP   = camera->GetFarPlane();

        out_result[0] = (GetLightSpaceMatrix(directionalLightDir, camera, nearP,                  shadowCascadeLevels[0]));
        out_result[1] = (GetLightSpaceMatrix(directionalLightDir, camera, shadowCascadeLevels[0], shadowCascadeLevels[1]));
        out_result[2] = (GetLightSpaceMatrix(directionalLightDir, camera, shadowCascadeLevels[1], shadowCascadeLevels[2]));
        out_result[3] = (GetLightSpaceMatrix(directionalLightDir, camera, shadowCascadeLevels[2], farP));
    }

    glm::mat4 Renderer::GetLightSpaceMatrix(glm::vec3 directionalLightDir, Camera* camera, const float nearPlane, const float farPlane)
    {
        glm::mat4 proj = glm::perspective(glm::radians(camera->GetFOV()), (float)sceneFramebufferSize.x / (float)sceneFramebufferSize.y, nearPlane, farPlane);

        std::array<glm::vec4, 8> corners;
        GetFrustumCornersWorldSpace(proj * camera->GetViewMatrix(), corners);

        glm::vec3 center = glm::vec3(0, 0, 0);
        for (const auto& v : corners)
        {
            center += glm::vec3(v);
        }

        center /= corners.size();

        const auto lightView = glm::lookAt(center + glm::normalize(directionalLightDir), center, glm::vec3(0.0f, 1.0f, 0.0f));
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();

        for (const auto& v : corners)
        {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        constexpr float zMult = 10.0f;

        if (minZ < 0) minZ *= zMult;
        else          minZ /= zMult;

        if (maxZ < 0) maxZ /= zMult;
        else          maxZ *= zMult;

        const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        return lightProjection * lightView;
    }

    void Renderer::GetFrustumCornersWorldSpace(const glm::mat4& projview, std::array<glm::vec4, 8>& out_result)
    {
        const glm::mat4 inv = glm::inverse(projview);
        uint32 index = 0;

        for (uint32 x = 0; x < 2; ++x)
        {
            for (uint32 y = 0; y < 2; ++y)
            {
                for (uint32 z = 0; z < 2; ++z)
                {
                    const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    out_result[index] = (pt / pt.w);
                    index++;
                }
            }
        }
    }

    void Renderer::PrepareGBuffer(uint32 width, uint32 height)
    {
        gbuffer->albedo   = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM,          width, height);
        gbuffer->normal   = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM,          width, height);
        gbuffer->emission = CreateTexture2D(RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32, width, height);
        gbuffer->id       = CreateTexture2D(RENDERING_FORMAT_R32_SINT,                width, height, false, TEXTURE_USAGE_COPY_SRC_BIT);
        gbuffer->depth    = CreateTexture2D(RENDERING_FORMAT_D32_SFLOAT,              width, height);

        gbuffer->albedoView   = CreateTextureView(gbuffer->albedo,   TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->normalView   = CreateTextureView(gbuffer->normal,   TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->emissionView = CreateTextureView(gbuffer->emission, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->idView       = CreateTextureView(gbuffer->id,       TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->depthView    = CreateTextureView(gbuffer->depth,    TEXTURE_TYPE_2D, TEXTURE_ASPECT_DEPTH_BIT);

        RenderPassClearValue clearvalues[5] = {};
        Attachment           attachments[5] = {};
        Subpass              subpass        = {};

        {
            // ベースカラー
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(colorRef);
            attachments[0] = color;
            clearvalues[0].SetFloat(0, 0, 0, 0);

            // ノーマル
            Attachment normal = {};
            normal.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            normal.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normal.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            normal.storeOp       = ATTACHMENT_STORE_OP_STORE;
            normal.samples       = TEXTURE_SAMPLES_1;
            normal.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference normalRef = {};
            normalRef.attachment = 1;
            normalRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(normalRef);
            attachments[1] = normal;
            clearvalues[1].SetFloat(0, 0, 0, 1);

            // エミッション
            Attachment emission = {};
            emission.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            emission.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            emission.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            emission.storeOp       = ATTACHMENT_STORE_OP_STORE;
            emission.samples       = TEXTURE_SAMPLES_1;
            emission.format        = RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32;

            AttachmentReference emissionRef = {};
            emissionRef.attachment = 2;
            emissionRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(emissionRef);
            attachments[2] = emission;
            clearvalues[2].SetFloat(0, 0, 0, 1);

            // ID
            Attachment id = {};
            id.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            id.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            id.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            id.storeOp       = ATTACHMENT_STORE_OP_STORE;
            id.samples       = TEXTURE_SAMPLES_1;
            id.format        = RENDERING_FORMAT_R32_SINT;

            AttachmentReference idRef = {};
            idRef.attachment = 3;
            idRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(idRef);
            attachments[3] = id;
            clearvalues[3].SetInt(10, 0, 0, 1);

            // 深度
            Attachment depth = {};
            depth.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            depth.finalLayout   = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depth.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            depth.storeOp       = ATTACHMENT_STORE_OP_STORE;
            depth.samples       = TEXTURE_SAMPLES_1;
            depth.format        = RENDERING_FORMAT_D32_SFLOAT;

            AttachmentReference depthRef = {};
            depthRef.attachment = 4;
            depthRef.layout     = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            subpass.depthstencilReference = depthRef;
            attachments[4] = depth;
            clearvalues[4].SetDepthStencil(1.0f, 0);

            gbuffer->pass = api->CreateRenderPass(std::size(attachments), attachments, 1, &subpass, 0, nullptr, std::size(clearvalues), clearvalues);
        }

        {
            TextureHandle* attachments[] = { 
                gbuffer->albedo->GetHandle(),
                gbuffer->normal->GetHandle(),
                gbuffer->emission->GetHandle(),
                gbuffer->id->GetHandle(),
                gbuffer->depth->GetHandle(),
            };

            gbuffer->framebuffer = CreateFramebuffer(gbuffer->pass, std::size(attachments), attachments, width, height);
        }

        {
            // Gバッファ―
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .InputLayout(1, &defaultLayout)
                .Depth(true, true, COMPARE_OP_LESS)
                .Blend(false, 4)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/DeferredPrimitive.glsl", compiledData);
            gbuffer->shader   = api->CreateShader(compiledData);
            gbuffer->pipeline = api->CreateGraphicsPipeline(gbuffer->shader, &pipelineInfo, gbuffer->pass);
        }

        // ユニフォームバッファ
        gbuffer->materialUBO  = CreateUniformBuffer(nullptr, sizeof(Test::MaterialUBO));
        gbuffer->transformUBO = CreateUniformBuffer(nullptr, sizeof(Test::Transform));

        // トランスフォーム
        gbuffer->transformSet = CreateDescriptorSet(gbuffer->shader, 0);
        gbuffer->transformSet->SetResource(0, gbuffer->transformUBO);
        gbuffer->transformSet->Flush();

        // マテリアル
        gbuffer->materialSet = CreateDescriptorSet(gbuffer->shader, 1);
        gbuffer->materialSet->SetResource(0, gbuffer->materialUBO             );
        gbuffer->materialSet->SetResource(1, defaultTextureView, linearSampler);
        gbuffer->materialSet->Flush();
    }

    void Renderer::PrepareLightingBuffer(uint32 width, uint32 height)
    {
        // 初期レイアウトとサブパスレイアウトの違い
        //https://community.khronos.org/t/initial-layout-in-vkattachmentdescription/109850/5

        // パス
        Attachment color = {};
        color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
        color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        color.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp       = ATTACHMENT_STORE_OP_STORE;
        color.samples       = TEXTURE_SAMPLES_1;
        color.format        = RENDERING_FORMAT_R16G16B16A16_SFLOAT;

        AttachmentReference colorRef = {};
        colorRef.attachment = 0;
        colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        Subpass subpass = {};
        subpass.colorReferences.push_back(colorRef);

        RenderPassClearValue clear;
        clear.SetFloat(0, 0, 0, 1);

        lighting.pass        = api->CreateRenderPass(1, &color, 1, &subpass, 0, nullptr, 1, &clear);
        lighting.color       = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);

        auto hcolor = lighting.color->GetHandle();
        lighting.view        = CreateTextureView(lighting.color, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        lighting.framebuffer = CreateFramebuffer(lighting.pass, 1, &hcolor, width, height);

        // パイプライン
        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/DeferredLighting.glsl", compiledData);
        lighting.shader   = api->CreateShader(compiledData);
        lighting.pipeline = api->CreateGraphicsPipeline(lighting.shader, &pipelineInfo, lighting.pass);
        lighting.sceneUBO = CreateUniformBuffer(nullptr, sizeof(Test::SceneUBO));

        // セット
        lighting.set = CreateDescriptorSet(lighting.shader, 0);
        lighting.set->SetResource( 0, gbuffer->albedoView,   linearSampler);
        lighting.set->SetResource( 1, gbuffer->normalView,   linearSampler);
        lighting.set->SetResource( 2, gbuffer->emissionView, linearSampler);
        lighting.set->SetResource( 3, gbuffer->depthView,    linearSampler);
        lighting.set->SetResource( 4, irradianceTextureView, linearSampler);
        lighting.set->SetResource( 5, prefilterTextureView,  linearSampler);
        lighting.set->SetResource( 6, brdflutTextureView,    linearSampler);
        lighting.set->SetResource( 7, shadow.depthView,      shadowSampler);
        lighting.set->SetResource( 8, lighting.sceneUBO                   );
        lighting.set->SetResource( 9, shadow.cascadeUBO                   );
        lighting.set->SetResource(10, shadow.lightTransformUBO            );
        lighting.set->Flush();
    }

    void Renderer::PrepareEnvironmentBuffer(uint32 width, uint32 height)
    {
        {
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_LOAD;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R16G16B16A16_SFLOAT;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Attachment depth = {};
            depth.initialLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depth.finalLayout   = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depth.loadOp        = ATTACHMENT_LOAD_OP_LOAD;
            depth.storeOp       = ATTACHMENT_STORE_OP_STORE;
            depth.samples       = TEXTURE_SAMPLES_1;
            depth.format        = RENDERING_FORMAT_D32_SFLOAT;

            AttachmentReference depthRef = {};
            depthRef.attachment = 1;
            depthRef.layout     = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);
            subpass.depthstencilReference = depthRef;

            RenderPassClearValue clear[2] = {};
            clear[0].SetFloat(0, 0, 0, 1);
            clear[1].SetDepthStencil(1, 0);

            Attachment attachments[] = {color, depth};
            environment.pass = api->CreateRenderPass(std::size(attachments), attachments, 1, &subpass, 0, nullptr, 2, clear);

            TextureHandle* textures[] = { lighting.color->GetHandle(), gbuffer->depth->GetHandle() };
            environment.framebuffer = CreateFramebuffer(environment.pass, std::size(textures), textures, width, height);
        }

        {
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .InputLayout(1, &defaultLayout)
                .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
                .Depth(true, false, COMPARE_OP_LESS_OR_EQUAL)
                .Blend(false, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Environment.glsl", compiledData);
            environment.shader   = api->CreateShader(compiledData);
            environment.pipeline = api->CreateGraphicsPipeline(environment.shader, &pipelineInfo, environment.pass);
            environment.ubo      = CreateUniformBuffer(nullptr, sizeof(Test::EnvironmentUBO));

            environment.set = CreateDescriptorSet(environment.shader, 0);
            environment.set->SetResource(0, environment.ubo);
            environment.set->SetResource(1, cubemapTextureView, linearSampler);
            environment.set->Flush();
        }
    }

    void Renderer::CleanupIBL()
    {
        DestroyTexture(envTexture);
        DestroyTextureView(envTextureView);

        api->DestroyFramebuffer(IBLProcessFB);
        api->DestroyRenderPass(IBLProcessPass);
        DestroyBuffer(equirectangularUBO);

        api->DestroyPipeline(equirectangularPipeline);
        api->DestroyShader(equirectangularShader);
        DestroyDescriptorSet(equirectangularSet);
        DestroyTexture(cubemapTexture);
        DestroyTextureView(cubemapTextureView);

        api->DestroyPipeline(irradiancePipeline);
        api->DestroyShader(irradianceShader);
        DestroyDescriptorSet(irradianceSet);
        DestroyTexture(irradianceTexture);
        DestroyTextureView(irradianceTextureView);

        api->DestroyPipeline(prefilterPipeline);
        api->DestroyShader(prefilterShader);
        DestroyDescriptorSet(prefilterSet);
        DestroyTexture(prefilterTexture);
        DestroyTextureView(prefilterTextureView);

        api->DestroyPipeline(brdflutPipeline);
        api->DestroyShader(brdflutShader);
        DestroyTexture(brdflutTexture);
        DestroyTextureView(brdflutTextureView);
    }

    void Renderer::CleanupGBuffer()
    {
        api->DestroyShader(gbuffer->shader);
        api->DestroyPipeline(gbuffer->pipeline);
        api->DestroyRenderPass(gbuffer->pass);
        api->DestroyFramebuffer(gbuffer->framebuffer);

        DestroyTexture(gbuffer->albedo);
        DestroyTexture(gbuffer->normal);
        DestroyTexture(gbuffer->emission);
        DestroyTexture(gbuffer->id);
        DestroyTexture(gbuffer->depth);

        DestroyTextureView(gbuffer->albedoView);
        DestroyTextureView(gbuffer->normalView);
        DestroyTextureView(gbuffer->emissionView);
        DestroyTextureView(gbuffer->idView);
        DestroyTextureView(gbuffer->depthView);

        DestroyDescriptorSet(gbuffer->transformSet);
        DestroyDescriptorSet(gbuffer->materialSet);

        DestroyBuffer(gbuffer->materialUBO);
        DestroyBuffer(gbuffer->transformUBO);
    }

    void Renderer::CleanupLightingBuffer()
    {
        api->DestroyShader(lighting.shader);
        api->DestroyPipeline(lighting.pipeline);
        api->DestroyRenderPass(lighting.pass);
        api->DestroyFramebuffer(lighting.framebuffer);
        DestroyTextureView(lighting.view);

        DestroyTexture(lighting.color);
        DestroyDescriptorSet(lighting.set);
        DestroyBuffer(lighting.sceneUBO);
    }

    void Renderer::CleanupEnvironmentBuffer()
    {
        api->DestroyShader(environment.shader);
        api->DestroyPipeline(environment.pipeline);
        api->DestroyRenderPass(environment.pass);
        api->DestroyFramebuffer(environment.framebuffer);

        DestroyDescriptorSet(environment.set);
        DestroyBuffer(environment.ubo);
    }

    void Renderer::ResizeGBuffer(uint32 width, uint32 height)
    {
        DestroyTexture(gbuffer->albedo);
        DestroyTexture(gbuffer->normal);
        DestroyTexture(gbuffer->emission);
        DestroyTexture(gbuffer->id);
        DestroyTexture(gbuffer->depth);

        DestroyTextureView(gbuffer->albedoView);
        DestroyTextureView(gbuffer->normalView);
        DestroyTextureView(gbuffer->emissionView);
        DestroyTextureView(gbuffer->idView);
        DestroyTextureView(gbuffer->depthView);

        DestroyFramebuffer(gbuffer->framebuffer);

        gbuffer->albedo   = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM,          width, height);
        gbuffer->normal   = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM,          width, height);
        gbuffer->emission = CreateTexture2D(RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32, width, height);
        gbuffer->id       = CreateTexture2D(RENDERING_FORMAT_R32_SINT,                width, height, false, TEXTURE_USAGE_COPY_SRC_BIT);
        gbuffer->depth    = CreateTexture2D(RENDERING_FORMAT_D32_SFLOAT,              width, height);

        gbuffer->albedoView   = CreateTextureView(gbuffer->albedo,   TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->normalView   = CreateTextureView(gbuffer->normal,   TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->emissionView = CreateTextureView(gbuffer->emission, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->idView       = CreateTextureView(gbuffer->id,       TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->depthView    = CreateTextureView(gbuffer->depth,    TEXTURE_TYPE_2D, TEXTURE_ASPECT_DEPTH_BIT);

        TextureHandle* textures[] = { 
            gbuffer->albedo->GetHandle(),
            gbuffer->normal->GetHandle(),
            gbuffer->emission->GetHandle(),
            gbuffer->id->GetHandle(),
            gbuffer->depth->GetHandle(),
        };

        gbuffer->framebuffer = CreateFramebuffer(gbuffer->pass, std::size(textures), textures, width, height);
    }

    void Renderer::ResizeLightingBuffer(uint32 width, uint32 height)
    {
        DestroyTexture(lighting.color);
        DestroyTextureView(lighting.view);
        DestroyFramebuffer(lighting.framebuffer);

        lighting.color = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);

        auto hcolor = lighting.color->GetHandle();
        lighting.view        = CreateTextureView(lighting.color, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        lighting.framebuffer = CreateFramebuffer(lighting.pass, 1, &hcolor, width, height);

        DestroyDescriptorSet(lighting.set);
        lighting.set = CreateDescriptorSet(lighting.shader, 0);
        lighting.set->SetResource( 0, gbuffer->albedoView,   linearSampler);
        lighting.set->SetResource( 1, gbuffer->normalView,   linearSampler);
        lighting.set->SetResource( 2, gbuffer->emissionView, linearSampler);
        lighting.set->SetResource( 3, gbuffer->depthView,    linearSampler);
        lighting.set->SetResource( 4, irradianceTextureView, linearSampler);
        lighting.set->SetResource( 5, prefilterTextureView,  linearSampler);
        lighting.set->SetResource( 6, brdflutTextureView,    linearSampler);
        lighting.set->SetResource( 7, shadow.depthView,      shadowSampler);
        lighting.set->SetResource( 8, lighting.sceneUBO                   );
        lighting.set->SetResource( 9, shadow.cascadeUBO                   );
        lighting.set->SetResource(10, shadow.lightTransformUBO            );
        lighting.set->Flush();
    }

    void Renderer::ResizeEnvironmentBuffer(uint32 width, uint32 height)
    {
        DestroyFramebuffer(environment.framebuffer);

        TextureHandle* textures[] = { lighting.color->GetHandle(), gbuffer->depth->GetHandle() };
        environment.framebuffer = CreateFramebuffer(environment.pass, std::size(textures), textures, width, height);
    }

    std::vector<Extent> Renderer::CalculateBlomSampling(uint32 width, uint32 height)
    {
        // 解像度に変化し、上限で6回サンプリングする
        auto miplevels = RenderingUtility::CalculateMipmap(width, height);
        if (miplevels.size() - 1 > bloom->numDefaultSampling + 1)
        {
            miplevels.resize(bloom->numDefaultSampling + 1);
        }

        miplevels.erase(miplevels.begin());
        return miplevels;
    }

    void Renderer::PrepareBloomBuffer(uint32 width, uint32 height)
    {
        {
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R16G16B16A16_SFLOAT;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);

            SubpassDependency dep = {};
            dep.srcSubpass = RENDER_AUTO_ID;
            dep.dstSubpass = 0;
            dep.srcStages  = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dep.dstStages  = PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dep.srcAccess  = BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dep.dstAccess  = BARRIER_ACCESS_SHADER_READ_BIT;

            RenderPassClearValue clear;
            clear.SetFloat(0, 0, 0, 1);

            // レンダーパス
            bloom->pass = api->CreateRenderPass(1, &color, 1, &subpass, 1, &dep, 1, &clear);
        }

        bloom->resolutions.clear();
        bloom->resolutions = CalculateBlomSampling(width, height);
        bloom->sampling.resize(bloom->resolutions.size());
        bloom->samplingView.resize(bloom->resolutions.size());
        bloom->samplingFB.resize(bloom->resolutions.size());
        bloom->downSamplingSet.resize(bloom->resolutions.size());
        bloom->upSamplingSet.resize(bloom->resolutions.size() - 1);

        // サンプリングイメージ
        for (uint32 i = 0; i < bloom->resolutions.size(); i++)
        {
            const Extent extent = bloom->resolutions[i];
            bloom->sampling[i]     = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, extent.width, extent.height);

            auto hsampling = bloom->sampling[i]->GetHandle();
            bloom->samplingView[i] = CreateTextureView(bloom->sampling[i], TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
            bloom->samplingFB[i]   = CreateFramebuffer(bloom->pass, 1, &hsampling, extent.width, extent.height);
        }

        // ブルーム プリフィルター/ブレンド イメージ
        bloom->prefilter     = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);
        bloom->prefilterView = CreateTextureView(bloom->prefilter, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        bloom->bloom         = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);
        bloom->bloomView     = CreateTextureView(bloom->bloom, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);

        // フレームバッファ
        auto hbloom = bloom->bloom->GetHandle();
        bloom->bloomFB = CreateFramebuffer(bloom->pass, 1, &hbloom, width, height);


        // パイプライン
        ShaderCompiledData compiledData;
        PipelineStateInfoBuilder builder;

        PipelineStateInfo pipelineInfo = builder
            .Depth(false, false)
            .Blend(false, 1) // ブレンド 無し
            .Value();

        PipelineStateInfo pipelineInfoUpSampling = builder
            .Depth(false, false)
            .Blend(true, 1) // ブレンド 有り
            .Value();

        ShaderCompiler::Get()->Compile("Assets/Shaders/Bloom.glsl", compiledData);
        bloom->bloomShader   = api->CreateShader(compiledData);
        bloom->bloomPipeline = api->CreateGraphicsPipeline(bloom->bloomShader, &pipelineInfo, bloom->pass);

        ShaderCompiler::Get()->Compile("Assets/Shaders/BloomPrefiltering.glsl", compiledData);
        bloom->prefilterShader   = api->CreateShader(compiledData);
        bloom->prefilterPipeline = api->CreateGraphicsPipeline(bloom->prefilterShader, &pipelineInfo, bloom->pass);

        ShaderCompiler::Get()->Compile("Assets/Shaders/BloomDownSampling.glsl", compiledData);
        bloom->downSamplingShader   = api->CreateShader(compiledData);
        bloom->downSamplingPipeline = api->CreateGraphicsPipeline(bloom->downSamplingShader, &pipelineInfo, bloom->pass);

        ShaderCompiler::Get()->Compile("Assets/Shaders/BloomUpSampling.glsl", compiledData);
        bloom->upSamplingShader   = api->CreateShader(compiledData);
        bloom->upSamplingPipeline = api->CreateGraphicsPipeline(bloom->upSamplingShader, &pipelineInfoUpSampling, bloom->pass);


        // プリフィルター
        bloom->prefilterSet = CreateDescriptorSet(bloom->prefilterShader, 0);
        bloom->prefilterSet->SetResource(0, lighting.view, linearSampler);
        bloom->prefilterSet->Flush();

        // ダウンサンプリング (source)  - (target)
        // prefilter - sample[0]
        // sample[0] - sample[1]
        // sample[1] - sample[2]
        // sample[2] - sample[3]
        // sample[3] - sample[4]
        // sample[4] - sample[5];
        bloom->downSamplingSet[0] = CreateDescriptorSet(bloom->downSamplingShader, 0);
        bloom->downSamplingSet[0]->SetResource(0, bloom->prefilterView, linearSampler);
        bloom->downSamplingSet[0]->Flush();

        for (uint32 i = 1; i < bloom->downSamplingSet.size(); i++)
        {
            bloom->downSamplingSet[i] = CreateDescriptorSet(bloom->downSamplingShader, 0);
            bloom->downSamplingSet[i]->SetResource(0, bloom->samplingView[i - 1], linearSampler);
            bloom->downSamplingSet[i]->Flush();
        }

        // アップサンプリング (source)  - (target)
        // sample[5] - sample[4]
        // sample[4] - sample[3]
        // sample[3] - sample[2]
        // sample[2] - sample[1]
        // sample[1] - sample[0]
        uint32 upSamplingIndex = bloom->upSamplingSet.size();
        for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
        {
            bloom->upSamplingSet[i] = CreateDescriptorSet(bloom->upSamplingShader, 0);
            bloom->upSamplingSet[i]->SetResource(0, bloom->samplingView[upSamplingIndex], linearSampler);
            bloom->upSamplingSet[i]->Flush();

            upSamplingIndex--;
        }

        // ブルームコンポジット
        bloom->bloomSet = CreateDescriptorSet(bloom->bloomShader, 0);
        bloom->bloomSet->SetResource(0, lighting.view,          linearSampler);
        bloom->bloomSet->SetResource(1, bloom->samplingView[0], linearSampler);
        bloom->bloomSet->Flush();
    }

    void Renderer::ResizeBloomBuffer(uint32 width, uint32 height)
    {
        for (uint32 i = 0; i < bloom->resolutions.size(); i++)
        {
            DestroyTexture(bloom->sampling[i]);
            DestroyTextureView(bloom->samplingView[i]);
            DestroyFramebuffer(bloom->samplingFB[i]);
        }

        DestroyFramebuffer(bloom->bloomFB);
        DestroyTexture(bloom->prefilter);
        DestroyTextureView(bloom->prefilterView);
        DestroyTexture(bloom->bloom);
        DestroyTextureView(bloom->bloomView);

        DestroyDescriptorSet(bloom->prefilterSet);
        DestroyDescriptorSet(bloom->bloomSet);

        for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
        {
            DestroyDescriptorSet(bloom->upSamplingSet[i]);
        }

        for (uint32 i = 0; i < bloom->downSamplingSet.size(); i++)
        {
            DestroyDescriptorSet(bloom->downSamplingSet[i]);
        }

        bloom->resolutions.clear();
        bloom->resolutions = CalculateBlomSampling(width, height);
        bloom->sampling.resize(bloom->resolutions.size());
        bloom->samplingView.resize(bloom->resolutions.size());
        bloom->samplingFB.resize(bloom->resolutions.size());
        bloom->downSamplingSet.resize(bloom->resolutions.size());
        bloom->upSamplingSet.resize(bloom->resolutions.size() - 1);

        // イメージ
        for (uint32 i = 0; i < bloom->resolutions.size(); i++)
        {
            const Extent extent = bloom->resolutions[i];
            bloom->sampling[i] = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, extent.width, extent.height);

            auto hsample = bloom->sampling[i]->GetHandle();
            bloom->samplingView[i] = CreateTextureView(bloom->sampling[i], TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
            bloom->samplingFB[i]   = CreateFramebuffer(bloom->pass, 1, &hsample, extent.width, extent.height);
        }

        bloom->prefilter     = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);
        bloom->prefilterView = CreateTextureView(bloom->prefilter, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        bloom->bloom         = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);

        auto hbloom = bloom->bloom->GetHandle();
        bloom->bloomView     = CreateTextureView(bloom->bloom, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        bloom->bloomFB       = CreateFramebuffer(bloom->pass, 1, &hbloom, width, height);


        // プリフィルター
        bloom->prefilterSet = CreateDescriptorSet(bloom->prefilterShader, 0);
        bloom->prefilterSet->SetResource(0, lighting.view, linearSampler);
        bloom->prefilterSet->Flush();

        // ダウンサンプリング (source)  - (target)
        // prefilter - sample[0]
        // sample[0] - sample[1]
        // sample[1] - sample[2]
        // sample[2] - sample[3]
        // sample[3] - sample[4]
        // sample[4] - sample[5]
        bloom->downSamplingSet[0] = CreateDescriptorSet(bloom->downSamplingShader, 0);
        bloom->downSamplingSet[0]->SetResource(0, bloom->prefilterView, linearSampler);
        bloom->downSamplingSet[0]->Flush();

        for (uint32 i = 1; i < bloom->downSamplingSet.size(); i++)
        {
            bloom->downSamplingSet[i] = CreateDescriptorSet(bloom->downSamplingShader, 0);
            bloom->downSamplingSet[i]->SetResource(0, bloom->samplingView[i - 1], linearSampler);
            bloom->downSamplingSet[i]->Flush();
        }

        // アップサンプリング (source)  - (target)
        // sample[5] - sample[4]
        // sample[4] - sample[3]
        // sample[3] - sample[2]
        // sample[2] - sample[1]
        // sample[1] - sample[0]
        uint32 upSamplingIndex = bloom->upSamplingSet.size();
        for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
        {
            bloom->upSamplingSet[i] = CreateDescriptorSet(bloom->upSamplingShader, 0);
            bloom->upSamplingSet[i]->SetResource(0, bloom->samplingView[upSamplingIndex], linearSampler);
            bloom->upSamplingSet[i]->Flush();

            upSamplingIndex--;
        }

        bloom->bloomSet = CreateDescriptorSet(bloom->bloomShader, 0);
        bloom->bloomSet->SetResource(0, lighting.view,          linearSampler);
        bloom->bloomSet->SetResource(1, bloom->samplingView[0], linearSampler);
        bloom->bloomSet->Flush();
    }

    void Renderer::CleanupBloomBuffer()
    {
        api->DestroyRenderPass(bloom->pass);

        for (uint32 i = 0; i < bloom->resolutions.size(); i++)
        {
            api->DestroyFramebuffer(bloom->samplingFB[i]);
            DestroyTextureView(bloom->samplingView[i]);
            DestroyTexture(bloom->sampling[i]);
        }

        for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
        {
            DestroyDescriptorSet(bloom->upSamplingSet[i]);
        }

        for (uint32 i = 0; i < bloom->downSamplingSet.size(); i++)
        {
            DestroyDescriptorSet(bloom->downSamplingSet[i]);
        }

        DestroyTexture(bloom->bloom);
        DestroyTexture(bloom->prefilter);

        DestroyTextureView(bloom->bloomView);
        DestroyTextureView(bloom->prefilterView);
        api->DestroyFramebuffer(bloom->bloomFB);

        api->DestroyShader(bloom->bloomShader);
        api->DestroyPipeline(bloom->bloomPipeline);
        api->DestroyShader(bloom->prefilterShader);
        api->DestroyPipeline(bloom->prefilterPipeline);
        api->DestroyShader(bloom->downSamplingShader);
        api->DestroyPipeline(bloom->downSamplingPipeline);
        api->DestroyShader(bloom->upSamplingShader);
        api->DestroyPipeline(bloom->upSamplingPipeline);

        DestroyDescriptorSet(bloom->bloomSet);
        DestroyDescriptorSet(bloom->prefilterSet);
    }

    int32 Renderer::ReadPixelObjectID(uint32 x, uint32 y)
    {
        ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            TextureBarrierInfo info = {};
            info.texture      = gbuffer->id->GetHandle();
            info.subresources = {};
            info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout    = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info.newLayout    = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            BufferTextureCopyRegion region = {};
            region.bufferOffset        = 0;
            region.textureSubresources = {};
            region.textureRegionSize   = {1, 1, 1};
            region.textureOffset       = {x, y, 0};

            api->Cmd_CopyTextureToBuffer(cmd, gbuffer->id->GetHandle(), TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, pixelIDBuffer->GetHandle(0), 1, &region);

            info.oldLayout = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
        });

        int32* outID = (int32*)pixelIDBuffer->GetMappedPointer();
        return *outID;
    }

    void Renderer::Resize(uint32 width, uint32 height)
    {
        if (width == 0 || height == 0)
            return;

        sceneFramebufferSize = {width, height};

        ResizeGBuffer(width, height);
        ResizeLightingBuffer(width, height);
        ResizeEnvironmentBuffer(width, height);
        ResizeBloomBuffer(width, height);

        {
            DestroyFramebuffer(compositeFB);
            DestroyTexture(compositeTexture);
            DestroyTextureView(compositeTextureView);

            compositeTexture = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, width, height);

            auto hcomposite = compositeTexture->GetHandle();
            compositeTextureView = CreateTextureView(compositeTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
            compositeFB          = CreateFramebuffer(compositePass, 1, &hcomposite, width, height);

            DestroyDescriptorSet(compositeSet);
            compositeSet = CreateDescriptorSet(compositeShader, 0);
            compositeSet->SetResource(0, bloom->bloomView, linearSampler);
            compositeSet->Flush();
        }

        {
            DestroyDescriptorSet(imageSet);
            imageSet = CreateDescriptorSet(compositeShader, 0);
            imageSet->SetResource(0, compositeTextureView, linearSampler);
            imageSet->Flush();
        }
    }

    void Renderer::UpdateUBO()
    {
        Camera* camera = Engine::Get()->GetEditor()->GetEditorCamera();

        {
            Test::ShadowTransformData shadowData;
            shadowData.world = glm::mat4(1.0);

            shadow.transformUBO->SetData(&shadowData, sizeof(Test::ShadowTransformData));
        }

        {
            Test::Transform sceneData;
            sceneData.projection = camera->GetProjectionMatrix();
            sceneData.view       = camera->GetViewMatrix();
            sceneData.world      = glm::mat4(1.0);

            gbuffer->transformUBO->SetData(&sceneData, sizeof(Test::Transform));
        }

        {
            Test::MaterialUBO matData = {};
            matData.albedo        = glm::vec3(1.0);
            matData.emission      = glm::vec3(0.0);
            matData.metallic      = 0.0f;
            matData.roughness     = 0.5f;
            matData.textureTiling = glm::vec2(1.0, 1.0);

            gbuffer->materialUBO->SetData(&matData, sizeof(Test::MaterialUBO));
        }

        {
            Test::GridData gridData;
            gridData.projection = camera->GetProjectionMatrix();
            gridData.view       = camera->GetViewMatrix();
            gridData.pos        = glm::vec4(camera->GetPosition(), camera->GetFarPlane());

            gridUBO->SetData(&gridData, sizeof(Test::GridData));
        }

        {
            std::array<glm::mat4, 4> outLightMatrices;
            CalculateLightSapceMatrices(sceneLightDir, camera, outLightMatrices);

            Test::LightSpaceTransformData lightData = {};
            lightData.cascade[0] = outLightMatrices[0];
            lightData.cascade[1] = outLightMatrices[1];
            lightData.cascade[2] = outLightMatrices[2];
            lightData.cascade[3] = outLightMatrices[3];
            shadow.lightTransformUBO->SetData(&lightData, sizeof(Test::LightSpaceTransformData));

            Test::SceneUBO sceneUBO = {};
            sceneUBO.lightColor               = glm::vec4(1.0, 1.0, 1.0, 1.0);
            sceneUBO.lightDir                 = glm::vec4(sceneLightDir, 1.0);
            sceneUBO.cameraPosition           = glm::vec4(camera->GetPosition(), camera->GetFarPlane());
            sceneUBO.view                     = camera->GetViewMatrix();
            sceneUBO.invViewProjection        = glm::inverse(camera->GetProjectionMatrix() * camera->GetViewMatrix());
            lighting.sceneUBO->SetData(&sceneUBO, sizeof(Test::SceneUBO));

            Test::CascadeData cascadeData;
            cascadeData.cascadePlaneDistances[0].x = shadowCascadeLevels[0];
            cascadeData.cascadePlaneDistances[1].x = shadowCascadeLevels[1];
            cascadeData.cascadePlaneDistances[2].x = shadowCascadeLevels[2];
            cascadeData.cascadePlaneDistances[3].x = shadowCascadeLevels[3];
            shadow.cascadeUBO->SetData(&cascadeData, sizeof(Test::CascadeData));
        }

        {
            Test::EnvironmentUBO environmentUBO = {};
            environmentUBO.view       = camera->GetViewMatrix();
            environmentUBO.projection = camera->GetProjectionMatrix();
            environment.ubo->SetData(&environmentUBO, sizeof(Test::EnvironmentUBO));
        }
    }



    void Renderer::Render(float dt)
    {
        UpdateUBO();


        FrameData& frame = frameData[frameIndex];
        const auto& viewportSize = sceneFramebufferSize;

        // コマンドバッファ開始
        api->BeginCommandBuffer(frame.commandBuffer);

        //===================================================================================================
        // memcopy mapped buffer in-between command buffer calls?
        // https://www.reddit.com/r/vulkan/comments/110ygxu/memcopy_mapped_buffer_inbetween_command_buffer/
        //---------------------------------------------------------------------------------------------------
        // UBO のマルチバッファリング化が実装されるまで、パイプラインバリアで同期するようにする
        // ただし、マルチバッファリングを使用しない限り、即時書き込みのUBOの同期をとる方法はないので、現状効果はない
        //===================================================================================================
        
        // HOST_COHERENT フラグ: 明示的にフラッシュしなくても、コマンドバッファが転送された時点でGPUと同期される
        // メモリバリア         : GPU側が読み取るタイミングの動機

        // マルチバッファリングを実装できれば、前フレームの実行がフェンスで完了している状態でUBOの更新が可能なので、この待機は不必要となる
        // MemoryBarrierInfo mb = {};
        // mb.srcAccess = BARRIER_ACCESS_HOST_WRITE_BIT;
        // mb.dstAccess = BARRIER_ACCESS_UNIFORM_READ_BIT;
        // api->PipelineBarrier(frame.commandBuffer, PIPELINE_STAGE_HOST_BIT, PIPELINE_STAGE_VERTEX_SHADER_BIT, 1, &mb, 0, nullptr, 0, nullptr);


        // シャドウパス
        if (1)
        {
            api->Cmd_SetViewport(frame.commandBuffer, 0, 0, shadowMapResolution, shadowMapResolution);
            api->Cmd_SetScissor(frame.commandBuffer, 0, 0, shadowMapResolution, shadowMapResolution);

            auto* view = shadow.depthView->GetHandle();
            api->Cmd_BeginRenderPass(frame.commandBuffer, shadow.pass, shadow.framebuffer, 1, &view);

            api->Cmd_BindPipeline(frame.commandBuffer, shadow.pipeline);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, shadow.set->GetHandle(frameIndex), 0);

            // スポンザ
            for (MeshSource* source : sponzaMesh->GetMeshSources())
            {
                BufferHandle* vb = source->GetVertexBuffer()->GetHandle();
                BufferHandle* ib = source->GetIndexBuffer()->GetHandle();
                uint32 indexCount = source->GetIndexCount();
                api->Cmd_BindVertexBuffer(frame.commandBuffer, vb, 0);
                api->Cmd_BindIndexBuffer(frame.commandBuffer, ib, INDEX_BUFFER_FORMAT_UINT32, 0);
                api->Cmd_DrawIndexed(frame.commandBuffer, indexCount, 1, 0, 0, 0);
            }

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }

        if (1) // メッシュパス
        {
            api->Cmd_SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
            api->Cmd_SetScissor(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);

            TextureViewHandle* views[] = {
                gbuffer->albedoView->GetHandle(),
                gbuffer->normalView->GetHandle(),
                gbuffer->emissionView->GetHandle(),
                gbuffer->idView->GetHandle(),
                gbuffer->depthView->GetHandle(),
            };

            api->Cmd_BeginRenderPass(frame.commandBuffer, gbuffer->pass, gbuffer->framebuffer, 5, views);
            
            api->Cmd_BindPipeline(frame.commandBuffer, gbuffer->pipeline);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, gbuffer->transformSet->GetHandle(frameIndex), 0);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, gbuffer->materialSet->GetHandle(frameIndex), 1);

            //========================================================================
            // TODO: バインドレスとインスタンシング描画のためのストレージバッファの設計までに...
            //------------------------------------------------------------------------
            // 設計が完了するまでは個別でドローコールすることになるので、ワールド行列はプッシュ定数で
            // 渡すように変更する。現在は定数バッファで渡しているので、シーンで一律なカメラの
            // ビュー・プロジェクション行列とは分離させる
            // 
            // ※本来は描画パス全体で使用する共通パラメータ(View/Projection)として1回だけバインド
            //========================================================================
            
            // スポンザ
            for (MeshSource* source : sponzaMesh->GetMeshSources())
            {
                BufferHandle* vb  = source->GetVertexBuffer()->GetHandle();
                BufferHandle* ib  = source->GetIndexBuffer()->GetHandle();
                uint32 indexCount = source->GetIndexCount();
                api->Cmd_BindVertexBuffer(frame.commandBuffer, vb, 0);
                api->Cmd_BindIndexBuffer(frame.commandBuffer, ib, INDEX_BUFFER_FORMAT_UINT32, 0);
                api->Cmd_DrawIndexed(frame.commandBuffer, indexCount, 1, 0, 0, 0);
            }

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }

        if (1) // ライティングパス
        {
            auto* view = lighting.view->GetHandle();
            api->Cmd_BeginRenderPass(frame.commandBuffer, lighting.pass, lighting.framebuffer, 1, &view);

            api->Cmd_BindPipeline(frame.commandBuffer, lighting.pipeline);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, lighting.set->GetHandle(frameIndex), 0);
            api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }

        if (1) // スカイパス
        {
            TextureViewHandle* views[] = { lighting.view->GetHandle(), gbuffer->depthView->GetHandle() };
            api->Cmd_BeginRenderPass(frame.commandBuffer, environment.pass, environment.framebuffer, 2, views);

            if (1) // スカイ
            {
                api->Cmd_BindPipeline(frame.commandBuffer, environment.pipeline);
                api->Cmd_BindDescriptorSet(frame.commandBuffer, environment.set->GetHandle(frameIndex), 0);

                MeshSource* ms = cubeMesh->GetMeshSource();
                api->Cmd_BindVertexBuffer(frame.commandBuffer, ms->GetVertexBuffer()->GetHandle(), 0);
                api->Cmd_BindIndexBuffer(frame.commandBuffer, ms->GetIndexBuffer()->GetHandle(), INDEX_BUFFER_FORMAT_UINT32, 0);
                api->Cmd_DrawIndexed(frame.commandBuffer, ms->GetIndexCount(), 1, 0, 0, 0);
            }

            if (1) // グリッド
            {
                api->Cmd_BindPipeline(frame.commandBuffer, gridPipeline);
                api->Cmd_BindDescriptorSet(frame.commandBuffer, gridSet->GetHandle(frameIndex), 0);
                api->Cmd_Draw(frame.commandBuffer, 6, 1, 0, 0);
            }

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }

        if (1) // ブルーム
        {
            // プリフィルタリング
            if (1)
            {
                api->Cmd_SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
                api->Cmd_SetScissor(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);

                auto* view = bloom->prefilterView->GetHandle();
                api->Cmd_BeginRenderPass(frame.commandBuffer, bloom->pass, bloom->bloomFB, 1, &view);
                api->Cmd_BindPipeline(frame.commandBuffer, bloom->prefilterPipeline);

                float threshold = 10.0f;
                api->Cmd_PushConstants(frame.commandBuffer, bloom->prefilterShader, &threshold, 1);
                api->Cmd_BindDescriptorSet(frame.commandBuffer, bloom->prefilterSet->GetHandle(frameIndex), 0);
                api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

                api->Cmd_EndRenderPass(frame.commandBuffer);
            }

            // ダウンサンプリング
            if (1)
            {
                for (uint32 i = 0; i < bloom->downSamplingSet.size(); i++)
                {
                    auto* view = bloom->samplingView[i]->GetHandle();
                    api->Cmd_BeginRenderPass(frame.commandBuffer, bloom->pass, bloom->samplingFB[i], 1, &view);
                    api->Cmd_BindPipeline(frame.commandBuffer, bloom->downSamplingPipeline);
                    
                    api->Cmd_SetViewport(frame.commandBuffer, 0, 0, bloom->resolutions[i].width, bloom->resolutions[i].height);
                    api->Cmd_SetScissor(frame.commandBuffer,  0, 0, bloom->resolutions[i].width, bloom->resolutions[i].height);

                    glm::ivec2 srcResolution = i == 0? sceneFramebufferSize : glm::ivec2(bloom->resolutions[i - 1].width, bloom->resolutions[i - 1].height);
                    api->Cmd_PushConstants(frame.commandBuffer, bloom->downSamplingShader, &srcResolution[0], sizeof(srcResolution) / sizeof(uint32));
                    
                    api->Cmd_BindDescriptorSet(frame.commandBuffer, bloom->downSamplingSet[i]->GetHandle(frameIndex), 0);
                    api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

                    api->Cmd_EndRenderPass(frame.commandBuffer);
                }
            }

            // アップサンプリング
            if (1)
            {
                uint32 upSamplingIndex = bloom->upSamplingSet.size();
                for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
                {
                    auto* view = bloom->samplingView[upSamplingIndex - 1]->GetHandle();
                    api->Cmd_BeginRenderPass(frame.commandBuffer, bloom->pass, bloom->samplingFB[upSamplingIndex - 1], 1, &view);
                    api->Cmd_BindPipeline(frame.commandBuffer, bloom->upSamplingPipeline);
                    
                    api->Cmd_SetViewport(frame.commandBuffer, 0, 0, bloom->resolutions[upSamplingIndex - 1].width, bloom->resolutions[upSamplingIndex - 1].height);
                    api->Cmd_SetScissor(frame.commandBuffer,  0, 0, bloom->resolutions[upSamplingIndex - 1].width, bloom->resolutions[upSamplingIndex - 1].height);
                    
                    float filterRadius = 0.01f;
                    api->Cmd_PushConstants(frame.commandBuffer, bloom->upSamplingShader, &filterRadius, 1);

                    api->Cmd_BindDescriptorSet(frame.commandBuffer, bloom->upSamplingSet[i]->GetHandle(frameIndex), 0);
                    api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

                    api->Cmd_EndRenderPass(frame.commandBuffer);
                    upSamplingIndex--;
                }
            }

            // マージ
            if (1)
            {
                auto* view = bloom->bloomView->GetHandle();
                api->Cmd_BeginRenderPass(frame.commandBuffer, bloom->pass, bloom->bloomFB, 1, &view);
                api->Cmd_BindPipeline(frame.commandBuffer, bloom->bloomPipeline);

                api->Cmd_SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
                api->Cmd_SetScissor(frame.commandBuffer,  0, 0, viewportSize.x, viewportSize.y);

                float intencity = 0.1f;
                api->Cmd_PushConstants(frame.commandBuffer, bloom->bloomShader, &intencity, 1);

                api->Cmd_BindDescriptorSet(frame.commandBuffer, bloom->bloomSet->GetHandle(frameIndex), 0);
                api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

                api->Cmd_EndRenderPass(frame.commandBuffer);
            }
        }

        if (1) // コンポジットパス
        {
            auto* view = compositeTextureView->GetHandle();
            api->Cmd_BeginRenderPass(frame.commandBuffer, compositePass, compositeFB, 1, &view);

            api->Cmd_SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
            api->Cmd_SetScissor(frame.commandBuffer,  0, 0, viewportSize.x, viewportSize.y);

            api->Cmd_BindPipeline(frame.commandBuffer, compositePipeline);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, compositeSet->GetHandle(frameIndex), 0);
            api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }
    }




    Texture2D* Renderer::CreateTextureFromMemory(const uint8* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap)
    {
        // RGBA8_UNORM フォーマットテクスチャ
        TextureHandle* gpuTexture = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D, RENDERING_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, genMipmap, TEXTURE_USAGE_COPY_DST_BIT);
        _SubmitTextureData(gpuTexture, width, height, genMipmap, pixelData, dataSize);

        Texture2D* texture = slnew(Texture2D, numFramesInFlight);
        texture->handle[0] = gpuTexture;

        return texture;
    }

    Texture2D* Renderer::CreateTextureFromMemory(const float* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap)
    {
        // RGBA16_SFLOAT フォーマットテクスチャ
        TextureHandle* gpuTexture = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D, RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height, 1, 1, genMipmap, TEXTURE_USAGE_COPY_DST_BIT);
        _SubmitTextureData(gpuTexture, width, height, genMipmap, pixelData, dataSize);

        Texture2D* texture = slnew(Texture2D, numFramesInFlight);
        texture->handle[0] = gpuTexture;

        return texture;
    }

    Texture2D* Renderer::CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        Texture2D* texture = slnew(Texture2D, numFramesInFlight);
        TextureHandle* h = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D, format, width, height, 1, 1, genMipmap, additionalFlags);
        texture->handle[0] = h;

        return texture;
    }

    Texture2DArray* Renderer::CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        Texture2DArray* texture = slnew(Texture2DArray, numFramesInFlight);
        TextureHandle* h = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D_ARRAY, format, width, height, 1, array, genMipmap, additionalFlags);
        texture->handle[0] = h;

        return texture;
    }

    TextureCube* Renderer::CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        TextureCube* texture = slnew(TextureCube, numFramesInFlight);
        TextureHandle* h = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_CUBE, format, width, height, 1, 6, genMipmap, additionalFlags);
        texture->handle[0] = h;

        return texture;
    }

    void Renderer::DestroyTexture(Texture* texture)
    {
        FrameData& frame = frameData[frameIndex];

        TextureHandle* h = texture->GetHandle();
        frame.pendingResources->texture.push_back(h);

        sldelete(texture);
    }

    TextureView* Renderer::CreateTextureView(Texture* texture, TextureType type, TextureAspectFlags aspect, uint32 baseArrayLayer, uint32 numArrayLayer, uint32 baseMipLevel, uint32 numMipLevel)
    {
        TextureViewInfo viewInfo = {};
        viewInfo.type                      = type;
        viewInfo.subresource.aspect        = aspect;
        viewInfo.subresource.baseLayer     = baseArrayLayer;
        viewInfo.subresource.layerCount    = numArrayLayer;
        viewInfo.subresource.baseMipLevel  = baseMipLevel;
        viewInfo.subresource.mipLevelCount = numMipLevel;

        TextureView* view    = slnew(TextureView, numFramesInFlight);
        TextureViewHandle* h = api->CreateTextureView(texture->GetHandle(), viewInfo);
        view->handle[0] = h;

        return view;
    }

    void Renderer::DestroyTextureView(TextureView* view)
    {
        FrameData& frame = frameData[frameIndex];

        TextureViewHandle* h = view->GetHandle();
        frame.pendingResources->textureView.push_back(h);

        sldelete(view);
    }

    Sampler* Renderer::CreateSampler(SamplerFilter filter, SamplerRepeatMode mode, bool enableCompare, CompareOperator compareOp)
    {
        SamplerInfo samplerInfo = {};
        samplerInfo.magFilter     = filter;
        samplerInfo.minFilter     = filter;
        samplerInfo.mipFilter     = filter;
        samplerInfo.repeatU       = mode;
        samplerInfo.repeatV       = mode;
        samplerInfo.repeatW       = mode;
        samplerInfo.enableCompare = enableCompare;
        samplerInfo.compareOp     = compareOp;

        Sampler* sampler = slnew(Sampler, numFramesInFlight);
        SamplerHandle* h = api->CreateSampler(samplerInfo);
        sampler->handle[0] = h;

        return sampler;
    }

    void Renderer::DestroySampler(Sampler* sampler)
    {
        FrameData& frame = frameData[frameIndex];

        SamplerHandle* h = sampler->GetHandle();
        frame.pendingResources->sampler.push_back(h);

        sldelete(sampler);
    }

    TextureHandle* Renderer::_CreateTexture(TextureDimension dimension, TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 depth, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        int32 usage = TEXTURE_USAGE_SAMPLING_BIT;
        usage |= RenderingUtility::IsDepthFormat(format)? TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
        usage |= genMipmap? TEXTURE_USAGE_COPY_SRC_BIT | TEXTURE_USAGE_COPY_DST_BIT : 0;
        usage |= additionalFlags;

        auto mipmaps = RenderingUtility::CalculateMipmap(width, height);

        TextureInfo texformat = {};
        texformat.format    = format;
        texformat.width     = width;
        texformat.height    = height;
        texformat.dimension = dimension;
        texformat.type      = type;
        texformat.usageBits = usage;
        texformat.samples   = TEXTURE_SAMPLES_1;
        texformat.array     = array;
        texformat.depth     = depth;
        texformat.mipLevels = genMipmap? mipmaps.size() : 1;

        return api->CreateTexture(texformat);
    }

    void Renderer::_SubmitTextureData(TextureHandle* texture, uint32 width, uint32 height, bool genMipmap, const void* pixelData, uint64 dataSize)
    {
        // ステージングバッファ
        BufferHandle* staging = api->CreateBuffer(dataSize, BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);

        // ステージングにテクスチャデータをコピー
        void* mappedPtr = api->MapBuffer(staging);
        std::memcpy(mappedPtr, pixelData, dataSize);
        api->UnmapBuffer(staging);

        // コピーコマンド
        ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            TextureSubresourceRange range = {};
            range.aspect = TEXTURE_ASPECT_COLOR_BIT;

            TextureBarrierInfo info = {};
            info.texture      = texture;
            info.subresources = range;
            info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout    = TEXTURE_LAYOUT_UNDEFINED;
            info.newLayout    = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;

            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            TextureSubresource subresource = {};
            subresource.aspect     = TEXTURE_ASPECT_COLOR_BIT;

            BufferTextureCopyRegion region = {};
            region.bufferOffset        = 0;
            region.textureOffset       = { 0, 0, 0 };
            region.textureRegionSize   = { width, height, 1 };
            region.textureSubresources = subresource;

            api->Cmd_CopyBufferToTexture(cmd, staging, texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            if (genMipmap)
            {
                // ミップマップ生成
                _GenerateMipmaps(cmd, texture, width, height, 1, 1, TEXTURE_ASPECT_COLOR_BIT);

                // シェーダーリードに移行 (ミップマップ生成時のコピーでコピーソースに移行するため、コピーソース -> シェーダーリード)
                info.oldLayout = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
            }
            else
            {
                // シェーダーリードに移行 (バッファからの転送でレイアウト変更がないため、コピー先 -> シェーダーリード)
                info.oldLayout = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
                info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
            }
        });

        // ステージング破棄
        api->DestroyBuffer(staging);
    }
    
    void Renderer::_GenerateMipmaps(CommandBufferHandle* cmd, TextureHandle* texture, uint32 width, uint32 height, uint32 depth, uint32 array, TextureAspectFlags aspect)
    {
        auto mipmaps = RenderingUtility::CalculateMipmap(width, height);

        // コピー回数は (ミップ数 - 1)
        for (uint32 mipLevel = 0; mipLevel < mipmaps.size() - 1; mipLevel++)
        {
            // バリア
            TextureBarrierInfo info = {};
            info.texture                    = texture;
            info.subresources.aspect        = aspect;
            info.subresources.baseMipLevel  = mipLevel;
            info.subresources.mipLevelCount = 1;
            info.subresources.baseLayer     = 0;
            info.subresources.layerCount    = array;
            info.srcAccess                  = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess                  = BARRIER_ACCESS_MEMORY_WRITE_BIT | BARRIER_ACCESS_MEMORY_READ_BIT;
            info.oldLayout                  = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
            info.newLayout                  = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            // Blit
            Extent srcExtent = mipmaps[mipLevel + 0];
            Extent dstExtent = mipmaps[mipLevel + 1];

            //------------------------------------------------------------------------------------------------------------------------------------------
            // NOTE:
            // srcImageが VK_IMAGE_TYPE_1D または VK_IMAGE_TYPE_2D 型の場合、pRegionsの各要素について、srcOffsets[0].zは0、srcOffsets[1].z は1でなければなりません。
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdBlitImage.html#VUID-vkCmdBlitImage-srcImage-00247
            //------------------------------------------------------------------------------------------------------------------------------------------

            TextureBlitRegion region = {};
            region.srcOffset[0] = { 0, 0, 0 };
            region.dstOffset[0] = { 0, 0, 0 };
            region.srcOffset[1] = srcExtent;
            region.dstOffset[1] = dstExtent;

            region.srcSubresources.aspect     = aspect;
            region.srcSubresources.mipLevel   = mipLevel;
            region.srcSubresources.baseLayer  = 0;
            region.srcSubresources.layerCount = array;

            region.dstSubresources.aspect     = aspect;
            region.dstSubresources.mipLevel   = mipLevel + 1;
            region.dstSubresources.baseLayer  = 0;
            region.dstSubresources.layerCount = array;

            api->Cmd_BlitTexture(cmd, texture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, SAMPLER_FILTER_LINEAR);
        }

        //------------------------------------------------------------------------
        // 最後のミップレベルはコピー先としか利用せず、コピーソースレイアウトに移行していない
        // 後で実行されるレイアウト移行時に、ミップレベル間でレイアウトの不一致があると不便なので
        // 最後のミップレベルのレイアウトも [コピー先 → コピーソース] に移行させておく
        //------------------------------------------------------------------------
        TextureSubresourceRange range = {};
        range.aspect        = TEXTURE_ASPECT_COLOR_BIT;
        range.baseMipLevel  = mipmaps.size() - 1;
        range.mipLevelCount = 1;

        TextureBarrierInfo info = {};
        info.texture      = texture;
        info.subresources = range;
        info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
        info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT | BARRIER_ACCESS_MEMORY_READ_BIT;
        info.oldLayout    = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
        info.newLayout    = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
    }


    Buffer* Renderer::CreateBuffer(void* data, uint64 size)
    {
        Buffer* buffer = slnew(Buffer, numFramesInFlight);

        void* mapped = nullptr;
        BufferHandle* h = _CreateAndMapBuffer(0, data, size, &mapped);
        buffer->handle[0] = h;
        buffer->mappedPtr = mapped;

        return buffer;
    }

    UniformBuffer* Renderer::CreateUniformBuffer(void* data, uint64 size)
    {
        UniformBuffer* buffer = slnew(UniformBuffer, numFramesInFlight);
        for (uint32 i = 0; i < numFramesInFlight; i++)
        {
            void* mapped = nullptr;
            BufferHandle* h = _CreateAndMapBuffer(BUFFER_USAGE_UNIFORM_BIT, data, size, &mapped);
            buffer->handle[i] = h;
            buffer->mappedPtr = mapped;
        }
        
        return buffer;
    }

    BufferHandle* Renderer::CreateStorageBuffer(void* data, uint64 size)
    {
        //StorageBuffer* buffer = slnew(StorageBuffer, numFramesInFlight);
        //for (uint32 i = 0; i < numFramesInFlight; i++)
        //{
        //    void* mapped = nullptr;
        //    BufferHandle* h = _CreateAndMapBuffer(BUFFER_USAGE_STORAGE_BIT, data, size, &mapped);
        //    buffer->handle[i] = h;
        //    buffer->mappedPtr = mapped;
        //}

        return _CreateAndMapBuffer(BUFFER_USAGE_STORAGE_BIT, data, size, nullptr);
    }

    VertexBuffer* Renderer::CreateVertexBuffer(void* data, uint64 size)
    {
        VertexBuffer* buffer = slnew(VertexBuffer, numFramesInFlight);
        BufferHandle* h = _CreateAndSubmitBufferData(BUFFER_USAGE_VERTEX_BIT, data, size);
        buffer->handle[0] = h;

        return buffer;
    }

    IndexBuffer* Renderer::CreateIndexBuffer(void* data, uint64 size)
    {
        IndexBuffer* buffer = slnew(IndexBuffer, numFramesInFlight);
        BufferHandle* h = _CreateAndSubmitBufferData(BUFFER_USAGE_INDEX_BIT, data, size);
        buffer->handle[0] = h;

        return buffer;
    }

    void Renderer::DestroyBuffer(Buffer* buffer)
    {
        FrameData& frame = frameData[frameIndex];

        for (uint32 i = 0; i < numFramesInFlight; i++)
        {
            BufferHandle* handle = buffer->GetHandle(i);
            if (handle)
            {
                api->UnmapBuffer(handle);
                frame.pendingResources->buffer.push_back(handle);
            }
        }

        sldelete(buffer);
    }

    bool Renderer::UpdateBufferData(BufferHandle* buffer, const void* data, uint32 dataByte)
    {
        return api->UpdateBufferData(buffer, data, dataByte);
    }

    BufferHandle* Renderer::_CreateAndMapBuffer(BufferUsageFlags type, const void* data, uint64 dataSize, void** outMappedPtr)
    {
        BufferHandle* buffer  = api->CreateBuffer(dataSize, type | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_ALLOCATION_TYPE_CPU);
        void* mappedPtr = api->MapBuffer(buffer);

        if (data != nullptr)
            std::memcpy(mappedPtr, data, dataSize);

        if (outMappedPtr != nullptr)
            *outMappedPtr = mappedPtr;

        return buffer;
    }

    BufferHandle* Renderer::_CreateAndSubmitBufferData(BufferUsageFlags type, const void* data, uint64 dataSize)
    {
        BufferHandle* buffer  = api->CreateBuffer(dataSize, type | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_ALLOCATION_TYPE_GPU);
        BufferHandle* staging = api->CreateBuffer(dataSize, type | BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);
        
        void* mapped = api->MapBuffer(staging);
        std::memcpy(mapped, data, dataSize);
        api->UnmapBuffer(staging);

        ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            BufferCopyRegion region = {};
            region.size = dataSize;

            api->Cmd_CopyBuffer(cmd, staging, buffer, 1, &region);
        });

        api->DestroyBuffer(staging);

        return buffer;
    }



    FramebufferHandle* Renderer::CreateFramebuffer(RenderPassHandle* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height)
    {
        return api->CreateFramebuffer(renderpass, numTexture, textures, width, height);
    }

    void Renderer::DestroyFramebuffer(FramebufferHandle* framebuffer)
    {
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources->framebuffer.push_back(framebuffer);
    }


    DescriptorSet* Renderer::CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex)
    {
        DescriptorSet* set = slnew(DescriptorSet, numFramesInFlight);
        for (uint32 i = 0; i < numFramesInFlight; i++)
        {
            DescriptorSetHandle* h = api->CreateDescriptorSet(shader, setIndex);
            set->handle[i] = h;
        }

        return set;
    }

    void Renderer::UpdateDescriptorSet(DescriptorSetHandle* set, DescriptorSetInfo& setInfo)
    {
        api->UpdateDescriptorSet(set, setInfo.infos.size(), setInfo.infos.data());
    }

    void Renderer::DestroyDescriptorSet(DescriptorSet* set)
    {
        FrameData& frame = frameData[frameIndex];

        for (uint32 i = 0; i < numFramesInFlight; i++)
        {
            DescriptorSetHandle* h = set->GetHandle(i);
            frame.pendingResources->descriptorset.push_back(h);
        }

        sldelete(set);
    }



    SwapChainHandle* Renderer::CreateSwapChain(SurfaceHandle* surface, uint32 width, uint32 height, VSyncMode mode)
    {
        return api->CreateSwapChain(surface, width, height, swapchainBufferCount, mode);
    }

    void Renderer::DestoySwapChain(SwapChainHandle* swapchain)
    {
        api->DestroySwapChain(swapchain);
    }

    bool Renderer::ResizeSwapChain(SwapChainHandle* swapchain, uint32 width, uint32 height, VSyncMode mode)
    {
        return api->ResizeSwapChain(swapchain, width, height, swapchainBufferCount, mode);
    }

    bool Renderer::Present()
    {
        SwapChainHandle* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame = frameData[frameIndex];

        bool result = api->Present(graphicsQueue, swapchain, frame.renderSemaphore);
        frameIndex = (frameIndex + 1) % 2;

        return result;
    }

    void Renderer::BeginSwapChainPass()
    {
        FrameData& frame = frameData[frameIndex];
        api->Cmd_BeginRenderPass(frame.commandBuffer, swapchainPass, currentSwapchainFramebuffer, 1, &currentSwapchainView);
    }

    void Renderer::EndSwapChainPass()
    {
        FrameData& frame = frameData[frameIndex];
        api->Cmd_EndRenderPass(frame.commandBuffer);
    }

    void Renderer::ImmidiateExcute(std::function<void(CommandBufferHandle*)>&& func)
    {
        api->ImmidiateCommands(graphicsQueue, immidiateContext.commandBuffer, immidiateContext.fence, std::move(func));
    }

    void Renderer::_DestroyPendingResources(uint32 frame)
    {
        FrameData& f = frameData[frame];

        for (BufferHandle* buffer : f.pendingResources->buffer)
        {
            api->DestroyBuffer(buffer);
        }

        f.pendingResources->buffer.clear();

        for (TextureHandle* texture : f.pendingResources->texture)
        {
            api->DestroyTexture(texture);
        }

        f.pendingResources->texture.clear();

        for (TextureViewHandle* view : f.pendingResources->textureView)
        {
            api->DestroyTextureView(view);
        }

        f.pendingResources->textureView.clear();

        for (SamplerHandle* sampler : f.pendingResources->sampler)
        {
            api->DestroySampler(sampler);
        }

        f.pendingResources->sampler.clear();

        for (DescriptorSetHandle* set : f.pendingResources->descriptorset)
        {
            api->DestroyDescriptorSet(set);
        }

        f.pendingResources->descriptorset.clear();

        for (FramebufferHandle* framebuffer : f.pendingResources->framebuffer)
        {
            api->DestroyFramebuffer(framebuffer);
        }

        f.pendingResources->framebuffer.clear();

        for (ShaderHandle* shader : f.pendingResources->shader)
        {
            api->DestroyShader(shader);
        }

        f.pendingResources->shader.clear();

        for (PipelineHandle* pipeline : f.pendingResources->pipeline)
        {
            api->DestroyPipeline(pipeline);
        }

        f.pendingResources->pipeline.clear();
    }

    const DeviceInfo& Renderer::GetDeviceInfo() const
    {
        return context->GetDeviceInfo();
    }

    DescriptorSet* Renderer::GetSceneImageSet() const
    {
        return imageSet;
    }

    RenderingContext* Renderer::GetContext() const
    {
        return context;
    }

    RenderingAPI* Renderer::GetAPI() const
    {
        return api;
    }

    const FrameData& Renderer::GetFrameData() const
    {
        return frameData[frameIndex];
    }

    uint32 Renderer::GetCurrentFrameIndex() const
    {
        return frameIndex;
    }

    uint32 Renderer::GetFrameCountInFlight() const
    {
        return numFramesInFlight;
    }

    CommandQueueHandle* Renderer::GetGraphicsCommandQueue() const
    {
        return graphicsQueue;
    }

    QueueID Renderer::GetGraphicsQueueID() const
    {
        return graphicsQueueID;
    }
}
