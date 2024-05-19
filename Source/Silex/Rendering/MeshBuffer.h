
#pragma once

#include "Core/Core.h"
#include "Rendering/RenderDefine.h"


namespace Silex
{
    struct VertexBufferElement
    {
        const char*         Name         = {};
        RHI::ShaderDataType Type         = {};
        uint32              Size         = 0;
        uint32              Offset       = 0;
        uint32              LayoutIndex  = 0;
    };

    class VertexBufferLayout
    {
    public:

        void Add(uint32 layoutIndex, const char* name, RHI::ShaderDataType type)
        {
            uint64 size = RHI::ShaderDataTypeSize(type);
            uint64 offset = 0;

            if (!Elements.empty())
            {
                VertexBufferElement& b = Elements.back();
                offset = b.Size + b.Offset;
            }

            VertexBufferElement& e = Elements.emplace_back();
            e.Name         = name;
            e.Type         = type;
            e.Size         = size;
            e.Offset      += offset;
            e.LayoutIndex  = layoutIndex;

            Stride += size;
        }

        std::vector<VertexBufferElement> Elements = {};
        uint32                           Stride   = 0;
    };


    //========================================
    // 頂点バッファ
    //========================================
    class VertexBuffer : public Object
    {
        SL_CLASS(VertexBuffer, Object)

    public:

        virtual ~VertexBuffer() {}

        virtual void SetData(void* buffer, uint32 byteSize, uint32 offset = 0) = 0;
        virtual void Bind()                                                        const = 0;
        virtual void BindOffset(uint32 bindingindex, uint32 offset, uint32 stride) const = 0;
        virtual void Unbind()                                                      const = 0;

        virtual uint32 GetCount() const = 0;
        virtual uint32 GetSize()  const = 0;
        virtual uint32 GetID()    const = 0;

        virtual void* GetRawData() const = 0;

    public:

        static VertexBuffer* Create(void* data, uint32 byteSize);
    };

    //========================================
    // インデックスバッファ
    //========================================
    class IndexBuffer : public Object
    {
        SL_CLASS(IndexBuffer, Object)

    public:

        virtual ~IndexBuffer() {}

        virtual void SetData(void* buffer, uint32 byteSize, uint32 offset = 0) = 0;
        virtual void Bind()   const = 0;
        virtual void Unbind() const = 0;

        virtual uint32 GetCount() const = 0;
        virtual uint32 GetSize()  const = 0;
        virtual uint32 GetID()    const = 0;

        virtual void* GetRawData() const = 0;

        static IndexBuffer* Create(void* data, uint32 byteSize);
    };


    //========================================
    // インスタンスバッファ
    //========================================
    class InstanceBuffer : public Object
    {
        SL_CLASS(InstanceBuffer, Object)

    public:

        virtual ~InstanceBuffer() {}

        virtual void Resize(void* buffer, uint32 byteSize) = 0;
        virtual void SetData(void* buffer, uint32 byteSize, uint32 offset = 0) = 0;

        virtual void Bind()   const = 0;
        virtual void Unbind() const = 0;

    public:

        static InstanceBuffer* Create(void* data, uint32 size);
    };
}
