
#pragma once

#include "Core/Core.h"
#include "Core/SharedPointer.h"
#include "Rendering/RenderDefine.h"


namespace Silex
{
    class StorageBuffer : public Object
    {
        SL_CLASS(StorageBuffer, Object)

    public:

        virtual void SetData(uint32 offset, uint32 size, const void* data)          = 0;
        virtual void ReCreate(uint32 slot, uint32 size, const void* data = nullptr) = 0;

        virtual uint32 GetID() const = 0;

    public:

        static Shared<StorageBuffer> Create(uint32 size, uint32 slot, void* data = nullptr);
    };
}
