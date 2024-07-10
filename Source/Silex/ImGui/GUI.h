
#pragma once

#include "Core/SharedPointer.h"


namespace Silex
{
    class RenderingContext;
    using GUICreateFunction = class GUI* (*)();


    class GUI : public Object
    {
        SL_CLASS(GUI, Object)

    public:

        GUI();
        ~GUI();

        // 生成
        static GUI* Create()
        {
            return createFunction();
        }

        // 生成用関数の登録
        static void ResisterCreateFunction(GUICreateFunction func)
        {
            createFunction = func;
        }

    public:

        virtual void Init(RenderingContext* context);

        virtual void BeginFrame();
        virtual void EndFrame();

        void Render();

    private:

        static inline GUICreateFunction createFunction = nullptr;
    };
}
