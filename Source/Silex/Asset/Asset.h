
#pragma once

#include "Core/Core.h"
#include "Core/Random.h"
#include "Asset/AssetImporter.h"
#include "Asset/AssetCreator.h"


namespace Silex
{
    class Material;



    using AssetID = uint64;

    enum class AssetType : uint32
    {
        None      = 0,
        Scene     = 1,
        Mesh      = 2,
        Texture2D = 4,
        Material  = 8,
        SkyLight  = 16,
    };

    struct AssetMetadata
    {
        AssetID               ID;
        AssetType             Type;
        std::filesystem::path FilePath;
    };

    class Asset : public Object
    {
        SL_DECLARE_CLASS(Asset, Object)

    public:

        bool IsAssetOf(AssetType flag)  { return m_AssetFlag == flag; }
        bool operator==(AssetType flag) { return m_AssetFlag == flag; }
        bool operator!=(AssetType flag) { return m_AssetFlag != flag; }

        // フラグ
        void      SetAssetType(AssetType flag) { m_AssetFlag = flag; }
        AssetType GetAssetType()               { return m_AssetFlag; }

        // ファイルパス
        std::string& GetFilePath()                        { return m_FilePath; }
        void         SetFilePath(const std::string& path) { m_FilePath = path; }

        // ID
        AssetID GetAssetID() const     { return m_AssetID; }
        void    SetAssetID(AssetID id) { m_AssetID = id;   }

        // 名前
        void               SetName(const std::string& name) { m_Name = name; }
        const std::string& GetName() const                  { return m_Name; }

        // プロパティ設定
        void SetupAssetProperties(const std::string& filePath, AssetType flag);

    protected:

        AssetID     m_AssetID   = 0;
        AssetType   m_AssetFlag = AssetType::None;
        std::string m_FilePath  = {};
        std::string m_Name      = {};
    };


    inline AssetType FileNameToAssetType(const std::filesystem::path& filePath)
    {
        std::string extention = filePath.extension().string();

        if (extention == ".slmt")                       return AssetType::Material;
        else if (extention == ".slsc")                       return AssetType::Scene;
        else if (extention == ".fbx" || extention == ".obj") return AssetType::Mesh;
        else if (extention == ".png" || extention == ".jpg") return AssetType::Texture2D;
        else if (extention == ".hdr")                        return AssetType::SkyLight;

        return AssetType::None;
    }


    class AssetManager
    {
    public:

        static void Init();
        static void Shutdown();
        static AssetManager* Get();

    public:

        //=================================
        // アセットID
        //=================================
        uint64 GenerateAssetID();
        bool IsValidID(AssetID id);

        //=================================
        // メタデータ
        //=================================
        bool          HasMetadata(const std::filesystem::path& directory);
        AssetMetadata GetMetadata(const std::filesystem::path& directory);
        AssetMetadata GetMetadata(AssetID id);

        std::unordered_map<AssetID, AssetMetadata>& GetMetadatas();

        //=================================
        // アセット
        //=================================
        bool IsLoaded(const AssetID id);
        std::unordered_map<AssetID, Shared<Asset>>& GetAllAssets();

        template<class T>
        Shared<T> GetAssetAs(const AssetID id)
        {
            return m_AssetData[id].As<T>();
        }

        Shared<Asset> GetAsset(const AssetID id)
        {
            return m_AssetData[id];
        }

        template<class T, class... Args>
        Shared<T> CreateAsset(const std::filesystem::path& directory, Args&&... args)
        {
            //　現状はマテリアルのみサポート (インポートと生成との意味合いが混同しているため)
            static_assert(Traits::IsSame<Material, T>() && Traits::IsBaseOf<Asset, T>());

            AssetMetadata metadata = s_Instance->AddToMetadata(directory);
            Shared<T> asset = AssetCreator<T>::Create(directory, Traits::Forward<Args>(args)...);

            s_Instance->AddToAssetAndID(metadata.ID, asset);
            s_Instance->WriteDatabaseToFile(s_AssetDatabasePath);

            return asset;
        }

        void DeleteAsset(const AssetID id)
        {
            AssetMetadata data = s_Instance->GetMetadata(id);
            std::string path   = data.FilePath.string();

            // アセットファイルを削除
            std::remove(path.c_str());

            // リストから削除
            s_Instance->RemoveFromMetadata(id);
            s_Instance->RemoveFromAsset(id);

            // シリアライズ
            s_Instance->WriteDatabaseToFile(s_AssetDatabasePath);
        }

        template<typename T>
        Shared<Asset> LoadAssetFromFile(const std::string& filePath)
        {
            static_assert(Traits::IsBaseOf<Asset, T>());

            return AssetImporter::Import<T>(filePath);
        }

    private:

        // アセットデータベースファイルからメタデータを読み込む
        void LoadAssetMetaDataFromDatabaseFile(const std::filesystem::path& filePath);

        // 物理ファイルとメタデータと照合しながら、アセットディレクトリ全体を走査
        void InspectAssetDirectory(const std::filesystem::path& directory);
        void InspectRecursive(const std::filesystem::path& directory);

        // アセット・メタデータ追加
        AssetMetadata AddToMetadata(const std::filesystem::path& directory);
        void          AddToAssetAndID(const AssetID id, Shared<Asset> asset);
        void          AddToAsset(Shared<Asset> asset);

        // アセット・メタデータ削除
        void RemoveFromMetadata(const AssetID id);
        void RemoveFromAsset(const AssetID id);

        // アセットデータベースファイルにメタデータを書き込む
        void WriteDatabaseToFile(const std::filesystem::path& directory);

        // メモリにアセットをロードする
        void LoadAssetToMemory(const std::filesystem::path& filePath);

    private:

        uint32 m_BuiltinAssetCount = 0;

        std::unordered_map<AssetID, Shared<Asset>> m_AssetData;
        std::unordered_map<AssetID, AssetMetadata> m_Metadata;

        static inline const char* s_AssetDatabasePath = "Assets/AssetDatabase.yml";
        static inline const char* s_AssetDiectoryPath = "Assets";

        static inline AssetManager* s_Instance;
    };
}
