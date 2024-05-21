
#include "PCH.h"

#include "Editor/EditorUI.h"

#include <glfw/glfw3.h>
#include <imgui/imgui.h>
#include <_imguizmo/ImGuizmo.h>


namespace Silex
{
    void EditorUI::Init()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/Fonts/VL-Gothic-Regular.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

        {
            ImGuiStyle& style = ImGui::GetStyle();
            ImGui::StyleColorsDark();

            style.WindowRounding       = 0.0f;
            style.GrabRounding         = 0.0f;
            style.TabRounding          = 4.0f;
            style.DockingSeparatorSize = 1.0f;

            style.Colors[ImGuiCol_Text]                  = ImVec4( 0.70f, 0.70f, 0.70f, 1.00f );

            style.Colors[ImGuiCol_MenuBarBg]             = ImVec4( 0.15f, 0.15f, 0.15f, 1.00f );

            style.Colors[ImGuiCol_Header]                = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );
            style.Colors[ImGuiCol_HeaderHovered]         = ImVec4( 0.30f, 0.30f, 0.31f, 1.00f );
            style.Colors[ImGuiCol_HeaderActive]          = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );

            style.Colors[ImGuiCol_DockingPreview]        = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );
            style.Colors[ImGuiCol_CheckMark]             = ImVec4( 0.25f, 0.85f, 0.85f, 1.00f );

            style.Colors[ImGuiCol_Separator]             = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );
            style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );
            style.Colors[ImGuiCol_SeparatorActive]       = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );

            style.Colors[ImGuiCol_Button]                = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );
            style.Colors[ImGuiCol_ButtonHovered]         = ImVec4( 0.30f, 0.30f, 0.30f, 1.00f );
            style.Colors[ImGuiCol_ButtonActive]          = ImVec4( 0.20f, 0.20f, 0.20f, 1.00f );

            style.Colors[ImGuiCol_FrameBg]               = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f );
            style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4( 0.10f, 0.10f, 0.10f, 1.00f);
            style.Colors[ImGuiCol_FrameBgActive]         = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f);

            style.Colors[ImGuiCol_Tab]                   = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f ); // タブ
            style.Colors[ImGuiCol_TabHovered]            = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f ); // タブ ホバー
            style.Colors[ImGuiCol_TabActive]             = ImVec4( 0.15f, 0.15f, 0.15f, 1.00f ); // タブ フォーカス
            style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4( 0.15f, 0.15f, 0.15f, 1.00f ); // タブ フォーカス　非ホバー
            style.Colors[ImGuiCol_TabUnfocused]          = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f ); // タブ 非フォーカス　非ホバー 
            style.Colors[ImGuiCol_TitleBg]               = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f ); // ウィンドウタイトル
            style.Colors[ImGuiCol_TitleBgActive]         = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f ); // ウィンドウタイトル フォーカス
            style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f ); // ウィンドウタイトル 折り畳み
            style.Colors[ImGuiCol_WindowBg]              = ImVec4( 0.15f, 0.15f, 0.15f, 1.00f ); // ウィンドウ 背景

            style.Colors[ImGuiCol_ResizeGrip]            = ImVec4( 0.91f, 0.91f, 0.91f, 0.25f );
            style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4( 0.81f, 0.81f, 0.81f, 0.67f );
            style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4( 0.46f, 0.46f, 0.46f, 0.95f );
            style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4( 0.02f, 0.02f, 0.02f, 0.53f );
            style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4( 0.31f, 0.31f, 0.31f, 1.00f );
            style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4( 0.41f, 0.41f, 0.41f, 1.00f );
            style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4( 0.51f, 0.51f, 0.51f, 1.00f );
            style.Colors[ImGuiCol_SliderGrab]            = ImVec4( 0.51f, 0.51f, 0.51f, 0.70f );
            style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4( 0.66f, 0.66f, 0.66f, 1.00f );

            style.Colors[ImGuiCol_DockingEmptyBg]        = ImVec4( 0.10f, 0.20f, 0.30f, 1.00f ); // DockSpaceの裏背景クリアカラー
        }
    }

    void EditorUI::Shutdown()
    {
        ImGui::DestroyContext();
        Memory::Deallocate(this);
    }

    void EditorUI::BeginFrame()
    {
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void EditorUI::Render()
    {
        ImGui::Render();
    }

    void EditorUI::EndFrame()
    {
        //==============================================================
        // ビューポートを有効にする場合（メインウィンドウ以外でのImGui描画）
        // OpenGLコンテキストの更新を行う必要がある（ウィンドウコンテキストが更新された場合）
        // このコンテキストはスレッド固有で、1度に1つコンテキストに対するOpenGL命令しか
        // 実行できない。スレッドが変わる場合はコンテキストも切り替える必要がある
        //==============================================================
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
#if SL_PLATFORM_OPENGL
            GLFWwindow* prevContext = glfwGetCurrentContext();

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            glfwMakeContextCurrent(prevContext);
#else
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
#endif
        }
    }
}
