
#include "PCH.h"

#include "Asset/AssetImporter.h"
#include "Serialize/AssetSerializer.h"
#include "Rendering/Mesh.h"
#include "Rendering/Environment.h"
#include "Rendering/RenderingStructures.h"
#include "Rendering/Renderer.h"
#include "Core/Random.h"
#include "Asset/TextureReader.h"


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
        TextureHandle* textureHandle = nullptr;

        TextureReader reader;
        if (reader.IsHDR(filePath.c_str()))
        {
            float* pixelData = reader.ReadHDR(filePath.c_str());
            textureHandle = Renderer::Get()->CreateTextureFromMemory(pixelData, reader.data.byteSize, reader.data.width, reader.data.height, true);
        }
        else
        {
            byte* pixelData = reader.Read(filePath.c_str());
            textureHandle = Renderer::Get()->CreateTextureFromMemory(pixelData, reader.data.byteSize, reader.data.width, reader.data.height, true);
        }

        // TODO: この部分も、新実装レンダラーのテクスチャ生成関数内で行う
        //==============================================
        Texture2D* texture = slnew(Texture2D);
        texture->SetHandle(textureHandle);
        //==============================================

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
