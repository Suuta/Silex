
#include "PCH.h"

#include "Core/Random.h"
#include "Asset/Asset.h"
#include "Serialize/Serialize.h"
#include "Serialize/AssetSerializer.h"
#include "Rendering/Material.h"
#include "Rendering/Renderer.h"

/*
    MeshComponent:
      Mesh: 2
      Material :
        ShadingModel : 1
        Table :
          0 :
            Albedo: [1, 1, 1]
            AlbedoMap: 2
            Emission: [0, 0, 0]
            Metallic: 1
            Roughness: 1
            TextureTiling: [1, 1]
            CastShadow: true
          1 :
            Albedo: [1, 1, 1]
            AlbedoMap: 2
            Emission: [0, 0, 0]
            Metallic: 1
            Roughness: 1
            TextureTiling: [1, 1]
            CastShadow: true

        ↓

    // マテリアル
    ShadingModel: 1
    Albedo: [1, 1, 1]
    AlbedoMap : 0
    Emission: [0, 0, 0]
    Metallic: 1
    Roughness: 1
    TextureTiling: [1, 1]
    CastShadow: true

    ShadingModel: 1
    Albedo: [1, 1, 1]
    AlbedoMap: 0
    Emission: [0, 0, 0]
    Metallic: 1
    Roughness: 1
    TextureTiling: [1, 1]
    CastShadow: true

    // シーン
    MeshComponent:
      Mesh: 2
      Material:
        0: 5372816381729 // assetID
        1: 7894653678935 // assetID
        ...
*/


namespace Silex
{
    template<>
    void AssetSerializer<MaterialAsset>::Serialize(const Ref<MaterialAsset>& aseet, const std::string& filePath)
    {
        //AssetID albedoAssetID = Renderer::Get()->GetDefaultTexture()->GetAssetID();
        //if (aseet->AlbedoMap)
        //    albedoAssetID = aseet->AlbedoMap->GetAssetID();

        YAML::Emitter out;

        out << YAML::BeginMap;
        out << YAML::Key << "shadingModel"  << YAML::Value << aseet->Get()->ShadingModel;
        out << YAML::Key << "albedo"        << YAML::Value << aseet->Get()->Albedo;
        //out << YAML::Key << "albedoMap"     << YAML::Value << albedoAssetID;
        out << YAML::Key << "emission"      << YAML::Value << aseet->Get()->Emission;
        out << YAML::Key << "metallic"      << YAML::Value << aseet->Get()->Metallic;
        out << YAML::Key << "roughness"     << YAML::Value << aseet->Get()->Roughness;
        out << YAML::Key << "textureTiling" << YAML::Value << aseet->Get()->TextureTiling;
        out << YAML::EndMap;

        std::ofstream fout(filePath);
        fout << out.c_str();
        fout.close();
    }

    template<>
    Ref<MaterialAsset> AssetSerializer<MaterialAsset>::Deserialize(const std::string& filePath)
    {
        Material* material = slnew(Material);
        Ref<MaterialAsset> asset = CreateRef<MaterialAsset>(material);


        YAML::Node data = YAML::LoadFile(filePath);

        auto shadingModel  = data["shadingModel"].as<int32>();
        auto albedo        = data["albedo"].as<glm::vec3>();
        auto albedoMap     = data["albedoMap"].as<AssetID>();
        auto emission      = data["emission"].as<glm::vec3>();
        auto metallic      = data["metallic"].as<float>();
        auto roughness     = data["roughness"].as<float>();
        auto textureTiling = data["textureTiling"].as<glm::vec2>();

        asset->Get()->ShadingModel  = (ShadingModelType)shadingModel;
        asset->Get()->Emission      = emission;
        asset->Get()->Metallic      = metallic;
        asset->Get()->Roughness     = roughness;
        asset->Get()->TextureTiling = textureTiling;
        asset->Get()->Albedo        = albedo;

        // テクスチャ読み込み完了を前提とする
        const Ref<Texture2DAsset>& texture = AssetManager::Get()->GetAssetAs<Texture2DAsset>(albedoMap);
        asset->Get()->AlbedoMap = texture;

        return asset;
    }
}