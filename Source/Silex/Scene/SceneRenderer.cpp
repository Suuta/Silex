
#include "PCH.h"

#include "Asset/Asset.h"
#include "Core/Timer.h"
#include "Core/Engine.h"
#include "Editor/SplashImage.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Mesh.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/SkyLight.h"
#include "Rendering/Texture.h"
#include "Rendering/UniformBuffer.h"
#include "Rendering/StorageBuffer.h"
#include "Scene/SceneRenderer.h"

#include <glad/glad.h>



namespace Silex
{
    void SceneRenderer::Init()
    {
        context = Memory::Allocate<SceneRenderingContext>();

        //============================================
        // シェーダー
        //============================================
        context->preDownSamplingShader     = ShaderManager::Get()->Load("PreDownSampling");
        context->downSamplingShader        = ShaderManager::Get()->Load("DownSampling");
        context->upSamplingShader          = ShaderManager::Get()->Load("UpSampling");
        context->shadowShader              = ShaderManager::Get()->Load("Shadow");
        context->deferredLightingShader    = ShaderManager::Get()->Load("DeferredLighting");
        context->gBufferShader             = ShaderManager::Get()->Load("GBuffer");
        context->skyShader                 = ShaderManager::Get()->Load("Sky");
        context->fxaaShader                = ShaderManager::Get()->Load("FXAA");
        context->outlineShader             = ShaderManager::Get()->Load("Outline");
        context->chromaticAberrationShader = ShaderManager::Get()->Load("ChromaticAberration");
        context->bloomShader               = ShaderManager::Get()->Load("Bloom");
        context->tonemapShader             = ShaderManager::Get()->Load("Tonemap");

        //============================================
        // シャドウマップ: 1024 x 3
        //============================================
        // glPolygonOffset(2.0f, 0.0f) 要調整

        RHI::FramebufferAttachmentDesc shadowMapDesc = {};
        shadowMapDesc.Format           = RHI::RenderFormat::D32F;
        shadowMapDesc.AttachmentType   = RHI::AttachmentType::Depth;
        shadowMapDesc.TextureType      = RHI::TextureType::Texture2DArray;
        shadowMapDesc.TextureArraySize = context->shadowCascadeLevels.size();

        RHI::FramebufferDesc descFB = {};
        descFB.Width                = 2048;
        descFB.Height               = 2048;
        descFB.ColorAttachmentCount = 0;
        descFB.ClearColor           = { 0.0, 0.0, 0.0, 1.0 };
        descFB.AttachmentDescs      = { shadowMapDesc };
        context->shadowMapFB = Framebuffer::Create(descFB);

        //============================================
        // Gバッファ
        //============================================

        // アルベド
        RHI::FramebufferAttachmentDesc albedoDesc   = {};
        albedoDesc.Format           = RHI::RenderFormat::RGBA16F;
        albedoDesc.AttachmentType   = RHI::AttachmentType::Color;
        albedoDesc.TextureType      = RHI::TextureType::Texture2D;
        albedoDesc.TextureArraySize = 1;

        // 法線
        RHI::FramebufferAttachmentDesc normalDesc   = {};
        normalDesc.Format           = RHI::RenderFormat::RGBA32F;
        normalDesc.AttachmentType   = RHI::AttachmentType::Color;
        normalDesc.TextureType      = RHI::TextureType::Texture2D;
        normalDesc.TextureArraySize = 1;

        // 座標
        RHI::FramebufferAttachmentDesc positionDesc = {};
        positionDesc.Format           = RHI::RenderFormat::RGBA32F;
        positionDesc.AttachmentType   = RHI::AttachmentType::Color;
        positionDesc.TextureType      = RHI::TextureType::Texture2D;
        positionDesc.TextureArraySize = 1;

        // エミッション
        RHI::FramebufferAttachmentDesc emissionDesc = {};
        emissionDesc.Format           = RHI::RenderFormat::RGBA32F;
        emissionDesc.AttachmentType   = RHI::AttachmentType::Color;
        emissionDesc.TextureType      = RHI::TextureType::Texture2D;
        emissionDesc.TextureArraySize = 1;

        // マテリアルID + エンティティID
        RHI::FramebufferAttachmentDesc idDesc = {};
        idDesc.Format           = RHI::RenderFormat::RGBA32I;
        idDesc.AttachmentType   = RHI::AttachmentType::Color;
        idDesc.TextureType      = RHI::TextureType::Texture2D;
        idDesc.TextureArraySize = 1;

        // 深度バッファ
        RHI::FramebufferAttachmentDesc depthDesc = {};
        depthDesc.Format           = RHI::RenderFormat::D24S8;
        depthDesc.AttachmentType   = RHI::AttachmentType::DepthStencil;
        depthDesc.TextureType      = RHI::TextureType::Texture2D;
        depthDesc.TextureArraySize = 1;

        RHI::FramebufferDesc GBufferFBDecs = {};
        GBufferFBDecs.Width                = 1280;
        GBufferFBDecs.Height               = 720;
        GBufferFBDecs.ColorAttachmentCount = 5;
        GBufferFBDecs.ClearColor           = { 0.0, 0.0, 0.0, 1.0 };
        GBufferFBDecs.AttachmentDescs      = { albedoDesc, normalDesc, positionDesc, emissionDesc, idDesc, depthDesc };
        context->gBufferFB = Framebuffer::Create(GBufferFBDecs);

        //============================================
        // デファードライティングバッファ
        //============================================
        RHI::FramebufferAttachmentDesc mainColorDesc = {};
        mainColorDesc.Format           = RHI::RenderFormat::RGBA16F;
        mainColorDesc.AttachmentType   = RHI::AttachmentType::Color;
        mainColorDesc.TextureType      = RHI::TextureType::Texture2D;
        mainColorDesc.TextureArraySize = 1;

        RHI::FramebufferAttachmentDesc mainDepthDesc = {};
        mainDepthDesc.Format           = RHI::RenderFormat::D24S8;
        mainDepthDesc.AttachmentType   = RHI::AttachmentType::DepthStencil;
        mainDepthDesc.TextureType      = RHI::TextureType::Texture2D;
        mainDepthDesc.TextureArraySize = 1;

        RHI::FramebufferDesc mainFBDecs = {};
        mainFBDecs.Width                = 1280;
        mainFBDecs.Height               = 720;
        mainFBDecs.ColorAttachmentCount = 1;
        mainFBDecs.ClearColor           = { 0.0, 0.0, 0.0, 1.0 };
        mainFBDecs.AttachmentDescs      = { mainColorDesc, mainDepthDesc };
        context->deferredFB = Framebuffer::Create(mainFBDecs);

        //============================================
        // ブルーム
        //============================================
        RHI::TextureDesc bloomTextureDesc = {};
        bloomTextureDesc.Format = RHI::RenderFormat::RGBA16F;
        bloomTextureDesc.Type   = RHI::TextureType::Texture2D;
        bloomTextureDesc.Filter = RHI::TextureFilter::Linear;
        bloomTextureDesc.Wrap   = RHI::TextureWrap::ClampEdge;
        bloomTextureDesc.Size   = 1;

        context->bloomResolutions = CalculateBloomMipSize(1280, 720);
        for (int i = 0; i < context->bloomResolutions.size(); i++)
        {
            bloomTextureDesc.Width     = context->bloomResolutions[i].x;
            bloomTextureDesc.Height    = context->bloomResolutions[i].y;
            context->bloomTextures[i] = Texture2D::Create(bloomTextureDesc);
        }

        // ブルーム適応（しきい値設定のフィルタリング）テクスチャ
        bloomTextureDesc.Width  = 1280;
        bloomTextureDesc.Height = 720;
        context->preDownSamplingTexture = Texture2D::Create(bloomTextureDesc);


        RHI::FramebufferAttachmentDesc bloomSamplingDesc = {};
        bloomSamplingDesc.Format           = RHI::RenderFormat::RGBA16F;
        bloomSamplingDesc.AttachmentType   = RHI::AttachmentType::Color;
        bloomSamplingDesc.TextureType      = RHI::TextureType::Texture2D;
        bloomSamplingDesc.TextureArraySize = 1;

        // 書き込み用FBであり、テクスチャは後でアタッチする為、最小サイズ(1 x 1)の仮テクスチャを生成している
        RHI::FramebufferDesc bloomSamplingFB = {};
        bloomSamplingFB.Width                  = 1;
        bloomSamplingFB.Height                 = 1;
        bloomSamplingFB.ColorAttachmentCount   = 1;
        bloomSamplingFB.ClearColor             = { 0.0, 0.0, 0.0, 1.0 };
        bloomSamplingFB.AttachmentDescs        = { bloomSamplingDesc };
        
        context->bloomFB = Framebuffer::Create(bloomSamplingFB);

        //============================================
        // ポストプロセスバッファ
        //============================================
        RHI::FramebufferAttachmentDesc ppColorDesc = {};
        ppColorDesc.Format           = RHI::RenderFormat::RGBA16F;
        ppColorDesc.AttachmentType   = RHI::AttachmentType::Color;
        ppColorDesc.TextureType      = RHI::TextureType::Texture2D;
        ppColorDesc.TextureArraySize = 1;

        RHI::FramebufferDesc postProcessFB = {};
        postProcessFB.Width                = 1280;
        postProcessFB.Height               = 720;
        postProcessFB.ColorAttachmentCount = 1;
        postProcessFB.ClearColor           = { 0.0, 0.0, 0.0, 1.0 };
        postProcessFB.AttachmentDescs      = { ppColorDesc };

        context->finalPassFB = Framebuffer::Create(postProcessFB);
        context->temporaryFB = Framebuffer::Create(postProcessFB);

        //============================================
        // ライトスペースカスケードシャドウ行列
        //============================================
        context->cascadeUBO  = UniformBuffer::Create(sizeof(glm::mat4) * context->numCascadeLevel, 0, nullptr);
        context->materialUBO = UniformBuffer::Create(sizeof(MaterialUBO),                          1, nullptr);

        //============================================
        // インスタンシング用トランスフォーム
        //============================================
        context->meshParameters   = new MeshParameter[context->numMaxInstancing];
        context->meshParameterSBO = StorageBuffer::Create(context->numMaxInstancing * sizeof(MeshParameter), 0, nullptr);
    }

    void SceneRenderer::Shutdown()
    {
        delete[] context->meshParameters;

        for (int i = 0; i < context->bloomTextures.size(); i++)
        {
            Memory::Deallocate(context->bloomTextures[i]);
        }

        Memory::Deallocate(context->preDownSamplingTexture);
        Memory::Deallocate(context);
    }

    void SceneRenderer::BeginFrame(Scene* scene, Camera* camera)
    {
        // シーン情報をセット
        context->renderScene = scene;
        context->sceneCamera = camera;

        // 統計をリセット
        context->stats.numRenderMesh       = 0;
        context->stats.numGeometryDrawCall = 0;
        context->stats.numShadowDrawCall   = 0;

        // ステートをリセット
        context->shouldRenderShadow   = false;
        context->shouldRenderGeometry = false;

        // ポストプロセスのクリア
        context->option.postProcess = {};

        // ライトのリセット
        context->skyLight.enableIBL = false;
        context->skyLight.renderSky = false;
        context->directionalLight = {};

        // 描画リストリセット
        context->meshDrawList.clear();

        // シャドウインスタンスデータクリア
        context->shadowDrawData.clear();
        context->ShadowParameterData.clear();

        // メッシュインスタンスデータクリア
        context->meshDrawData.clear();
        context->meshParameterData.clear();
    }

    void SceneRenderer::EndFrame()
    {
        ShadowMapPass();
        GeometryPass();

        DeferredLightinglPass();
      //ForwardLightinglPass();

        SkyboxPass();
        PostProcessPass();
    }

    void SceneRenderer::SetSkyLight(const SkyLightComponent& data)
    {
        context->skyLight = data;
    }

    void SceneRenderer::SetDirectionalLight(const DirectionalLightComponent& data)
    {
        context->directionalLight = data;
        context->shouldRenderShadow = true;
    }

    void SceneRenderer::AddMeshDrawList(const MeshDrawData& data)
    {
        context->meshDrawList.emplace_back(data);
        context->shouldRenderGeometry = true;
        context->stats.numRenderMesh++;
    }

    void SceneRenderer::SetViewportSize(uint32 width, uint32 height)
    {
        if (context->viewportSize.x != width || context->viewportSize.y != height)
        {
            context->viewportSize.x = width;
            context->viewportSize.y = height;

            // フレームバッファのリサイズ
            context->gBufferFB->Resize(width, height);
            context->deferredFB->Resize(width, height);
            context->finalPassFB->Resize(width, height);
            context->temporaryFB->Resize(width, height);

            // ブルームサンプリングテクスチャ
            {
                // テクスチャのリサイズ
                RHI::TextureDesc bloomTextureDesc = {};
                bloomTextureDesc.Format = RHI::RenderFormat::RGBA16F;
                bloomTextureDesc.Type   = RHI::TextureType::Texture2D;
                bloomTextureDesc.Filter = RHI::TextureFilter::Linear;
                bloomTextureDesc.Wrap   = RHI::TextureWrap::ClampEdge;
                bloomTextureDesc.Size   = 1;

                context->bloomResolutions = CalculateBloomMipSize(width, height);

                for (int i = 0; i < context->bloomResolutions.size(); i++)
                {
                    Memory::Deallocate(context->bloomTextures[i]);

                    bloomTextureDesc.Width     = context->bloomResolutions[i].x;
                    bloomTextureDesc.Height    = context->bloomResolutions[i].y;
                    context->bloomTextures[i] = Texture2D::Create(bloomTextureDesc);
                }

                Memory::Deallocate(context->preDownSamplingTexture);
                bloomTextureDesc.Width  = width;
                bloomTextureDesc.Height = height;
                context->preDownSamplingTexture = Texture2D::Create(bloomTextureDesc);
            }
        }
    }


    //==================================
    // シャドウマップパス
    //==================================
    void SceneRenderer::ShadowMapPass()
    {
        context->shadowMapFB->Bind();
        context->shadowMapFB->Clear();

        if (context->shouldRenderShadow && context->shouldRenderGeometry && context->stats.numRenderMesh > 0)
        {
            SL_SCOPE_PROFILE("ShadowPass");

            context->shadowShader->Bind();

            // カスケードデータ
            const std::array<glm::mat4, context->numCascadeLevel> lightMatrices = GetLightSpaceMatrices(context->directionalLight.direction, *context->sceneCamera);
            context->cascadeUBO->SetData(0, sizeof(glm::mat4) * 4, &lightMatrices[0]);

            // 描画リストに積まれたメッシュを描画
            for (auto& data : context->meshDrawList)
            {
                Shared<Mesh> mesh = data.mesh.mesh;
                if (mesh && data.mesh.castShadow)
                {
                    uint32 sourceIndex = 0;
                    for (MeshSource* meshSource : mesh->GetMeshSources())
                    {
                        // メッシュのアセットIDから新規・既存メッシュを判定する
                        InstancingUnitID unit = { mesh->GetAssetID(), sourceIndex, (AssetID)0 };

                        // インスタンスユニットごとのトランスフォームデータ
                        glm::mat4 ts = data.transform * meshSource->GetTransform();
                        auto& param = context->ShadowParameterData[unit].parameters.emplace_back();
                        param.transform = ts;

                        // インスタンスユニットごとの描画データ
                        InstancingUnitData& unitdata = context->shadowDrawData[unit];
                        unitdata.instanceCount++;
                        unitdata.indexCount  = meshSource->HasIndex() ? meshSource->GetIndexCount() : 0;
                        unitdata.vertexCount = meshSource->GetVertexCount();
                        unitdata.meshAsset   = mesh.Get();
                        unitdata.source      = meshSource;
                        unitdata.material    = nullptr;

                        sourceIndex++;
                    }
                }
            }

            // インスタンスごとの描画データをシェーダーで扱うデータに整列する
            uint32 offset = 0;
            for (auto& [id, param] : context->ShadowParameterData)
            {
                param.offset = offset;
                for (const auto& param : param.parameters)
                {
                    context->meshParameters[offset].transform = param.transform;
                    offset++;
                }
            }

            // 全データを転送
            context->meshParameterSBO->SetData(0, sizeof(MeshParameter) * offset, context->meshParameters);

            glPolygonOffset(2, 0);

            // インスタンシング描画
            for (auto& [id, data] : context->shadowDrawData)
            {
                // ストレージバッファデータのインスタンスオフセットを指定する
                context->shadowShader->Set("instanceOffset", context->ShadowParameterData[id].offset);

                data.source->Bind();
                Renderer::Get()->DrawIndexedInstance(data.meshAsset->GetPrimitiveType(), data.indexCount, data.instanceCount);

                context->stats.numShadowDrawCall++;
            }

            glPolygonOffset(0, 0);
        }
    }

    //==================================
    // Gバッファーパス
    //==================================
    void SceneRenderer::GeometryPass()
    {
        context->gBufferFB->Bind();
        context->gBufferFB->Clear();
        context->gBufferFB->ClearAttachment(4, glm::ivec4(0, -1, 0, 0 )); // IDバッファクリア

        Renderer::Get()->SetStencilFunc(RHI::StrencilOp::Always, 1, 0xFF);
        
        if (context->shouldRenderGeometry)
        {
            SL_SCOPE_PROFILE("GBufferPass");

            uint32 offset = 0;

            {
                SL_SCOPE_PROFILE("Calculate MeshParameter");

                for (auto& data : context->meshDrawList)
                {
                    Shared<Mesh> mesh   = data.mesh.mesh;
                    auto& materialTable = data.mesh.materials;

                    if (mesh)
                    {
                        uint32 sourceIndex = 0;
                        for (MeshSource* meshSource : mesh->GetMeshSources())
                        {
                            glm::mat4 ts   = data.transform * meshSource->GetTransform();
                            auto& material = materialTable[meshSource->GetMaterialIndex()];

                            if (!material)
                                material = Renderer::Get()->GetDefaultMaterial();

                            // メッシュのアセットIDから新規・既存メッシュを判定する
                            InstancingUnitID unit = { mesh->GetAssetID(), sourceIndex, material->GetAssetID() };

                            // インスタンス毎のデータ
                            auto& param        = context->meshParameterData[unit].parameters.emplace_back();
                            param.transform    = ts;
                            param.normalMatrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(ts))));
                            param.pixelID[0]   = data.entityID;
                            param.pixelID[1]   = material->ShadingModel;

                            // メッシュ毎の描画データ
                            InstancingUnitData& unitdata = context->meshDrawData[unit];
                            unitdata.instanceCount++;
                            unitdata.indexCount  = meshSource->HasIndex() ? meshSource->GetIndexCount() : 0;
                            unitdata.vertexCount = meshSource->GetVertexCount();
                            unitdata.meshAsset   = mesh.Get();
                            unitdata.material    = material.Get();
                            unitdata.source      = meshSource;

                            sourceIndex++;
                        }
                    }
                }

                // インスタンスごとの描画データをシェーダーで扱うデータに整列する
                for (auto& [id, param] : context->meshParameterData)
                {
                    param.offset = offset;
                    for (const auto& param : param.parameters)
                    {
                        context->meshParameters[offset].transform    = param.transform;
                        context->meshParameters[offset].pixelID.x    = param.pixelID.x;
                        context->meshParameters[offset].pixelID.y    = param.pixelID.y;
                        context->meshParameters[offset].normalMatrix = param.normalMatrix;
                        offset++;
                    }
                }
            }

            // 全データを転送
            context->meshParameterSBO->SetData(0, sizeof(MeshParameter) * offset, context->meshParameters);

            // 座標変換 uniform
            context->gBufferShader->Bind();
            context->gBufferShader->Set("projection", context->sceneCamera->GetProjectionMatrix());
            context->gBufferShader->Set("view",       context->sceneCamera->GetViewMatrix());
            context->gBufferShader->Set("albedoMap",  0);

            // インスタンシング描画
            for (auto& [id, data] : context->meshDrawData)
            {
                const auto& material = data.material;

                // ストレージバッファデータのインスタンスオフセットを指定する
                context->gBufferShader->Set("instanceOffset", context->meshParameterData[id].offset);
#if 1
                context->gBufferShader->Set("albedo",        material->Albedo);
                context->gBufferShader->Set("emission",      material->Emission);
                context->gBufferShader->Set("metallic",      material->Metallic);
                context->gBufferShader->Set("roughness",     material->Roughness);
                context->gBufferShader->Set("textureTiling", material->TextureTiling);
#else
                MaterialUBO ubo;
                ubo.albedo        = material->albedo;
                ubo.emission      = material->emission;
                ubo.metallic      = material->metallic;
                ubo.roughness     = material->roughness;
                ubo.textureTiling = material->textureTiling;

                m_Context->MaterialUBO->SetData(0, sizeof(MaterialUBO), &ubo);
#endif

                uint32 albedoID = material->AlbedoMap ? material->AlbedoMap->GetID() : 1;
                Renderer::Get()->SetShaderTexture(0, albedoID);

                data.source->Bind();
                Renderer::Get()->DrawIndexedInstance(data.meshAsset->GetPrimitiveType(), data.indexCount, data.instanceCount);

                context->stats.numGeometryDrawCall++;
            }
        }
    }

    //==================================
    // ライティングパス
    //==================================
    void SceneRenderer::DeferredLightinglPass()
    {
        context->deferredFB->Bind();
        context->deferredFB->Clear();

        // Gバッファの深度ステンシル -> デファードライティングにコピー
        Renderer::Get()->BlitFramebuffer(context->gBufferFB, context->deferredFB, RHI::AttachmentBuffer::Stencil);

        // ステンシル値が1の場所に書き込み
        Renderer::Get()->SetStencilFunc(RHI::StrencilOp::Equal, 1, 0xFF);

        // ジオメトリ
        if (context->shouldRenderGeometry)
        {
            SL_SCOPE_PROFILE("DeferredLightingPass");

            // ライティング情報
            int32 numCascades = context->shadowCascadeLevels.size();
            context->deferredLightingShader->Bind();
            context->deferredLightingShader->Set("view",                  context->sceneCamera->GetViewMatrix());
            context->deferredLightingShader->Set("camPos",                context->sceneCamera->GetPosition());
            context->deferredLightingShader->Set("lightDir",              context->directionalLight.direction);
            context->deferredLightingShader->Set("lightColor",            context->directionalLight.color * context->directionalLight.intencity * context->intencityMultiplication);
            context->deferredLightingShader->Set("shadowDepthBias",       context->directionalLight.shadowDepthBias);
            context->deferredLightingShader->Set("enableSoftShadow",      context->directionalLight.enableSoftShadow);
            context->deferredLightingShader->Set("farPlane",              context->sceneCamera->GetFarPlane());
            context->deferredLightingShader->Set("cascadeCount",          numCascades);
            context->deferredLightingShader->Set("showCascade",           context->directionalLight.showCascade);
            context->deferredLightingShader->Set("cascadePlaneDistances", context->shadowCascadeLevels.data(), numCascades);
            context->deferredLightingShader->Set("iblIntencity",          context->skyLight.intencity);

            // Gバッファのカラー情報をバインド
            context->deferredLightingShader->Set("irradianceMap",    0); // irradiance
            context->deferredLightingShader->Set("prefilterMap",     1); // prefilter
            context->deferredLightingShader->Set("brdfLUT",          2); // BRDF
            context->deferredLightingShader->Set("cascadeshadowMap", 3); // シャドウマップ
            context->deferredLightingShader->Set("albedoMap",        4); // アルベド
            context->deferredLightingShader->Set("normalMap",        5); // 法線
            context->deferredLightingShader->Set("positionMap",      6); // 座標
            context->deferredLightingShader->Set("emissionMap",      7); // エミッション
            context->deferredLightingShader->Set("idMap",            8); // マテリアルID

            if (context->skyLight.enableIBL && context->skyLight.sky)
            {
                Renderer::Get()->SetShaderTexture(0, context->skyLight.sky->GetIrradianceMap());
                Renderer::Get()->SetShaderTexture(1, context->skyLight.sky->GetPrefilterMap());
                Renderer::Get()->SetShaderTexture(2, context->skyLight.sky->GetBRDF());
            }
            else
            {
                Renderer::Get()->SetShaderTexture(0, 0);
                Renderer::Get()->SetShaderTexture(1, 0);
                Renderer::Get()->SetShaderTexture(2, 0);
            }

            context->shadowMapFB->BindAttachment(3, 0);
            context->gBufferFB->BindAttachment(4, 0);
            context->gBufferFB->BindAttachment(5, 1);
            context->gBufferFB->BindAttachment(6, 2);
            context->gBufferFB->BindAttachment(7, 3);
            context->gBufferFB->BindAttachment(8, 4);

            Renderer::Get()->DrawScreenQuad();
        }
    }

    //==================================
    // スカイボックス
    //==================================
    void SceneRenderer::SkyboxPass()
    {
        // ステンシル値が0の場所にのみ描画を許可
        Renderer::Get()->SetStencilFunc(RHI::StrencilOp::Equal, 0, 0xFF);

        // スカイボックスパス
        if (context->skyLight.renderSky)
        {
            SL_SCOPE_PROFILE("SkyboxPass");

            Renderer::Get()->SetCullFace(RHI::CullFace::Front);

            context->skyShader->Bind();
            context->skyShader->Set("view",           glm::mat4(glm::mat3(context->sceneCamera->GetViewMatrix())));
            context->skyShader->Set("projection",     context->sceneCamera->GetProjectionMatrix());
            context->skyShader->Set("environmentMap", 0);

            Renderer::Get()->SetShaderTexture(0, context->skyLight.sky ? context->skyLight.sky->GetCubeMap() : 0);
            Renderer::Get()->DrawCube();

            Renderer::Get()->SetCullFace(RHI::CullFace::Back);
        }
    }

    //==================================
    // フォワードパス
    //==================================
    void SceneRenderer::ForwardLightinglPass()
    {

    }

    //==================================
    // ポストプロセス
    //==================================
    void SceneRenderer::PostProcessPass()
    {
        if (context->enablePostProcess)
        {
            Renderer::Get()->BlitFramebuffer(context->deferredFB, context->temporaryFB, RHI::AttachmentBuffer::Color);
            Renderer::Get()->BlitFramebuffer(context->deferredFB, context->finalPassFB, RHI::AttachmentBuffer::Color);

            SceneRenderOption option = context->option;

            // Outline
            if (option.postProcess.enableOutline)
                OutlinePass();

            // 色縮差
            if (option.postProcess.enableChromaticAberration)
                ChromaticAberrationPass();

            // ブルーム
            if (option.postProcess.enableBloom)
                BloomPass();

            // FXAA
            if (option.postProcess.enableFXAA)
                FXAAPass();

            // トーンマッピング
            if (option.postProcess.enableTonemap)
                TonemapPass();
        }
    }

    //==================================
    // ブルームパス
    //==================================
    void SceneRenderer::BloomPass()
    {
        SL_SCOPE_PROFILE("BloomPass");

        SceneRenderOption option = context->option;
        context->bloomFB->Bind();
        context->bloomFB->Clear();

        // プリフィルター（明度のしきい値を適応）
        BloomPreFiltering();

        // ダウンサンプリング
        BloomDownSampling();

        // アップサンプリング
        BloomUpSampling();

        context->finalPassFB->Bind();
        context->bloomShader->Bind();
        context->bloomShader->Set("srcTexture", 0);
        context->bloomShader->Set("bloomBlur",  1);
        context->bloomShader->Set("intencity",  option.postProcess.bloomIntencity);

        // シーンFBとブラーFBを補完してブルームをかける
        context->temporaryFB->BindAttachment(0, 0);
        context->bloomFB->BindAttachment(1, 0);

        Renderer::Get()->DrawScreenQuad();
        Renderer::Get()->BlitFramebuffer(context->finalPassFB, context->temporaryFB, RHI::AttachmentBuffer::Color);
    }

    void SceneRenderer::BloomPreFiltering()
    {
        SceneRenderOption option = context->option;

        // シェーダー
        context->preDownSamplingShader->Bind();
        context->preDownSamplingShader->Set("threshold", option.postProcess.bloomThreshold);

        // シーンテクスチャをシェーダーにセット
        context->temporaryFB->BindAttachment(0, 0);

        // プリフィルタテクスチャをFBにバインド
        context->bloomFB->SetAttachmentTexture(0, context->preDownSamplingTexture->GetID(), RHI::AttachmentType::Color);

        // 描画
        Renderer::Get()->SetViewport(context->viewportSize.x, context->viewportSize.y);
        Renderer::Get()->DrawScreenQuad();
    }

    void SceneRenderer::BloomDownSampling()
    {
        context->downSamplingShader->Bind();
        context->downSamplingShader->Set("srcResolution", context->viewportSize);

        // プリフィルター後のシーンテクスチャを
        Renderer::Get()->SetShaderTexture(0, context->preDownSamplingTexture->GetID());

        for (int i = 0; i < context->bloomResolutions.size(); i++)
        {
            auto& resolution = context->bloomResolutions[i];
            uint32 textureID = context->bloomTextures[i]->GetID();

            // 書き込み先テクスチャをフレームバッファにアタッチ
            context->bloomFB->SetAttachmentTexture(0, textureID, RHI::AttachmentType::Color);

            //描画
            Renderer::Get()->SetViewport(resolution.x, resolution.y);
            Renderer::Get()->DrawScreenQuad();

            // ダウンサンプリングしたテクスチャを次の書き込み先として設定
            Renderer::Get()->SetShaderTexture(0, textureID);
            context->downSamplingShader->Set("srcResolution", resolution);
        }
    }

    void SceneRenderer::BloomUpSampling()
    {
        float bloomFilterRadius = 0.01f;
        context->upSamplingShader->Bind();
        context->upSamplingShader->Set("filterRadius", bloomFilterRadius);

        Renderer::Get()->EnableBlend(true);

        for (int i = context->bloomResolutions.size() - 1; i > 0; i--)
        {
            auto& resolution = context->bloomResolutions[i - 1];
            uint32 textureID = context->bloomTextures[i - 1]->GetID();

            // ダウンサンプリング先のテクスチャをアタッチ
            context->bloomFB->SetAttachmentTexture(0, textureID, RHI::AttachmentType::Color);

            // 1個前のダウンサンプリングされたテクスチャをソースに
            Renderer::Get()->SetShaderTexture(0, context->bloomTextures[i]->GetID());
            Renderer::Get()->SetViewport(resolution.x, resolution.y);
            Renderer::Get()->DrawScreenQuad();
        }

        Renderer::Get()->EnableBlend(false);
    }

    //==================================
    // FXAAパス
    //==================================
    void SceneRenderer::FXAAPass()
    {
        SL_SCOPE_PROFILE("FXAAPass");

        context->fxaaShader->Bind();
        context->finalPassFB->Bind();
        context->temporaryFB->BindAttachment(0, 0);

        Renderer::Get()->DrawScreenQuad();
        Renderer::Get()->BlitFramebuffer(context->finalPassFB, context->temporaryFB, RHI::AttachmentBuffer::Color);
    }

    //==================================
    //　アウトラインパス
    //==================================
    void SceneRenderer::OutlinePass()
    {
        SL_SCOPE_PROFILE("OutlinePass");

        SceneRenderOption option = context->option;
        context->outlineShader->Bind();
        context->outlineShader->Set("screenTexture", 0);
        context->outlineShader->Set("normalTexture", 1);
        context->outlineShader->Set("depthTexture", 2);
        context->outlineShader->Set("lineWidth",     option.postProcess.lineWidth);
        context->outlineShader->Set("outlineColor",  option.postProcess.outlineColor);

        context->finalPassFB->Bind();
        context->temporaryFB->BindAttachment(0, 0);
        context->gBufferFB->BindAttachment(1, 1);
        context->gBufferFB->BindAttachment(2, 2);

        Renderer::Get()->DrawScreenQuad();
        Renderer::Get()->BlitFramebuffer(context->finalPassFB, context->temporaryFB, RHI::AttachmentBuffer::Color);
    }

    //==================================
    // 色縮差パス
    //==================================
    void SceneRenderer::ChromaticAberrationPass()
    {
        SL_SCOPE_PROFILE("ChromaticAberrationPass");

        context->chromaticAberrationShader->Bind();
        context->finalPassFB->Bind();
        context->temporaryFB->BindAttachment(0, 0);

        Renderer::Get()->DrawScreenQuad();
        Renderer::Get()->BlitFramebuffer(context->finalPassFB, context->temporaryFB, RHI::AttachmentBuffer::Color);
    }

    //==================================
    // トーンマップパス
    //==================================
    void SceneRenderer::TonemapPass()
    {
        SL_SCOPE_PROFILE("TonemapPass");

        SceneRenderOption option = context->option;
        context->tonemapShader->Bind();
        context->tonemapShader->Set("exposure",        option.postProcess.exposure);
        context->tonemapShader->Set("gammaCorrection", option.postProcess.gammaCorrection);

        context->finalPassFB->Bind();
        context->temporaryFB->BindAttachment(0, 0);

        Renderer::Get()->DrawScreenQuad();
        Renderer::Get()->BlitFramebuffer(context->finalPassFB, context->temporaryFB, RHI::AttachmentBuffer::Color);
    }

    uint32 SceneRenderer::GetFinalRenderPassID()
    {
        return context->enablePostProcess? context->finalPassFB->GetAttachmentID(0) : context->gBufferFB->GetAttachmentID(0);
    }

    int32 SceneRenderer::ReadEntityIDFromPixcel(uint32 x, uint32 y)
    {
#if SL_PLATFORM_OPENGL
        // OpenGL: UV座標 反転
        float height = context->viewportSize.y - y;
#else
        float height = y;
#endif

        context->gBufferFB->Bind();
        glm::ivec4 p = context->gBufferFB->ReadPixelInt(4, x, height);

        return p.g;
    }

    std::array<glm::ivec2, 6> SceneRenderer::CalculateBloomMipSize(uint32 width, uint32 height)
    {
        std::array<glm::ivec2, 6> result = {};
        glm::vec2 resolution = { width, height };

        for (int32 i = 0; i < 6; i++)
        {
            resolution *= 0.5f;
            result[i]  = resolution;
        }

        return result;
    }

    std::array<glm::vec4, 8> SceneRenderer::GetFrustumCornersWorldSpace(const glm::mat4& projview)
    {
        const glm::mat4 inv = glm::inverse(projview);
        std::array<glm::vec4, 8> frustumCorners;
        uint32 index = 0;

        for (uint32 x = 0; x < 2; ++x)
        {
            for (uint32 y = 0; y < 2; ++y)
            {
                for (uint32 z = 0; z < 2; ++z)
                {
                    const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    frustumCorners[index] = (pt / pt.w);
                    index++;
                }
            }
        }

        return frustumCorners;
    }

    glm::mat4 SceneRenderer::GetLightSpaceMatrix(glm::vec3 directionalLightDir, const Camera& camera, const float nearPlane, const float farPlane)
    {
        auto proj    = glm::perspective(glm::radians(camera.GetFOV()), context->viewportSize.x / context->viewportSize.y, nearPlane, farPlane);
        auto corners = GetFrustumCornersWorldSpace(proj * camera.GetViewMatrix());

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

    std::array<glm::mat4, 4> SceneRenderer::GetLightSpaceMatrices(glm::vec3 directionalLightDir, const Camera& camera)
    {
        auto nearP  = camera.GetNearPlane();
        auto farP   = camera.GetFarPlane();
        auto levels = context->shadowCascadeLevels;
        
        std::array<glm::mat4, 4> result;
        result[0] = (GetLightSpaceMatrix(directionalLightDir, camera, nearP,     levels[0]));
        result[1] = (GetLightSpaceMatrix(directionalLightDir, camera, levels[0], levels[1]));
        result[2] = (GetLightSpaceMatrix(directionalLightDir, camera, levels[1], levels[2]));
        result[3] = (GetLightSpaceMatrix(directionalLightDir, camera, levels[2], farP));

        return result;
    }
}
