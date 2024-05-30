
#include "PCH.h"
#include "Editor/AssetBrowserPanel.h"
#include "Core/Engine.h"
#include "Rendering/Material.h"
#include "Serialize/AssetSerializer.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>


//================================================================================
// 方針
//--------------------------------------------------------------------------------
// ・特定のディレクトリ（マテリアル）を指定し、そのディレクトリ内のファイルに限定して操作を適応する
// ・生成時に物理ファイルを生成し、メタデータファイルを更新
// ・ファイル情報をマテリアルパラメータとし、マテリアルパラメータパネルから編集可能にする
// ・シーンファイルに直接記述していたマテリアル情報をマテリアルファイルに移行する
// ・シーンレンダラーのメッシュインスタンシングデータ判別にマテリアルの判別を追加
// ・操作はコンテキストメニュー形式で行い、削除・生成の2つのコマンドを適応できるようにする
// ・メッシュコンポーネントのスロットにマテリアルのスロットのみ用意する
//================================================================================

/* マテリアル シリアライズ形式
    Albedo: [1, 1, 1]
    AlbedoTexture : 0
    Emission : [0, 0, 0]
    Metallic : 1
    Roughness : 1
    TextureTiling : [1, 1]
    CastShadow : true
=========================*/

namespace Silex
{
    //=============================
    // AssetBrowserItem
    //=============================
    void AssetBrowserItem::Render(AssetBrowserPanel* panel, const glm::vec2& size)
    {
        bool isSelected = false;
        AssetID selectID = 0;

        if (panel->m_SelectAsset)
            selectID = panel->m_SelectAsset->GetAssetID();

        isSelected = m_ID == selectID;
        ImVec4 color = isSelected ? ImVec4(0.25, 0.85, 0.85, 1) : ImVec4(0, 0, 0, 0);

        ImGui::PushStyleColor(ImGuiCol_Button,        color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  color);
        ImGui::ImageButton(ImTextureID(m_Icon ? m_Icon->GetID() : 0), { size.x - 10.0f, size.y - 10.0f }, { 0, 0 }, { 1, 1 }, 2);
        ImGui::PopStyleColor(3);

        // 左クリック（選択）
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                panel->m_SelectAsset = AssetManager::Get()->GetAssetAs<Material>(m_ID);

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && m_Type == AssetItemType::Directory)
                panel->m_MoveRequestDirectoryAssetID = m_ID;
        }

        // 右クリック（コンテキストメニュー）
        if (ImGui::BeginPopupContextItem(m_FileName.c_str()))
        {
            if (ImGui::MenuItem("削除"))
            {
                // アセットリストから削除
                panel->m_DeleteRequestItemAssetID = m_ID;
                AssetManager::Get()->DeleteAsset(m_ID);
            }
        
            ImGui::EndPopup();
        }

        ImGui::TextWrapped(m_FileName.c_str());
        ImGui::NextColumn();
    }

    //=============================
    // AssetBrowserPanel
    //=============================
    void AssetBrowserPanel::Initialize()
    {
        LoadAssetIcons();

        std::string directory = Engine::Get()->GetEditor()->GetAssetDirectory().string();

        // 指定ディレクトリ内の ファイル/サブディレクトリ を走査して要素をアイテムとして保存
        AssetID rootDirrectoryID = TraversePhysicalDirectories(directory, nullptr);
        m_CurrentDirectory = m_Directories[rootDirrectoryID];
        m_RootDirectory    = m_CurrentDirectory;

        // 現在のディレクトリを表示ディレクトリとして適応
        ChangeDirectory(m_CurrentDirectory);
    }

    void AssetBrowserPanel::Finalize()
    {
    }

    void AssetBrowserPanel::Render(bool* showBrowser, bool* showProperty)
    {
        if (*showBrowser)
        {
            ImGui::Begin("アセットブラウザ", showBrowser, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
        
            ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable
                | ImGuiTableFlags_SizingFixedFit
                | ImGuiTableFlags_BordersInnerV;

            ImGui::PushID("Browser");

            if (ImGui::BeginTable("table", 2, tableFlags, ImVec2(0.0f, 0.0f)))
            {
                ImGui::TableSetupColumn("Outliner", 0, 300.0f);
                ImGui::TableSetupColumn("DirectoryWidth", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                {
                    // ディレクトリー階層表示
                    ImGui::BeginChild("DirectoryHierarchy");

                    for (auto& [id, node] : m_RootDirectory->ChildDirectory)
                    {
                        DrawDirectory(node);
                    }

                    ImGui::EndChild();
                }
                
                ImGui::TableSetColumnIndex(1);
                {
                    // 現在のディレクトリのアセットを表示
                    ImGui::BeginChild("DirectoryAssets");
                    DrawCurrentDirectoryAssets();
                    ImGui::EndChild();
                }

                ImGui::EndTable();
            }

            ImGui::PopID();
            ImGui::End();
        }

        // TODO: テンプレート化 or インターフェースで実装する
        if (*showProperty)
        {
            ImGui::Begin("マテリアル", showProperty);
            DrawMaterial();
            ImGui::End();
        }
    }

    AssetID AssetBrowserPanel::TraversePhysicalDirectories(const std::filesystem::path& directory, const Shared<DirectoryNode>& parentDirectory)
    {
        // ディレクトリをアセットとして扱い、アセットIDを生成する（このIDシリアライズされず、起動の度に変化する）
        Shared<DirectoryNode> node = CreateShared<DirectoryNode>();
        node->ID              = AssetManager::Get()->GenerateAssetID();
        node->ParentDirectory = parentDirectory;

        if (directory != Engine::Get()->GetEditor()->GetAssetDirectory())
        {
            node->FilePath = directory;
        }

        for (auto& entry : std::filesystem::directory_iterator(directory))
        {
            // ディレクトリなら再帰
            if (entry.is_directory())
            {
                AssetID subdirID               = TraversePhysicalDirectories(entry.path(), parentDirectory);
                node->ChildDirectory[subdirID] = m_Directories[subdirID];
                continue;
            }

            // メタデータの検証
            auto metadata = AssetManager::Get()->GetMetadata(entry.path());
            if (!AssetManager::Get()->IsValidID(metadata.ID))
            {
                AssetType type = FileNameToAssetType(entry.path());
                if (type == AssetType::None)
                    continue;

                // 無ければデータベースに登録？
                // AssetManager::Get()->AddToMetadata(entry.path())
            }

            node->Assets.push_back(metadata.ID);
        }

        m_Directories[node->ID] = node;
        return node->ID;
    }

    void AssetBrowserPanel::ChangeDirectory(const Shared<DirectoryNode>& directory)
    {
        m_CurrentDirectoryAssetItems.clear();

        // ディレクトリ追加
        for (auto& [id, node] : directory->ChildDirectory)
        {
            std::string fileName = node->FilePath.filename().string();
            m_CurrentDirectoryAssetItems[id] = (CreateShared<AssetBrowserItem>(AssetItemType::Directory, id, std::move(fileName), m_DirectoryIcon));
        }

        // アセットファイル追加
        for (auto& id : directory->Assets)
        {
            auto metadata = AssetManager::Get()->GetMetadata(id);
            if (metadata.FilePath.empty())
            {
                continue;
            }

            std::string fileName = metadata.FilePath.filename().string();
            m_CurrentDirectoryAssetItems[id] = (CreateShared<AssetBrowserItem>(AssetItemType::Asset, id, std::move(fileName), m_AssetIcons[metadata.Type]));
        }
    }

    void AssetBrowserPanel::DrawDirectory(const Shared<DirectoryNode>& node)
    {
        std::string label    = node->FilePath.filename().string();
        bool open            = ImGui::TreeNode(label.c_str());
        bool changeDirectory = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (open)
        {
            for (auto& [id, node] : node->ChildDirectory)
            {
                DrawDirectory(node);
            }

            ImGui::TreePop();
        }

        if (changeDirectory)
        {
            ChangeDirectory(node);
        }
    }

    void AssetBrowserPanel::DrawCurrentDirectoryAssets()
    {
        // マウスのオーバーラップが無い場合にコンテキストメニューを表示
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::BeginMenu("新規アセット"))
            {
                if (ImGui::MenuItem("マテリアル"))
                {
                    std::string filePath = OS::Get()->SaveFile("Silex Material (*.slmt)\0*.slmt\0", "slmt");
                    if (!filePath.empty())
                    {
                        // アセットパスを
                        std::string filename = std::filesystem::path(filePath).filename().string();
                        std::string path = m_CurrentDirectory->FilePath.string() + filename;

                        // 新規アセット生成
                        Shared<Material> materialAsset = AssetManager::Get()->CreateAsset<Material>(path);

                        // アセットブラウザのアセットリストに追加
                        m_CurrentDirectoryAssetItems[materialAsset->GetAssetID()] = (CreateShared<AssetBrowserItem>(AssetItemType::Asset, materialAsset->GetAssetID(), std::move(filename)));
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
        
        // 空領域クリックで、選択解除
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
        {
            m_SelectAsset = nullptr;
        }


        float thumbnailSize = 96.0f;

        float panelWidth = ImGui::GetContentRegionAvail().x - ImGui::GetCurrentWindow()->ScrollbarSizes.x;
        int columnCount = (int)(panelWidth / thumbnailSize);
        if (columnCount < 1)
            columnCount = 1;

        ImGui::Columns(columnCount, 0, false);

        for (auto& [id, item] : m_CurrentDirectoryAssetItems)
        {
            // アイテム描画
            item->Render(this, { thumbnailSize, thumbnailSize });
        }

        // ディレクトリ移動要求時に移動する
        if (m_MoveRequestDirectoryAssetID != 0)
        {
            auto& node = m_Directories[m_MoveRequestDirectoryAssetID];
            ChangeDirectory(node);
            m_MoveRequestDirectoryAssetID = 0;
        }

        // 削除要求のアセットを、アセット参照リストから削除
        m_CurrentDirectoryAssetItems.erase(m_DeleteRequestItemAssetID);
        m_DeleteRequestItemAssetID = 0;
    }

    void AssetBrowserPanel::DrawMaterial()
    {
        if (!m_SelectAsset)
            return;

        if (m_SelectAsset->GetAssetType() != AssetType::Material)
            return;

        const float windowWidth = ImGui::GetWindowWidth();
        const float offset      = ImGui::GetCurrentWindow()->WindowPadding.x;
        const float buttonWidth = 100;

        ImGui::Button("Standard", { (windowWidth * 0.75f) - offset, 25 });
        ImGui::SameLine(0.0f, 4.0f);
        
        if (ImGui::Button("Save", { windowWidth * 0.25f - offset - 4.0f, 25}))
        {
            AssetSerializer<Material>::Serialize(m_SelectAsset.As<Material>(), m_SelectAsset->GetFilePath());
        }

        ImGui::Separator();

        Shared<Material> material    = m_SelectAsset.As<Material>();
        uint32 albedoTextureThumnail = material->AlbedoMap? material->AlbedoMap->GetID() : 0;

        ImGui::Dummy({ 0, 4.0f });
        ImGui::Columns(2);
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y;

        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, lineHeight * 0.5f));

            if (ImGui::ImageButton((ImTextureID)albedoTextureThumnail, { lineHeight * 0.8f, lineHeight * 0.8f }, { 0, 0 }, { 1, 1 }, 1))
                ImGui::OpenPopup("##AlbedoPopup");

            if (ImGui::BeginPopup("##AlbedoPopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
            {
                AssetID current = {};
                bool selected = false;
                bool modified = false;

                if (ImGui::BeginListBox("##AlbedoPopup", ImVec2(500.f, 0.0f)))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));

                    auto& database = AssetManager::Get()->GetAllAssets();
                    for (auto& [id, asset] : database)
                    {
                        if (asset == nullptr || !asset->IsAssetOf(AssetType::Texture2D))
                            continue;

                        selected = (current == id);
                        if (ImGui::Selectable(asset->GetName().c_str(), selected))
                        {
                            current  = id;
                            modified = true;

                            material->AlbedoMap = asset.As<Texture2D>();
                        }

                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::PopStyleVar();
                    ImGui::EndListBox();
                }

                if (modified)
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }


            ImGui::SameLine(); ImGui::Dummy({ 8.0f, 0.0f }); ImGui::SameLine();

            ImGui::Text("Albedo");
            ImGui::Text("Emission");
            ImGui::Text("Metallic");
            ImGui::Text("Roughness");
            ImGui::Text("TextureTiling");
            ImGui::Text("ShadingModel");

            ImGui::PopStyleVar();
        }

        ImGui::NextColumn();

        {
            ImGui::PushItemWidth(ImGui::GetColumnWidth(1) - 10);

            ImGui::PushID("Albedo");        ImGui::ColorEdit3("",  glm::value_ptr(material->Albedo));              ImGui::PopID();
            ImGui::PushID("Emission");      ImGui::ColorEdit3("",  glm::value_ptr(material->Emission));            ImGui::PopID();
            ImGui::PushID("Metallic");      ImGui::SliderFloat("", &material->Metallic, 0.0f, 1.0f);               ImGui::PopID();
            ImGui::PushID("Roughness");     ImGui::SliderFloat("", &material->Roughness, 0.0f, 1.0f);              ImGui::PopID();
            ImGui::PushID("TextureTiling"); ImGui::DragFloat2("",  glm::value_ptr(material->TextureTiling), 0.1f); ImGui::PopID();

            ImGui::PushID("ShadingModel");

            struct ShadingModelLabel
            {
                ShadingModelType model;
                const char*      name;
            };

            std::array<ShadingModelLabel, 2> itemList;
            itemList[0] = {ShadingModelType::BlinnPhong, "BlinnPhong"};
            itemList[1] = {ShadingModelType::BRDF,       "BRDF"};

            static const char* s_currentItem = itemList[material->ShadingModel].name;

            if (ImGui::BeginCombo("", s_currentItem))
            {
                for (int i = 0; i < itemList.size(); ++i)
                {
                    const bool isSelected = (s_currentItem == itemList[i].name);

                    if (ImGui::Selectable(itemList[i].name, isSelected))
                    {
                        s_currentItem = itemList[i].name;
                        material->ShadingModel = itemList[i].model;
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::Dummy({ 0, 4.0f });
    }

    void AssetBrowserPanel::LoadAssetIcons()
    {
        AssetMetadata meta = AssetManager::Get()->GetMetadata("Assets/Editor/Directory.png");
        m_DirectoryIcon = AssetManager::Get()->GetAssetAs<Texture2D>(meta.ID);

        meta = AssetManager::Get()->GetMetadata("Assets/Editor/Material.png");
        m_AssetIcons[AssetType::Material] = AssetManager::Get()->GetAssetAs<Texture2D>(meta.ID);

        meta = AssetManager::Get()->GetMetadata("Assets/Editor/Scene.png");
        m_AssetIcons[AssetType::Scene] = AssetManager::Get()->GetAssetAs<Texture2D>(meta.ID);

        meta = AssetManager::Get()->GetMetadata("Assets/Editor/Texture.png");
        m_AssetIcons[AssetType::Texture2D] = AssetManager::Get()->GetAssetAs<Texture2D>(meta.ID);

        meta = AssetManager::Get()->GetMetadata("Assets/Editor/File.png");
        const auto& icon = AssetManager::Get()->GetAssetAs<Texture2D>(meta.ID);
        m_AssetIcons[AssetType::None]      = icon;
        m_AssetIcons[AssetType::SkyLight]  = icon;
        m_AssetIcons[AssetType::Mesh]      = icon;
    }
}
