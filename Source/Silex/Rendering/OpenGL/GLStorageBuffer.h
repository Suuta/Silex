
#pragma once

#include "Rendering/StorageBuffer.h"


namespace Silex
{
    class GLStorageBuffer : public StorageBuffer
    {
        SL_CLASS(GLStorageBuffer, StorageBuffer)

    public:

        GLStorageBuffer(uint32 size, uint32 slot, void* data);
        ~GLStorageBuffer();

        void SetData(uint32 offset, uint32 size, const void* data)          override;
        void ReCreate(uint32 slot, uint32 size, const void* data = nullptr) override;

        uint32 GetID() const override { return ID; }

    private:

        uint32 ID;
        uint64 Size;
        void*  Data;
        void*  MappedPtr;
    };
}
