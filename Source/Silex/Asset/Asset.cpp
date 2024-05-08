
#include "PCH.h"

#include "Asset/Asset.h"
#include "Core/Random.h"
#include "Editor/SplashImage.h"
#include "Renderer/MeshFactory.h"
#include "Renderer/Renderer.h"
#include "Renderer/SkyLight.h"
#include "Renderer/Texture.h"
#include "Serialize/AssetSerializer.h"

#include <yaml-cpp/yaml.h>
#include <numbers>

namespace Silex
{
    void Asset::SetupAssetProperties(const std::string& filePath, AssetType flag)
    {
        SetAssetType(flag);
        SetFilePath(filePath);

        std::filesystem::path path(filePath);
        SetName(path.stem().string());
    }


    //==================================================================================
    // ①アセットデータベースファイルの有無確認
    //----------------------------------------------------------------------------------
    // ＜存在しない＞
    // アセットディレクトリを再帰検索し、すべてのメタデータをデータベースに登録する（IDを生成する）
    // この時点でシーンファイル自体は存在している場合、（データベースファイルのみ存在しない）シーンが参照
    // するアセットIDとデータベースファイルのアセットIDは異なる（IDが新規生成されるため）
    // 
    // ＜存在する＞
    // ファイルからメタデータを読み込んでデータベースに登録する
    // 
    //----------------------------------------------------------------------------------
    // ②メタデータをデータベースファイルに書き込み
    //----------------------------------------------------------------------------------
    // 
    //----------------------------------------------------------------------------------
    // ③任意（未定）タイミングでアセットをメモリに乗せる
    //----------------------------------------------------------------------------------
    // アセットデータベースとアセットIDが異なる場合は読み込まない（AssetFlag::Missing）
    //
    //----------------------------------------------------------------------------------
    // ③シーンの保存
    //----------------------------------------------------------------------------------
    // 読み込まれていないアセットは（AssetFlag::Missing）
    // 再度プロパティから設定し、シーンの保存を行えば、そのIDでシーンファイル内のIDが保存されるので
    // 次回読み込み時は、そのデータベースファイルから参照が成功する
    // 
    //----------------------------------------------------------------------------------
    // ④エディター終了時
    //----------------------------------------------------------------------------------
    // 現在のデータベースをファイルに書き込む
    // 現在のIDを書き込むため、新規データベースファイルを使った次回起動時に正しく読み込まれない場合がある
    // 参照するアセットIDに不整合やシーンの保存・再適応をしない限りアセットはMissingのままであり、
    // 読み込みはスキップされる
    //
    //==================================================================================
    // 検討中: アセットを実際にメモリに乗せるタイミングは　シーン読み込み時 / エディター起動時 ？
    //==================================================================================



    AssetManager* AssetManager::Get()
    {
        return s_Instance;
    }

    void AssetManager::Init()
    {
        SL_ASSERT(s_Instance == nullptr)
        s_Instance = Memory::Allocate<AssetManager>();

        // ビルトインデータ(ID: 1 - 5 に割り当て)
        // アセットデータベースには登録されない（メモリオンリーアセット）
        {
            auto& tex = Renderer::Get()->GetDefaultTexture();
            tex->SetAssetType(AssetType::Texture2D);
            tex->SetName("Default");
            AssetManager::Get()->AddToAssetAndID(1, tex);
            s_Instance->m_BuiltinAssetCount++;

            auto& checker = Renderer::Get()->GetCheckerboardexture();
            checker->SetAssetType(AssetType::Texture2D);
            checker->SetName("Checkerboard");
            AssetManager::Get()->AddToAssetAndID(2, checker);
            s_Instance->m_BuiltinAssetCount++;

            auto& cube = Renderer::Get()->GetCubeMesh();
            cube->SetAssetType(AssetType::Mesh);
            cube->SetName("Cube");
            AssetManager::Get()->AddToAssetAndID(3, cube);
            s_Instance->m_BuiltinAssetCount++;

            auto& sphere = Renderer::Get()->GetSphereMesh();
            sphere->SetAssetType(AssetType::Mesh);
            sphere->SetName("Sphere");
            AssetManager::Get()->AddToAssetAndID(4, sphere);
            s_Instance->m_BuiltinAssetCount++;

            auto& material = Renderer::Get()->GetDefaultMaterial();
            material->SetAssetType(AssetType::Material);
            material->SetName("DefaultMaterial");
            material->AlbedoMap = checker;
            AssetManager::Get()->AddToAssetAndID(5, material);
            s_Instance->m_BuiltinAssetCount++;
        }

        if (std::filesystem::exists(s_AssetDatabasePath))
        {
            // データベースからメタデータを取得
            s_Instance->LoadAssetMetaDataFromDatabaseFile(s_AssetDatabasePath);
        }

        // 物理ファイルとメタデータと照合しながらアセットディレクトリ全体を走査
        s_Instance->InspectAssetDirectory(s_AssetDiectoryPath);

        // データベースファイルのメタデータを更新する
        s_Instance->WriteDatabaseToFile(s_AssetDatabasePath);

        // メタデータを元に実際にアセットをメモリにロードする
        s_Instance->LoadAssetToMemory(s_AssetDatabasePath);
    }

    void AssetManager::Shutdown()
    {
        // データベースファイルのメタデータを更新する
        s_Instance->WriteDatabaseToFile(s_AssetDatabasePath);

        Memory::Deallocate(s_Instance);
    }

    bool AssetManager::HasMetadata(const std::filesystem::path& directory)
    {
        for (auto& [id, metadata] : m_Metadata)
        {
            if (metadata.FilePath == directory)
                return true;
        }

        return false;
    }

    AssetMetadata AssetManager::GetMetadata(const std::filesystem::path& directory)
    {
        for (auto& [id, metadata] : m_Metadata)
        {
            if (metadata.FilePath == directory)
                return metadata;
        }

        return {};
    }

    bool AssetManager::IsLoaded(const AssetID id)
    {
        return m_AssetData.contains(id);
    }

    AssetMetadata AssetManager::GetMetadata(AssetID id)
    {
        return m_Metadata[id];
    }

    std::unordered_map<AssetID, Shared<Asset>>& AssetManager::GetAllAssets()
    {
        return m_AssetData;
    }

    std::unordered_map<AssetID, AssetMetadata>& AssetManager::GetMetadatas()
    {
        return m_Metadata;
    }

    uint64 AssetManager::GenerateAssetID()
    {
        return Random<uint64>::Range(m_BuiltinAssetCount + 1, std::numeric_limits<uint64>::max());
    }

    bool AssetManager::IsValidID(AssetID id)
    {
        return id != 0 && id > m_BuiltinAssetCount;
    }

    void AssetManager::LoadAssetMetaDataFromDatabaseFile(const std::filesystem::path& filePath)
    {
        // ファイル読み込み
        std::ifstream stream(filePath);
        SL_ASSERT(stream);
        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        auto IDs = data["AssetDatabase"];
        if (!IDs)
        {
            SL_LOG_ERROR("データベースファイルが破損しているか、存在しません");
            return;
        }

        // 各メタデータを走査し、エラーが無ければ登録
        for (auto n : IDs)
        {
            //NOTE:======================================
            // ここでデータベースファイルが存在していた場合に
            // メタデータをもとにアセットの読み込みを試みるが
            // 現状、アセットが変更・削除されている場合は無視する
            //===========================================

            AssetMetadata metadata;
            metadata.ID       = n["id"].as<uint64_t>();
            metadata.Type     = (AssetType)n["type"].as<uint32>();
            metadata.FilePath = n["path"].as<std::string>();

            // アセットタイプが不明
            if (metadata.Type == AssetType::None)
                continue;

            // 物理ファイルが存在しない
            if (!std::filesystem::exists(metadata.FilePath))
            {
                SL_LOG_ERROR("{}: が存在しません", metadata.FilePath.string().c_str());
                continue;
            }

            // メタデータを登録
            AssetID id = metadata.ID;
            m_Metadata[id] = metadata;
        }
    }

    void AssetManager::InspectAssetDirectory(const std::filesystem::path& directory)
    {
        InspectRecursive(directory);
    }

    void AssetManager::InspectRecursive(const std::filesystem::path& directory)
    {
        // データベースファイルを読み込んでメタデータを取得した場合は LoadAssetMetaDataFromFile 関数で追加済みだが、
        // データベースファイルが存在しなかった場合、このタイミングにAddToMetadata関数で登録される
        for (auto& dir : std::filesystem::directory_iterator(directory))
        {
            // データベースファイルは無視する
            if (dir.path() == s_AssetDatabasePath)
                continue;

            if (dir.is_directory()) InspectRecursive(dir.path());
            else                    AddToMetadata(dir.path());
        }
    }

    void AssetManager::AddToAssetAndID(const AssetID id, Shared<Asset> asset)
    {
        asset->SetAssetID(id);
        m_AssetData[id] = asset;
    }

    void AssetManager::AddToAsset(Shared<Asset> asset)
    {
        m_AssetData[asset->GetAssetID()] = asset;
    }

    AssetMetadata AssetManager::AddToMetadata(const std::filesystem::path& directory)
    {
        // ディレクトリ区切り文字変換
        std::string path = directory.string();
        std::replace(path.begin(), path.end(), '\\', '/');
        std::filesystem::path dir = path;

        // メタデータ内に存在するなら追加しない
        if (HasMetadata(dir))
            return {};

        // なければ新規登録
        AssetMetadata metadata;
        metadata.ID       = GenerateAssetID();
        metadata.FilePath = dir;
        metadata.Type     = FileNameToAssetType(dir);

        m_Metadata[metadata.ID] = metadata;
        return metadata;
    }

    void AssetManager::RemoveFromMetadata(const AssetID id)
    {
        if (m_Metadata.contains(id))
        {
            m_Metadata.erase(id);
        }
    }

    void AssetManager::RemoveFromAsset(const AssetID id)
    {
        if (m_AssetData.contains(id))
        {
            m_AssetData.erase(id);
        }
    }

    void AssetManager::WriteDatabaseToFile(const std::filesystem::path& directory)
    {
        YAML::Emitter out;

        out << YAML::BeginMap << YAML::Key << "AssetDatabase";
        out << YAML::BeginSeq;

        for (auto& [id, metadata] : m_Metadata)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "id"   << YAML::Value << metadata.ID;
            out << YAML::Key << "type" << YAML::Value << (uint32)metadata.Type;
            out << YAML::Key << "path" << YAML::Value << metadata.FilePath.string();
            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(directory);
        fout << out.c_str();
    }

    //===========================================================================
    // マテリアルがテクスチャに依存するので、アセットタイプごとにイテレーションするようにする
    // 必要があれば、アセットタイプごとのデータ群を返す関数を追加する
    //===========================================================================
    void AssetManager::LoadAssetToMemory(const std::filesystem::path& filePath)
    {
        LOAD_PROCESS("Load Texture", 20);

        // テクスチャ2D: マテリアルから参照されるので、最初に読み込むこと!
        for (auto& [ud, metadata] : m_Metadata)
        {
            if (metadata.Type == AssetType::Texture2D)
            {
                AssetID id       = metadata.ID;
                std::string path = metadata.FilePath.string();

                Shared<Asset> asset = nullptr;
                asset = LoadAssetFromFile<Texture2D>(path);

                s_Instance->AddToAssetAndID(id, asset);
            }
        }

        LOAD_PROCESS("Load EnvironmentMap", 40);

        // 環境マップ
        for (auto& [ud, metadata] : m_Metadata)
        {
            if (metadata.Type == AssetType::SkyLight)
            {
                AssetID id       = metadata.ID;
                std::string path = metadata.FilePath.string();

                Shared<Asset> asset = nullptr;
                asset = LoadAssetFromFile<SkyLight>(path);

                s_Instance->AddToAssetAndID(id, asset);
            }
        }

        LOAD_PROCESS("Load Material", 60);

        // マテリアル
        for (auto& [ud, metadata] : m_Metadata)
        {
            if (metadata.Type == AssetType::Material)
            {
                AssetID id = metadata.ID;
                std::string path = metadata.FilePath.string();

                Shared<Asset> asset = nullptr;
                asset = LoadAssetFromFile<Material>(path);

                s_Instance->AddToAssetAndID(id, asset);
            }
        }

        LOAD_PROCESS("Load Mesh", 80);

        // メッシュ
        for (auto& [ud, metadata] : m_Metadata)
        {
            if (metadata.Type == AssetType::Mesh)
            {
                AssetID id = metadata.ID;
                std::string path = metadata.FilePath.string();

                Shared<Asset> asset = nullptr;
                asset = LoadAssetFromFile<Mesh>(path);

                s_Instance->AddToAssetAndID(id, asset);
            }
        }
    }
}
