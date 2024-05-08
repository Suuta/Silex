
#pragma once

#include "Engine/Module.h"
#include "Core/SharedPointer.h"


namespace Silex
{
    class EditorUI : public Module
    {
        SL_DECLARE_CLASS(EditorUI, Module)

    public:

        virtual ~EditorUI() = default;

    public:

        static EditorUI* Create();


        void Init()                  override;
        void Shutdown()              override;
        void Render()                override;

        void Update(float deltaTime) override {};

    public:

        virtual void BeginFrame();
        virtual void EndFrame();

    private:


    };
}
