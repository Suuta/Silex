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

        AssetBrowserItem(AssetItemType type, AssetID id, const std::string& name, const Ref<Texture2D>& icon)
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

        const Ref<Texture2D>& GetIcon() const    { return m_Icon; }
        void SetIcon(const Ref<Texture2D>& icon) { m_Icon = icon; }

    protected:

        AssetItemType     m_Type;
        AssetID           m_ID;
        std::string       m_FileName;
        Ref<Texture2D> m_Icon;
    };

    struct DirectoryNode : Object
    {
        Ref<DirectoryNode>                              ParentDirectory;
        std::unordered_map<AssetID, Ref<DirectoryNode>> ChildDirectory;

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

        AssetID TraversePhysicalDirectories(const std::filesystem::path& directory, const Ref<DirectoryNode>& parentDirectory);
        void ChangeDirectory(const Ref<DirectoryNode>& directory);

        void DrawDirectory(const Ref<DirectoryNode>& node);
        void DrawCurrentDirectoryAssets();

        void DrawMaterial();
        void LoadAssetIcons();

    private:

        AssetID                                               m_MoveRequestDirectoryAssetID;
        AssetID                                               m_DeleteRequestItemAssetID;
        Ref<Asset>                                         m_SelectAsset;
        Ref<DirectoryNode>                                 m_CurrentDirectory;
        Ref<DirectoryNode>                                 m_RootDirectory;
        std::unordered_map<AssetID, Ref<AssetBrowserItem>> m_CurrentDirectoryAssetItems;
        std::unordered_map<AssetID, Ref<DirectoryNode>>    m_Directories;

        std::unordered_map<AssetType, Ref<Texture2D>> m_AssetIcons;
        Ref<Texture2D>                                m_DirectoryIcon;

    private: 

        friend class AssetBrowserItem;
    };
}

