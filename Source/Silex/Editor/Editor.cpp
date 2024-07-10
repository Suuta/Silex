
#include "PCH.h"

#include "Editor/Editor.h"
#include "Editor/ConsoleLogger.h"
#include "Editor/SplashImage.h"

#include "Core/Timer.h"
#include "Core/Random.h"
#include "Core/Engine.h"
#include "Rendering/Renderer.h"
#include "Serialize/SceneSerializer.h"

#include <imgui/imgui_internal.h>
#include <imgui/imgui.h>
#include <glm/gtx/matrix_decompose.hpp>


namespace Silex
{
    void Editor::Init()
    {
        SL_LOG_TRACE("Editor::Init");

        // シーン生成
        m_Scene = CreateShared<Scene>();

        // アウトラウナーにシーンを登録
        m_ScenePropertyPanel.SetScene(m_Scene);
        m_ScenePropertyPanel.onEntitySelectDelegate.Bind(this, &Editor::OnClickHierarchyEntity);

        //m_AssetBrowserPanel.Initialize();

        // シーンレンダラー初期化
        //m_SceneRenderer.Init();

        LOAD_PROCESS("Editor Init", 100.0f);
        OS::Get()->Sleep(500);
    }

    void Editor::Shutdown()
    {
        SL_LOG_TRACE("Editor::Shutdown");

        m_AssetBrowserPanel.Finalize();

        //m_SceneRenderer.Shutdown();
        //m_ScenePropertyPanel.onEntitySelectDelegate.Unbind();
    }

    void Editor::Update(float deltaTime)
    {
        HandleInput(deltaTime);
        m_EditorCamera.Update(deltaTime);

        m_Scene->Update(deltaTime, m_EditorCamera, &m_SceneRenderer);
    }

    void Editor::Render()
    {
        SL_SCOPE_PROFILE("Render - Editor");
        Renderer::Get()->SetDefaultFramebuffer();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGuiWindowFlags windowFlags = 0;
        windowFlags |= ImGuiWindowFlags_NoDocking;
        windowFlags |= ImGuiWindowFlags_NoTitleBar;
        windowFlags |= ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoResize;
        windowFlags |= ImGuiWindowFlags_NoCollapse;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        windowFlags |= ImGuiWindowFlags_NoNavFocus;
        windowFlags |= ImGuiWindowFlags_MenuBar;

        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace", nullptr, windowFlags);
            ImGui::PopStyleVar();

            ImGuiWindowFlags docapaceFlag = 0;
            //docapaceFlag |= ImGuiDockNodeFlags_NoWindowMenuButton;
            docapaceFlag |= ImGuiDockNodeFlags_None;

            ImGuiID dockspace_id = ImGui::GetID("Dockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), docapaceFlag);

            //============================
            // メニューバー
            //============================
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("編集"))
                {
                    if (ImGui::MenuItem("シーンを開く", "Ctr+O ")) OpenScene();
                    if (ImGui::MenuItem("シーンを保存", "Ctr+S ")) SaveScene();
                    if (ImGui::MenuItem("終了",       "Alt+F4")) Engine::Get()->Close();

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("ウィンドウ"))
                {
                    ImGui::MenuItem("シーンビューポート", nullptr, &bShowScene);
                    ImGui::MenuItem("アウトプットロガー", nullptr, &bShowLogger);
                    ImGui::MenuItem("統計",            nullptr, &bShowStats);
                    ImGui::MenuItem("アウトライナー",    nullptr, &bShowOutliner);
                    ImGui::MenuItem("プロパティ",       nullptr, &bShowProperty);
                    ImGui::MenuItem("マテリアル",       nullptr, &bShowMaterial);
                    ImGui::MenuItem("アセットブラウザ",  nullptr, &bShowAssetBrowser);
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            ImGui::End();
        }

        // シーンプロパティ
        m_ScenePropertyPanel.Render(&bShowOutliner, &bShowProperty);

        // アセットプロパティ
        m_AssetBrowserPanel.Render(&bShowAssetBrowser, &bShowMaterial);


        if (bShowScene)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("シーン", &bShowScene);

            // ビューポートへのホバー
            bHoveredViewport = ImGui::IsWindowHovered();

            // ウィンドウ内のコンテンツサイズ（タブやパディングは含めないので、ImGui::GetWindowSize関数は使わない）
            ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
            ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
            ImVec2 content    = { contentMax.x - contentMin.x, contentMax.y - contentMin.y };

            // ウィンドウ左端からのオフセット
            ImVec2 viewportOffset = ImGui::GetWindowPos();

            m_RelativeViewportRect[0] = { contentMin.x + viewportOffset.x, contentMin.y + viewportOffset.y };
            m_RelativeViewportRect[1] = { contentMax.x + viewportOffset.x, contentMax.y + viewportOffset.y };

            // シーン描画
            {
                // エディターが有効な状態では、シーンビューポートサイズがフレームバッファサイズになる
                if ((content.x > 0.0f && content.y > 0.0f) && (content.x != m_SceneViewportFramebufferSize.x || content.y != m_SceneViewportFramebufferSize.y))
                {
                    m_SceneViewportFramebufferSize.x = content.x;
                    m_SceneViewportFramebufferSize.y = content.y;

                    m_EditorCamera.SetViewportSize(m_SceneViewportFramebufferSize.x, m_SceneViewportFramebufferSize.y);
                    m_SceneRenderer.SetViewportSize(m_SceneViewportFramebufferSize.x, m_SceneViewportFramebufferSize.y);
                }

                // シーン描画
                ImGui::Image((void*)m_SceneRenderer.GetFinalRenderPassID(), content, { 0, 1 }, { 1, 0 });
            }

            // ギズモ
            if (bActiveGizmoForcus)
            {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(m_RelativeViewportRect[0].x, m_RelativeViewportRect[0].y, content.x, content.y);

                const float* viewMatrix       = glm::value_ptr(m_EditorCamera.GetViewMatrix());
                const float* projectionMatrix = glm::value_ptr(m_EditorCamera.GetProjectionMatrix());

                // ギズモ描画位置
                Entity selectEntity = m_ScenePropertyPanel.GetSelectedEntity();
                if (selectEntity)
                {
                    TransformComponent& tc = selectEntity.GetComponent<TransformComponent>();
                    glm::mat4 transform = tc.GetTransform();

                    m_SelectEntityPosition = tc.position;

                    // ギズモ描画
                    ImGuizmo::Manipulate(viewMatrix, projectionMatrix, m_ManipulateType, m_ManipulateMode, glm::value_ptr(transform));

                    // ギズモ操作（変化量加算）
                    if (ImGuizmo::IsUsing())
                    {
                        glm::vec3 skew;
                        glm::vec4 perspective;
                        glm::vec3 translation, scale, rotation;
                        glm::quat quatRotation;

                        glm::decompose(transform, scale, quatRotation, translation, skew, perspective);
                        rotation = glm::eulerAngles(quatRotation);

                        // TODO: 角度制限する
                        // フレームごとの回転を加算する
                        glm::vec3 dtRot = rotation - tc.rotation;
                        tc.position =  translation;
                        tc.rotation += dtRot;
                        tc.Scale    =  scale;

                        bUsingManipulater = true;
                    }
                    else
                    {
                        bUsingManipulater = false;
                    }
                }
            }

            ImGui::End();
            ImGui::PopStyleVar();
        }

        // エディターカメラ操作中に他のウィンドウへのフォーカスを無効にする
        if (bUsingEditorCamera)
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);

        // アウトプットログ
        if (bShowLogger)
        {
            ImGui::Begin("アウトプットログ", &bShowLogger);

            // Clear ボタンを右寄せするための計算
            float buttonWidth = 100.0f;
            float spacingFromRightEdge = ImGui::GetWindowContentRegionMax().x - buttonWidth - 20;

            if (spacingFromRightEdge > 0)
                ImGui::Dummy(ImVec2(spacingFromRightEdge, 0.0f));  // 空のスペースを追加してボタンを右に移動

            ImGui::SameLine();

            if (ImGui::Button("Clear", ImVec2(buttonWidth, 25.0f)))
                ConsoleLogger::Get().Clear();

            ImGui::Separator();

            ConsoleLogger::Get().LogData();

            ImGui::End();
        }

        // 統計
        if (bShowStats)
        {
            ImGui::Begin("統計", &bShowStats, ImGuiWindowFlags_NoTitleBar);
            ImGui::Text("FPS: %d (%.2f)ms", Engine::Get()->GetFrameRate(), Engine::Get()->GetDeltaTime() * 1000);
            ImGui::Text("Resolution: %d, %d", m_SceneViewportFramebufferSize.x, m_SceneViewportFramebufferSize.y);

            ImGui::SeparatorText("");

            SceneRenderStats stats = m_SceneRenderer.GetRenderStats();
            ImGui::Text("GeometryDrawCall: %d", stats.numGeometryDrawCall);
            ImGui::Text("ShadowDrawCall:   %d", stats.numShadowDrawCall);
            ImGui::Text("NumMesh:          %d", stats.numRenderMesh);

            ImGui::SeparatorText("");

            for (const auto& [profile, time] : Engine::Get()->GetPerformanceData())
            {
                ImGui::Text("%-*s %.2f ms", 32, profile, time);
            }

            // メモリー使用量
            //ImGui::SeparatorText("");
            //auto status = PoolAllocator::GetStatus();
            //for (uint32 i = 0; i < status.size(); i++)
            //{
            //    ImGui::Text("メモリプール[%4d byte]:%6d / %6d Block", 32 << i, status[i].TotalAllocated / (32 << i), status[i].TotalSize / (32 << i));
            //}

            ImGui::End();
        }

        // ウィンドウへのフォーカスを有効にする
        if (bUsingEditorCamera)
            ImGui::PopItemFlag();

        ImGui::PopStyleVar(2);
    }

    void Editor::HandleInput(float deltaTime)
    {
        // エディターカメラ操作
        if (Input::IsMouseButtonDown(Mouse::Right) && bHoveredViewport)
        {
            Input::SetCursorMode(CursorMode::Disable);
            bUsingEditorCamera = true;

            if (Input::IsKeyDown(Keys::W)) m_EditorCamera.Move(CameraMovementDir::Forward,  deltaTime);
            if (Input::IsKeyDown(Keys::S)) m_EditorCamera.Move(CameraMovementDir::Backward, deltaTime);
            if (Input::IsKeyDown(Keys::A)) m_EditorCamera.Move(CameraMovementDir::Left,     deltaTime);
            if (Input::IsKeyDown(Keys::D)) m_EditorCamera.Move(CameraMovementDir::Right,    deltaTime);
            if (Input::IsKeyDown(Keys::E)) m_EditorCamera.Move(CameraMovementDir::Up,       deltaTime);
            if (Input::IsKeyDown(Keys::Q)) m_EditorCamera.Move(CameraMovementDir::Down,     deltaTime);
        }
        else
        {
            Input::SetCursorMode(CursorMode::Normal);
            bUsingEditorCamera = false;

            // オブジェクト選択
            if (Input::IsMouseButtonReleased(Mouse::Left) && !bUsingManipulater && bHoveredViewport)
            {
                SelectViewportEntity();
            }

            if (Input::IsKeyDown(Keys::LeftControl))
            {
                if (Input::IsKeyPressed(Keys::S))
                {
                    SaveScene();
                }
                else if (Input::IsKeyPressed(Keys::O))
                {
                    OpenScene();
                }
                else if (Input::IsKeyPressed(Keys::N))
                {
                    NewScene();
                }
            }

            // ギズモ操作
            if (Input::IsKeyPressed(Keys::W))
            {
                bActiveGizmoForcus = true;
                m_ManipulateType   = ImGuizmo::TRANSLATE;
            }
            else if (Input::IsKeyPressed(Keys::E))
            {
                bActiveGizmoForcus = true;
                m_ManipulateType   = ImGuizmo::ROTATE;
            }
            else if (Input::IsKeyPressed(Keys::R))
            {
                bActiveGizmoForcus = true;
                m_ManipulateType   = ImGuizmo::SCALE;
            }
            else if (Input::IsKeyPressed(Keys::Q))
            {
                m_ManipulateMode = m_ManipulateMode? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            }
            else if (Input::IsKeyPressed(Keys::F) && bActiveGizmoForcus)
            {
                m_EditorCamera.SetPosition(m_SelectEntityPosition - m_EditorCamera.GetFront() * 5.0f);
            }
        }
    }

    void Editor::SelectViewportEntity()
    {
        if (!bHoveredViewport)
            return;

        // ピクセルデータ読み取り
        auto pos = Input::GetCursorPosition();
        pos.x -= m_RelativeViewportRect[0].x;
        pos.y -= m_RelativeViewportRect[0].y;

        m_SelectionID = m_SceneRenderer.ReadEntityIDFromPixcel(pos.x, pos.y);

        if (m_SelectionID >= 0)
        {
            bActiveGizmoForcus = true;

            Entity entity = { (entt::entity)m_SelectionID, m_Scene.Get() };
            m_ScenePropertyPanel.SetSelectedEntity(entity);
        }
        else
        {
            bActiveGizmoForcus = false;
            m_ScenePropertyPanel.SetSelectedEntity({});
        }
    }

    void Editor::OpenScene()
    {
        std::string filePath = OS::Get()->OpenFile("Silex Scene (*.slsc)\0*.slsc\0");
        if (!filePath.empty())
        {
            OpenScene(filePath);
        }
    }

    void Editor::OpenScene(const std::string& filePath)
    {
        Shared<Scene> newScene = CreateShared<Scene>();

        SceneSerializer serializer(newScene.Get());
        serializer.Deserialize(filePath);

        m_Scene = newScene;
        m_SceneRenderer.SetViewportSize(m_SceneViewportFramebufferSize.x, m_SceneViewportFramebufferSize.y);

        m_ScenePropertyPanel.SetScene(m_Scene);
        m_CurrentScenePath = filePath;
        m_CurrentSceneName = m_CurrentScenePath.stem().string();

        Window::Get()->SetTitle(("Silex - " + m_CurrentSceneName).c_str());
    }

    void Editor::SaveScene(bool bForceSaveAs)
    {
        // 名前を付けて保存
        if (bForceSaveAs)
        {
            SaveSceneAs();
        }
        else
        {
            // 現在のシーンが名称未指定の場合は、新たにシーンファイルを生成して保存
            if (m_CurrentScenePath.empty())
            {
                SaveSceneAs();
            }
            else
            {
                // シーンファイルが生成済みなら、そこに上書き
                SceneSerializer serializer(m_Scene.Get());
                serializer.Serialize(m_CurrentScenePath.string());
            }
        }
    }

    void Editor::SaveSceneAs()
    {
        std::string filePath = OS::Get()->SaveFile("Silex Scene (*.slsc)\0*.slsc\0", "slsc");

        SceneSerializer serializer(m_Scene.Get());
        serializer.Serialize(filePath);

        m_CurrentScenePath = filePath;
        m_CurrentSceneName = m_CurrentScenePath.stem().string();

        Window::Get()->SetTitle(("Silex - " + m_CurrentSceneName).c_str());
    }

    void Editor::NewScene()
    {
        Shared<Scene> newScene = CreateShared<Scene>();

        m_Scene = newScene;
        m_SceneRenderer.SetViewportSize(m_SceneViewportFramebufferSize.x, m_SceneViewportFramebufferSize.y);

        m_ScenePropertyPanel.SetScene(m_Scene);
        m_CurrentScenePath = "";
        m_CurrentSceneName = "名称未指定";

        Window::Get()->SetTitle(("Silex - " + m_CurrentSceneName).c_str());
    }

    // シーンアウトライナーでエンティティがクリックされた時のイベント
    void Editor::OnClickHierarchyEntity(bool selected)
    {
        bActiveGizmoForcus = selected;
    }

    void Editor::OnMouseMove(MouseMoveEvent& e)
    {
        // スクリーン座標のため、Y座標は下向きが正の値となる
        float xoffset = e.mouseX - m_PrevCursorPosition.x;
        float yoffset = m_PrevCursorPosition.y - e.mouseY;

        m_PrevCursorPosition = { e.mouseX, e.mouseY };

        if (bUsingEditorCamera)
        {
            m_EditorCamera.ProcessMouseMovement(xoffset, yoffset);
        }
    }

    void Editor::OnMouseScroll(MouseScrollEvent& e)
    {
        if (bHoveredViewport)
        {
            m_EditorCamera.ProcessMouseScroll(e.offsetY);
        }
    }
}
