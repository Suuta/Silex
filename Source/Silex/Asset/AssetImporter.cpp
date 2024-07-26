
#include "PCH.h"

#include "Asset/AssetImporter.h"
#include "Serialize/AssetSerializer.h"
#include "Rendering/Mesh.h"
#include "Rendering/SkyLight.h"
#include "Rendering/Texture.h"
#include "Core/Random.h"


namespace Silex
{
    template<>
    Shared<Mesh> AssetImporter::Import<Mesh>(const std::string& filePath)
    {
        Mesh* m = slnew(Mesh);
        m->Load(filePath);

        m->SetupAssetProperties(filePath, AssetType::Mesh);
        return Shared<Mesh>(m);
    }

    template<>
    Shared<Texture2D> AssetImporter::Import<Texture2D>(const std::string& filePath)
    {
        RHI::TextureDesc desc = {};
        desc.Filter    = RHI::TextureFilter::Linear;
        desc.Wrap      = RHI::TextureWrap::Repeat;
        desc.GenMipmap = true;
        Shared<Texture2D> t = Texture2D::Create(desc, filePath);

        t->SetupAssetProperties(filePath, AssetType::Texture2D);

        return t;
    }

    template<>
    Shared<SkyLight> AssetImporter::Import<SkyLight>(const std::string& filePath)
    {
        Shared<SkyLight> s = SkyLight::Create(filePath);
        s->SetupAssetProperties(filePath, AssetType::SkyLight);

        return s;
    }

    template<>
    Shared<Material> AssetImporter::Import<Material>(const std::string& filePath)
    {
        // マテリアルはデータファイルではなくパラメータファイルなのでデシリアライズしたファイルから生成する
        // 逆に、シリアライズはエディターの保存時に行う
        Shared<Material> m = AssetSerializer<Material>::Deserialize(filePath);
        m->SetupAssetProperties(filePath, AssetType::Material);

        return m;
    }
}
