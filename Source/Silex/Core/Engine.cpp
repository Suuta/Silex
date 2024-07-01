
#include "PCH.h"

#include "Core/Engine.h"
#include "Asset/Asset.h"
#include "Editor/SplashImage.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/OpenGL/GLEditorUI.h"


namespace Silex
{
    static Engine* engine     = nullptr;
    static Window* mainWindow = nullptr;


    bool LaunchEngine()
    {
        // OS初期化
        OS::Get()->Initialize();

        // スプラッシュイメージ表示
        SplashImage::Show();

        // コア機能初期化
        Logger::Initialize();
        Memory::Initialize();
        Input::Initialize();

        SL_LOG_INFO("***** Launch Engine *****");

        // エンジン初期化
        engine = Memory::Allocate<Engine>();
        if (!engine->Initialize())
        {
            return false;
        }

        // スプラッシュイメージ非表示
        SplashImage::Hide();

        return true;
    }

    void ShutdownEngine()
    {
        if (engine)
        {
            engine->Finalize();
            Memory::Deallocate(engine);
        }

        SL_LOG_INFO("***** Shutdown Engine *****");

        Input::Finalize();
        Memory::Finalize();
        Logger::Finalize();

        OS::Get()->Finalize();
    }




    //=========================================
    // Engine
    //=========================================
    Engine* Engine::Get()
    {
        return engine;
    }

    bool Engine::Initialize()
    {
        bool result = false;

        // ウィンドウ
        mainWindow = Window::Create("Silex", 1280, 720);
        result = mainWindow->Initialize();
        SL_CHECK(!result, false);

        // レンダリングコンテキスト生成
        renderingContext = RenderingContext::Create(mainWindow->GetPlatformHandle());
        result = renderingContext->Initialize(true);
        SL_CHECK(!result, false);

        // レンダリングデバイス生成 (描画APIを抽象化)
        renderingDevice = Memory::Allocate<RenderingDevice>();
        result = renderingDevice->Initialize(renderingContext);
        SL_CHECK(!result, false);

        // ウィンドウコンテキスト生成
        result = mainWindow->SetupWindowContext(renderingContext);
        SL_CHECK(!result, false);

        // コールバック登録
        mainWindow->BindWindowCloseEvent(this,  &Engine::OnWindowClose);
        mainWindow->BindWindowResizeEvent(this, &Engine::OnWindowResize);
        //mainWindow->BindMouseMoveEvent(this,    &Engine::OnMouseMove);
        //mainWindow->BindMouseScrollEvent(this,  &Engine::OnMouseScroll);


        // レンダラー
        //Renderer::Get()->Init();

        // アセットマネージャー
        //AssetManager::Get()->Init();

        // エディターUI (ImGui)
        //editorUI = EditorUI::Create();
        //editorUI->Init();

        // エディター
        //editor = Memory::Allocate<Editor>();
        //editor->Init();

        // ウィンドウ表示
        mainWindow->Show();

        return true;
    }

    void Engine::Finalize()
    {
        // if (editor)   editor->Shutdown();
        // if (editorUI) editorUI->Shutdown();
        //AssetManager::Get()->Shutdown();
        //Renderer::Get()->Shutdown();


        // ウィンドウコンテキスト破棄
        mainWindow->CleanupWindowContext(renderingContext);

        // レンダリングデバイス破棄
        Memory::Deallocate(renderingDevice);

        // レンダリングコンテキスト破棄
        Memory::Deallocate(renderingContext);

        // ウィンドウ破棄
        Memory::Deallocate(mainWindow);
    }

    bool Engine::MainLoop()
    {
        CalcurateFrameTime();

        if (!minimized)
        {
            //Renderer::Get()->BeginFrame();
            //editorUI->BeginFrame();

            //editor->Update(deltaTime);
            //editor->Render();

            //editorUI->EndFrame();
            //Renderer::Get()->EndFrame();

            Input::Flush();
        }

        PerformanceProfiler::Get().GetFrameData(&performanceData, true);

        // メインループ抜け出し確認
        return isRunning;
    }

    void Engine::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.width == 0 || e.height == 0)
        {
            minimized = true;
            return;
        }

        minimized = false;
    }

    void Engine::OnWindowClose(WindowCloseEvent& e)
    {
        isRunning = false;
    }

    void Engine::OnMouseMove(MouseMoveEvent& e)
    {
        editor->OnMouseMove(e);
    }

    void Engine::OnMouseScroll(MouseScrollEvent& e)
    {
        editor->OnMouseScroll(e);
    }

    void Engine::CalcurateFrameTime()
    {
        uint64 time = OS::Get()->GetTickSeconds();
        deltaTime     = (double)(time - lastFrameTime) / 1'000'000;
        lastFrameTime = time;

        static float  secondLeft = 0.0f;
        static uint32 frame = 0;
        secondLeft += deltaTime;
        frame++;

        if (secondLeft >= 1.0f)
        {
            frameRate  = frame;
            frame      = 0;
            secondLeft = 0.0f;
        }
    }

    // OnWindowCloseイベント（Xボタン / Alt + F4）以外で、エンジンループを終了させる場合に使用
    void Engine::Close()
    {
        isRunning = false;
    }
}
