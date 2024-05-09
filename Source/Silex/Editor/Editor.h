
#pragma once

#include "Core/Core.h"
#include "Core/SharedPointer.h"
#include "Renderer/Mesh.h"
#include "Renderer/Shader.h"
#include "Renderer/Camera.h"
#include "Scene/SceneRenderer.h"
#include "Editor/ScenePropertyPanel.h"
#include "Editor/AssetBrowserPanel.h"

#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>


struct GLFWwindow;

namespace Silex
{
    class Editor : public Object
    {
        SL_DECLARE_CLASS(Editor, Object)

    public:

        void Init();
        void Shutdown();
        void Update(float deltaTime);
        void Render();

    public:

        void OpenScene();                          // シーン開く
        void SaveScene(bool bForceSaveAs = false); // シーン保存
        void NewScene();

        // ウィンドウイベント
        void OnMouseMove(MouseMoveEvent& e);
        void OnMouseScroll(MouseScrollEvent& e);

    public:

        const std::filesystem::path& GetAssetDirectory() const { return m_AssetDirectory; }

    private:

        // ヒエラルキー選択イベント
        void OnClickHierarchyEntity(bool selected);

        void OpenScene(const std::string& filePath);
        void SaveSceneAs();

        void SelectViewportEntity();
        void HandleInput(float deltaTime);

    private:

        std::filesystem::path m_CurrentScenePath = "";
        std::string           m_CurrentSceneName = "名称未指定";

        glm::ivec2 m_SceneViewportFramebufferSize = { 1280, 720 };
        glm::ivec2 m_RelativeViewportRect[2];
        glm::ivec2 m_PrevCursorPosition;

        bool  bUsingEditorCamera = false;

        // カメラ
        Camera m_EditorCamera = { glm::vec3(0.0f, 1.0f, -10.0f) };

    private:

        Shared<Scene> m_Scene;
        SceneRenderer m_SceneRenderer;

        ScenePropertyPanel m_ScenePropertyPanel;
        AssetBrowserPanel  m_AssetBrowserPanel;

        std::filesystem::path m_AssetDirectory = "Assets/";

        // オブジェクト選択
        bool                bUsingManipulater      = false;
        bool                bHoveredViewport       = false;
        bool                bActiveGizmoForcus     = true;
        int32               m_SelectionID          = -1;
        ImGuizmo::OPERATION m_ManipulateType       = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE      m_ManipulateMode       = ImGuizmo::LOCAL;
        glm::vec3           m_SelectEntityPosition = {};

        // UIパネルの表示フラグ
        bool bShowScene        = true;
        bool bShowProperty     = true;
        bool bShowOutliner     = true;
        bool bShowLogger       = true;
        bool bShowStats        = true;
        bool bShowMaterial     = true;
        bool bShowAssetBrowser = true;
    };
}
