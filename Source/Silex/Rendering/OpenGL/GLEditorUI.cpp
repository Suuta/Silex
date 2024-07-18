
#include "PCH.h"

#include "Core/Engine.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/OpenGL/GLEditorUI.h"

#include <imgui/imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>



namespace Silex
{
    GLEditorUI::GLEditorUI()
    {
    }

    GLEditorUI::~GLEditorUI()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }

    void GLEditorUI::Init(RenderingContext* context)
    {
        Super::Init(context);
        GLFWwindow* window = Window::Get()->GetGLFWWindow();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 450");
    }

    void GLEditorUI::BeginFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void GLEditorUI::EndFrame()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
