
#include "PCH.h"

#include "Asset/AssetImporter.h"
#include "Serialize/AssetSerializer.h"
#include "Rendering/Mesh.h"
#include "Rendering/Environment.h"
#include "Rendering/RenderingStructures.h"
#include "Rendering/Renderer.h"
#include "Core/Random.h"


namespace Silex
{
    template<>
    Ref<MeshAsset> AssetImporter::Import<MeshAsset>(const std::string& filePath)
    {
        Mesh* mesh = slnew(Mesh);
        mesh->Load(filePath);

        Ref<MeshAsset> asset = CreateRef<MeshAsset>(mesh);
        asset->SetupAssetProperties(filePath, AssetType::Mesh);

        return asset;
    }

    template<>
    Ref<Texture2DAsset> AssetImporter::Import<Texture2DAsset>(const std::string& filePath)
    {
        TextureHandle* t = Renderer::Get()->CreateTextureFromMemory((const float*)nullptr, 1, 0, 0, true);
        Texture2D* texture = nullptr;

        Ref<Texture2DAsset> asset = CreateRef<Texture2DAsset>(texture);
        asset->SetupAssetProperties(filePath, AssetType::Texture);

        return asset;
    }

    template<>
    Ref<EnvironmentAsset> AssetImporter::Import<EnvironmentAsset>(const std::string& filePath)
    {
        Environment* environment = nullptr;
        // environment = Renderer::Get()->CreateEnvironment(filePath);

        Ref<EnvironmentAsset> asset = CreateRef<EnvironmentAsset>(environment);
        asset->SetupAssetProperties(filePath, AssetType::Environment);

        return asset;
    }

    template<>
    Ref<MaterialAsset> AssetImporter::Import<MaterialAsset>(const std::string& filePath)
    {
        // マテリアルはデータファイルではなくパラメータファイルなのでデシリアライズしたファイルから生成する
        // 逆に、シリアライズはエディターの保存時に行う
        Ref<MaterialAsset> asset = AssetSerializer<MaterialAsset>::Deserialize(filePath);
        asset->SetupAssetProperties(filePath, AssetType::Material);

        return asset;
    }
}
