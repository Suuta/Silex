
#include "PCH.h"

#include "Rendering/OpenGL/GLUniformBuffer.h"
#include <glad/glad.h>


namespace Silex
{
    GLUniformBuffer::GLUniformBuffer(uint32 size, uint32 slot, void* data)
        : ID(0)
        , Size(size)
        , Data(data)
    {
        glCreateBuffers(1, &ID);
        glBindBufferBase(GL_UNIFORM_BUFFER, slot, ID);
        glNamedBufferData(ID, size, data, GL_DYNAMIC_DRAW);
    }

    GLUniformBuffer::~GLUniformBuffer()
    {
        glDeleteBuffers(1, &ID);
    }

    void GLUniformBuffer::SetData(uint32 offset, uint32 size, const void* data)
    {
        glNamedBufferSubData(ID, offset, size, data);
    }
}
