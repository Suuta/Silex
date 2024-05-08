
#include "PCH.h"

#include "Core/Random.h"
#include "Asset/Asset.h"
#include "Serialize/Serialize.h"
#include "Serialize/AssetSerializer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Material.h"

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
    void AssetSerializer<Material>::Serialize(const Shared<Material>& aseet, const std::string& filePath)
    {
        AssetID albedoAssetID = Renderer::Get()->GetDefaultTexture()->GetAssetID();
        if (aseet->AlbedoMap) albedoAssetID = aseet->AlbedoMap->GetAssetID();

        YAML::Emitter out;

        out << YAML::BeginMap;
        out << YAML::Key << "shadingModel"  << YAML::Value << aseet->ShadingModel;
        out << YAML::Key << "albedo"        << YAML::Value << aseet->Albedo;
        out << YAML::Key << "albedoMap"     << YAML::Value << albedoAssetID;
        out << YAML::Key << "emission"      << YAML::Value << aseet->Emission;
        out << YAML::Key << "metallic"      << YAML::Value << aseet->Metallic;
        out << YAML::Key << "roughness"     << YAML::Value << aseet->Roughness;
        out << YAML::Key << "textureTiling" << YAML::Value << aseet->TextureTiling;
        out << YAML::EndMap;

        std::ofstream fout(filePath);
        fout << out.c_str();
        fout.close();
    }

    template<>
    Shared<Material> AssetSerializer<Material>::Deserialize(const std::string& filePath)
    {
        Shared<Material> material = CreateShared<Material>();

        YAML::Node data = YAML::LoadFile(filePath);

        auto shadingModel  = data["shadingModel"].as<int32>();
        auto albedo        = data["albedo"].as<glm::vec3>();
        auto albedoMap     = data["albedoMap"].as<AssetID>();
        auto emission      = data["emission"].as<glm::vec3>();
        auto metallic      = data["metallic"].as<float>();
        auto roughness     = data["roughness"].as<float>();
        auto textureTiling = data["textureTiling"].as<glm::vec2>();

        material->ShadingModel  = (ShadingModelType)shadingModel;
        material->Emission      = emission;
        material->Metallic      = metallic;
        material->Roughness     = roughness;
        material->TextureTiling = textureTiling;
        material->Albedo        = albedo;

        // テクスチャ読み込み完了を前提とする
        const Shared<Texture2D>& texture = AssetManager::Get()->GetAssetAs<Texture2D>(albedoMap);
        material->AlbedoMap = texture;

        return material;
    }
}