
#pragma once

#include "Core/Core.h"
#include "Core/Ref.h"
#include "Serialize/AssetSerializer.h"


namespace Silex
{
    class Material;
    class Scene;
    class Mesh;

    enum class AssetType : uint32;


    // 型Tで部分特殊化、指定の型以外は不適格
    template<class T, class... Args>
    class AssetCreator
    {
        static_assert(sizeof(T) == 0, "無効な型です");
    };

    template<class... Args>
    class AssetCreator<Material, Args...>
    {
    public:

        static Ref<Material> Create(const std::filesystem::path& directory, Args&&... args)
        {
            Ref<Material> asset = CreateRef<Material>(Traits::Forward<Args>(args)...);
            asset->SetupAssetProperties(directory.string(), AssetType::Material);

            //TODO: アセットマネージャーから取得するように変更
            //asset->AlbedoMap = Renderer::Get()->GetDefaultTexture();

            AssetSerializer<Material>::Serialize(asset, directory.string());

            return asset;
        }
    };
}
