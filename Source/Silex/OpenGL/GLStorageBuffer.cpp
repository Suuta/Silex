
#include "PCH.h"

#include "OpenGL/GLStorageBuffer.h"
#include <glad/glad.h>


namespace Silex
{
    //===========================================================
    // リリースビルドで正しく同期がとれていないのか、チラつきが見られるので、
    // 従来の glBufferSubData 系で更新している。
    //===========================================================
#define USE_MAPPING 0
#if USE_MAPPING
    GLStorageBuffer::GLStorageBuffer(uint32 size, uint32 slot, void* data)
        : ID(0)
        , Size(size)
        , Data(data)
        , MappedPtr(nullptr)
    {
        glCreateBuffers(1, &ID);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, ID);

        uint32 flags =
            GL_MAP_WRITE_BIT      |
            GL_MAP_PERSISTENT_BIT ;
            GL_MAP_COHERENT_BIT   ;

        glNamedBufferStorage(ID, size, data, flags);
        MappedPtr = glMapNamedBufferRange(ID, 0, size, flags);
    }

    GLStorageBuffer::~GLStorageBuffer()
    {
        if (MappedPtr)
            glUnmapNamedBuffer(ID);

        glDeleteBuffers(1, &ID);
    }

    void GLStorageBuffer::SetData(uint32 offset, uint32 size, const void* data)
    {
        std::memcpy((char*)MappedPtr + offset, data, size);
    }

    void GLStorageBuffer::ReCreate(uint32 slot, uint32 size, const void* data)
    {
        if (MappedPtr)
        {
            glUnmapNamedBuffer(ID);
            MappedPtr = nullptr;
        }

        glDeleteBuffers(1, &ID);
        glCreateBuffers(1, &ID);

        uint32 flags = 
            GL_MAP_WRITE_BIT      |
            GL_MAP_PERSISTENT_BIT |
            GL_MAP_COHERENT_BIT   ;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, ID);
        glNamedBufferStorage(ID, size, data, flags);

        MappedPtr = glMapNamedBufferRange(ID, 0, size, flags);
    }
#else

    GLStorageBuffer::GLStorageBuffer(uint32 size, uint32 slot, void* data)
        : ID(0), Size(size), Data(data), MappedPtr(nullptr)
    {
        glCreateBuffers(1, &ID);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, ID);

        glNamedBufferStorage(ID, size, data, GL_DYNAMIC_STORAGE_BIT);
    }

    GLStorageBuffer::~GLStorageBuffer()
    {
        glDeleteBuffers(1, &ID);
    }

    void GLStorageBuffer::SetData(uint32 offset, uint32 size, const void* data)
    {
        glNamedBufferSubData(ID, offset, size, data);
    }

    void GLStorageBuffer::ReCreate(uint32 slot, uint32 size, const void* data)
    {
        glDeleteBuffers(1, &ID);
        glCreateBuffers(1, &ID);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, ID);
        glNamedBufferStorage(ID, size, data, GL_DYNAMIC_STORAGE_BIT);
    }

#endif
}
