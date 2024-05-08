
#pragma once

#include "Core/CoreType.h"
#include "Scene/Scene.h"


//==========================================================
// インスタンシングデータ構築
//==========================================================
namespace Silex
{
    // numMaxInstancing（8192インスタンス）までメッシュ毎にインスタンス可能
    struct MeshParameter
    {
        glm::mat4  transform;
        glm::mat4  normalMatrix;
        glm::ivec4 pixelID;
    };

    // 描画パラメータのインスタンシングデータ
    struct InstancingUnitParameter
    {
        std::vector<MeshParameter> parameters;
        int32                      offset = 0;
    };

    // メッシュのインスタンシングデータ
    struct InstancingUnitData
    {
        Mesh*        meshAsset     = nullptr; // メッシュデータ
        MeshSource*  source        = nullptr; // メッシュの頂点データ
        Material*    material      = nullptr; // マテリアル
        uint32       instanceCount = 0;       // インスタンス数
        uint32       indexCount    = 0;       // 合計頂点インデックス数
        uint32       vertexCount   = 0;       // 合計頂点数
    };

    // インスタンス化可能メッシュデータの検索キー（std::unordered_map のキー）
    struct InstancingUnitID
    {
        AssetID meshID;
        uint32  sourceIndex;
        AssetID material;

        InstancingUnitID(AssetID meshID, uint32 sourceIndex, AssetID materialHandle)
            : meshID(meshID)
            , sourceIndex(sourceIndex)
            , material(materialHandle)
        {
        }

        bool operator==(const InstancingUnitID& other) const
        {
            return meshID == other.meshID && sourceIndex == other.sourceIndex && material == other.material;
        }
    };
}

//==========================================================
// std::hash 特殊化
//==========================================================
namespace std
{
    template<>
    struct hash<Silex::InstancingUnitID>
    {
        std::size_t operator()(const Silex::InstancingUnitID& unit) const
        {
            // MeshID は ユニークIDなのでそのままハッシュ値として利用
            return std::hash<Silex::AssetID>()(unit.meshID + unit.sourceIndex);
        }
    };
}


//==========================================================
// デファードシーンレンダラー　描画データ
//==========================================================
namespace Silex
{
    class Shader;
    class Texture2D;
    class SkyLight;
    class UniformBuffer;
    class StorageBuffer;
    class VertexBuffer;
    class Framebuffer;

    struct SceneRenderOption
    {
        PostProcessComponent postProcess;
    };

    struct SceneRenderStats
    {
        uint32 numRenderMesh       = 0;
        uint64 numGeometryDrawCall = 0;
        uint64 numShadowDrawCall   = 0;
    };

    struct MaterialUBO
    {
        glm::vec3 albedo;
        float     metallic;
        glm::vec3 emission;
        float     roughness;
        glm::vec2 textureTiling;
    };

    struct SceneRenderingContext
    {
        static inline const int32 numCascadeLevel  = 4;
        static inline const int32 numBloomSampling = 6;
        static inline const int32 numMaxInstancing = 8192;

        //========================================================
        // ライト
        //========================================================
        SkyLightComponent         skyLight;
        DirectionalLightComponent directionalLight;

        // シーン情報
        glm::vec2 viewportSize = { 1280.f, 720.f };
        Scene*    renderScene  = nullptr;
        Camera*   sceneCamera  = nullptr;

        // Gバッファデバッグ表示
        // glm::vec2 DebguViewportSize = {  640.f, 360.f };
        // bool EnableDebug = false;

        // シャドウカスケード距離
        std::array<float, numCascadeLevel> shadowCascadeLevels = { 10.0f, 40.0f, 100.0f, 200.0f };

        // シャドウカスケードUBO
        Shared<UniformBuffer> cascadeUBO;

        // マテリアルUBO
        Shared<UniformBuffer> materialUBO;

        //========================================================
        // フレームバッファ・リソース
        //========================================================
        Shared<Framebuffer> gBufferFB;
        Shared<Framebuffer> deferredFB;
        Shared<Framebuffer> finalPassFB;
        Shared<Framebuffer> bloomFB;
        Shared<Framebuffer> shadowMapFB;
        Shared<Framebuffer> temporaryFB;

        // ブルームサンプリングテクスチャ　※サイズが同じではないのでフレームバッファのアタッチメントとして扱えず、テクスチャ単体で使用している
        Texture2D*                preDownSamplingTexture = nullptr;
        std::array<Texture2D*, 6> bloomTextures          = {};
        std::array<glm::ivec2, 6> bloomResolutions       = {};

        //========================================================
        // 描画データ
        //========================================================

        // 描画要求されたメッシュコンポーネントリスト
        std::vector<MeshDrawData> meshDrawList;

        // インスタンシング用トランスフォーム
        MeshParameter*        meshParameters = nullptr;
        Shared<StorageBuffer> meshParameterSBO;

        // シャドウインスタンシングデータ
        std::unordered_map<InstancingUnitID, InstancingUnitData>      shadowDrawData;
        std::unordered_map<InstancingUnitID, InstancingUnitParameter> ShadowParameterData;

        // ジオメトリインスタンシングデータ
        std::unordered_map<InstancingUnitID, InstancingUnitData>      meshDrawData;
        std::unordered_map<InstancingUnitID, InstancingUnitParameter> meshParameterData;

        //========================================================
        // シェーダー
        //========================================================
        Shader* shadowShader              = nullptr;
        Shader* deferredLightingShader    = nullptr;
        Shader* gBufferShader             = nullptr;
        Shader* skyShader                 = nullptr;
        Shader* bloomShader               = nullptr;
        Shader* outlineShader             = nullptr;
        Shader* preDownSamplingShader     = nullptr;
        Shader* downSamplingShader        = nullptr;
        Shader* upSamplingShader          = nullptr;
        Shader* fxaaShader                = nullptr;
        Shader* chromaticAberrationShader = nullptr;
        Shader* tonemapShader             = nullptr;


        //========================================================
        // パラメータ・フラグ
        //========================================================

        // ライティングパラメータ
        float intencityMultiplication = 10.0;

        // 描画フラグ
        bool enablePostProcess    = true;
        bool shouldRenderShadow   = false;
        bool shouldRenderGeometry = false;

        SceneRenderStats  stats;
        SceneRenderOption option;
    };


    //==========================================================
    // シーンレンダラー
    //==========================================================
    class SceneRenderer
    {
    public:

        void Init();
        void Shutdown();

        void BeginFrame(Scene* scene, Camera* camera);
        void EndFrame();

        // フレームバッファのリサイズ
        void SetViewportSize(uint32 width, uint32 height);

        // シーンのライト情報をセット
        void SetSkyLight(const SkyLightComponent& data);
        void SetDirectionalLight(const DirectionalLightComponent& data);

        // メッシュデータを描画リストに追加
        void AddMeshDrawList(const MeshDrawData& data);

        // シーンの最終描画結果のテクスチャを取得
        uint32 GetFinalRenderPassID();

        // ピクセルのエンティティIDを取得（Gバッファ）
        int32 ReadEntityIDFromPixcel(uint32 x, uint32 y);

    public:

        SceneRenderStats   GetRenderStats()  { return context->stats;  }
        SceneRenderOption& GetRenderOption() { return context->option; }

    private:

        void ShadowMapPass();
        void GeometryPass();
        void SkyboxPass();
        void DeferredLightinglPass();
        void ForwardLightinglPass();
        void PostProcessPass();

    private:

        void BloomPass();
        void BloomPreFiltering();
        void BloomDownSampling();
        void BloomUpSampling();
        void FXAAPass();
        void OutlinePass();
        void ChromaticAberrationPass();
        void TonemapPass();

    private:

        // ブルームダウンサンプリング用のテクスチャ解像度計算（1 / n^2）
        std::array<glm::ivec2, 6> CalculateBloomMipSize(uint32 width, uint32 height);

        // カスケードレベル毎のライトスペース行列の計算
        std::array<glm::vec4, 8> GetFrustumCornersWorldSpace(const glm::mat4& projview);
        std::array<glm::mat4, 4> GetLightSpaceMatrices(glm::vec3 directionalLightDir, const Camera& camera);
        glm::mat4                GetLightSpaceMatrix(glm::vec3 directionalLightDir, const Camera& camera, const float nearPlane, const float farPlane);

    private:

        // 描画データ
        SceneRenderingContext* context;
    };
}
