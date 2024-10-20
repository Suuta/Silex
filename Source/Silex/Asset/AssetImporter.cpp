
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
    Ref<Mesh> AssetImporter::Import<Mesh>(const std::string& filePath)
    {
        Mesh* m = slnew(Mesh);
        m->Load(filePath);

        m->SetupAssetProperties(filePath, AssetType::Mesh);
        return Ref<Mesh>(m);
    }

    template<>
    Ref<Texture2D> AssetImporter::Import<Texture2D>(const std::string& filePath)
    {
        TextureHandle* t = Renderer::Get()->CreateTextureFromMemory((const float*)nullptr, 1, 0, 0, true);

        Ref<Texture2DAsset> asset = CreateRef<Texture2DAsset>(t);
        asset->SetupAssetProperties(filePath, AssetType::Texture);

        return asset;
    }

    template<>
    Ref<Environment> AssetImporter::Import<Environment>(const std::string& filePath)
    {
        Ref<Environment> s = Environment::Create(filePath);
        s->SetupAssetProperties(filePath, AssetType::Environment);

        return s;
    }

    template<>
    Ref<Material> AssetImporter::Import<Material>(const std::string& filePath)
    {
        // マテリアルはデータファイルではなくパラメータファイルなのでデシリアライズしたファイルから生成する
        // 逆に、シリアライズはエディターの保存時に行う
        Ref<Material> m = AssetSerializer<Material>::Deserialize(filePath);
        m->SetupAssetProperties(filePath, AssetType::Material);

        return m;
    }
}
