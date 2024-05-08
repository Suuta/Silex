
#include "PCH.h"

#include "OpenGL/GLMeshBuffer.h"
#include "OpenGL/OpenGLCore.h"


namespace Silex
{
    //========================================
    // 頂点バッファ
    //========================================
    GLVertexBuffer::GLVertexBuffer(void* data, uint32 size)
        : ID(0)
        , Size(size)
        , Data(data)
    {
        glCreateBuffers(1, &ID);
        glNamedBufferData(ID, Size, Data, GL_STATIC_DRAW);
    }

    GLVertexBuffer::~GLVertexBuffer()
    {
        glDeleteBuffers(1, &ID);
    }

    void GLVertexBuffer::SetData(void* data, uint32 byteSize, uint32 offset)
    {
        glNamedBufferSubData(ID, offset, byteSize, data);
    }

    void GLVertexBuffer::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, ID);
    }

    void GLVertexBuffer::BindOffset(uint32 bindingindex, uint32 offset, uint32 stride) const
    {
        glBindVertexBuffer(bindingindex, ID, offset, stride);
    }

    void GLVertexBuffer::Unbind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    //========================================
    // インデックスバッファ
    //========================================
    GLIndexBuffer::GLIndexBuffer(void* data, uint32 byteSize)
        : ID(0)
        , Size(byteSize)
        , Data(data)
    {
        glCreateBuffers(1, &ID);
        glNamedBufferData(ID, Size, Data, GL_STATIC_DRAW);
    }

    GLIndexBuffer::~GLIndexBuffer()
    {
        glDeleteBuffers(1, &ID);
    }

    void GLIndexBuffer::SetData(void* data, uint32 byteSize, uint32 offset)
    {
        glNamedBufferSubData(ID, offset, byteSize, data);
    }

    void GLIndexBuffer::Bind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
    }

    void GLIndexBuffer::Unbind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }


    //========================================
    // インスタンスバッファ
    //========================================
#if 0
    GLInstanceBuffer::GLInstanceBuffer(void* data, uint32 size)
        : ID(0)
        , Size(size)
        , Data(data)
    {
        glCreateBuffers(1, &ID);
        glNamedBufferData(ID, Size, Data, GL_DYNAMIC_DRAW);
    }

    GLInstanceBuffer::~GLInstanceBuffer()
    {
        glDeleteBuffers(1, &ID);
    }

    void GLInstanceBuffer::Resize(void* data, uint32 byteSize)
    {
        Data = data;
        Size = byteSize;

        glNamedBufferData(ID, Size, Data, GL_DYNAMIC_DRAW);
    }

    void GLInstanceBuffer::SetData(void* data, uint32 byteSize, uint32 offset)
    {
        glNamedBufferSubData(ID, offset, byteSize, data);
    }

    void GLInstanceBuffer::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, ID);
    }

    void GLInstanceBuffer::Unbind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
#endif
}
