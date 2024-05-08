#pragma once

#include "Asset/Asset.h"


namespace Silex
{
    class Texture2D;
    class AssetBrowserPanel;

    enum class AssetItemType : uint8
    {
        Directory,
        Asset,
    };

    class AssetBrowserItem : Object
    {
    public:

        AssetBrowserItem(AssetItemType type, AssetID id, const std::string& name)
            : m_Type(type)
            , m_ID(id)
            , m_FileName(name)
        {
        }

        AssetBrowserItem(AssetItemType type, AssetID id, const std::string& name, const Shared<Texture2D>& icon)
            : m_Type(type)
            , m_ID(id)
            , m_FileName(name)
            , m_Icon(icon)
        {
        }

        void Render(AssetBrowserPanel* panel, const glm::vec2& size);

        AssetID            GetID()   const { return m_ID; }
        AssetItemType      GetType() const { return m_Type; }
        const std::string& GetName() const { return m_FileName; }

        const Shared<Texture2D>& GetIcon() const    { return m_Icon; }
        void SetIcon(const Shared<Texture2D>& icon) { m_Icon = icon; }

    protected:

        AssetItemType     m_Type;
        AssetID           m_ID;
        std::string       m_FileName;
        Shared<Texture2D> m_Icon;
    };

    struct DirectoryNode : Object
    {
        Shared<DirectoryNode>                              ParentDirectory;
        std::unordered_map<AssetID, Shared<DirectoryNode>> ChildDirectory;

        AssetID               ID;
        std::filesystem::path FilePath;
        std::vector<AssetID>  Assets;
    };


    class AssetBrowserPanel
    {
    public:

        AssetBrowserPanel()  = default;
        ~AssetBrowserPanel() = default;

        void Initialize();
        void Finalize();
        void Render(bool* show, bool* showProperty);

    private:

        AssetID TraversePhysicalDirectories(const std::filesystem::path& directory, const Shared<DirectoryNode>& parentDirectory);
        void ChangeDirectory(const Shared<DirectoryNode>& directory);

        void DrawDirectory(const Shared<DirectoryNode>& node);
        void DrawCurrentDirectoryAssets();

        void DrawMaterial();
        void LoadAssetIcons();

    private:

        AssetID                                               m_MoveRequestDirectoryAssetID;
        AssetID                                               m_DeleteRequestItemAssetID;
        Shared<Asset>                                         m_SelectAsset;
        Shared<DirectoryNode>                                 m_CurrentDirectory;
        Shared<DirectoryNode>                                 m_RootDirectory;
        std::unordered_map<AssetID, Shared<AssetBrowserItem>> m_CurrentDirectoryAssetItems;
        std::unordered_map<AssetID, Shared<DirectoryNode>>    m_Directories;

        std::unordered_map<AssetType, Shared<Texture2D>> m_AssetIcons;
        Shared<Texture2D>                                m_DirectoryIcon;

    private: 

        friend class AssetBrowserItem;
    };
}

