
#pragma once

#include "Rendering/MeshBuffer.h"


namespace Silex
{
    //========================================
    // 頂点バッファ
    //========================================
    class GLVertexBuffer : public VertexBuffer
    {
        SL_DECLARE_CLASS(GLVertexBuffer, VertexBuffer)

    public:

        GLVertexBuffer(void* data, uint32 size);
        ~GLVertexBuffer();

        void SetData(void* data, uint32 byteSize, uint32 offset = 0) override;

        void Bind()                                                        const override;
        void BindOffset(uint32 bindingindex, uint32 offset, uint32 stride) const override;
        void Unbind()                                                      const override;

        uint32 GetCount() const override { return Size / sizeof(float); }
        uint32 GetSize()  const override { return Size;                 }
        uint32 GetID()    const override { return ID;                   }

        void* GetRawData() const override { return Data; }

    private:

        uint32 ID;
        uint32 Size;

        void* Data;
    };


    //========================================
    // インデックスバッファ
    //========================================
    class GLIndexBuffer : public IndexBuffer
    {
        SL_DECLARE_CLASS(GLIndexBuffer, IndexBuffer)

    public:

        GLIndexBuffer(void* data, uint32 size);
        virtual ~GLIndexBuffer();

        void SetData(void* data, uint32 size, uint32 offset = 0) override;
        void Bind()   const                                      override;
        void Unbind() const                                      override;

        uint32 GetCount() const override { return Size / sizeof(uint32); }
        uint32 GetSize()  const override { return Size;                  }
        uint32 GetID()    const override { return ID;                    }

        void* GetRawData() const override { return Data; }

    private:

        uint32 ID;
        uint32 Size;

        void* Data;
    };


#if 0
    class GLInstanceBuffer : public InstanceBuffer
    {
        SL_DECLARE_CLASS(GLInstanceBuffer, InstanceBuffer)

    public:

        GLInstanceBuffer(void* data, uint32 size);
        virtual ~GLInstanceBuffer();

        virtual void Resize(void* data, uint32 byteSize) override;
        virtual void SetData(void* data, uint32 byteSize, uint32 offset = 0) override;

        virtual void Bind()   const override;
        virtual void Unbind() const override;

    private:

        uint32 ID;
        uint32 Size;

        void* Data;
    };
#endif
}
