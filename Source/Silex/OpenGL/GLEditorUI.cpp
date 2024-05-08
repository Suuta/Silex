
#include "PCH.h"

#include "Engine/Engine.h"
#include "OpenGL/GLEditorUI.h"

#include <imgui/imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>



namespace Silex
{
    void GLEditorUI::Init()
    {
        SL_LOG_TRACE("GLEditorUI::Init");

        Super::Init();
        GLFWwindow* window = Engine::Get()->GetWindow()->GetGLFWWindow();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 450");
    }

    void GLEditorUI::Shutdown()
    {
        SL_LOG_TRACE("GLEditorUI::Shutdown");

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        Super::Shutdown();
    }

    void GLEditorUI::Render()
    {
        Super::Render();

        ImDrawData* drawData = ImGui::GetDrawData();
        ImGui_ImplOpenGL3_RenderDrawData(drawData);
    }

    void GLEditorUI::BeginFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        Super::BeginFrame();
    }

    void GLEditorUI::EndFrame()
    {
        Render();
        Super::EndFrame();
    }
}
