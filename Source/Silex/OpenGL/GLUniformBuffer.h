
#pragma once

#include "Renderer/UniformBuffer.h"


namespace Silex
{
    //OpenGL ユニフォームブロック メモリレイアウト
    //https://qiita.com/hoboaki/items/b188c4495f4708c19002

    class GLUniformBuffer : public UniformBuffer
    {
        SL_DECLARE_CLASS(GLUniformBuffer, UniformBuffer)

    public:

        GLUniformBuffer(uint32 size, uint32 slot, void* data);
        ~GLUniformBuffer();

        void SetData(uint32 offset, uint32 size, const void* data) override;
        uint32 GetID() const override { return ID; }

    private:

        uint32 ID;
        uint64 Size;
        void*  Data;
    };
}
