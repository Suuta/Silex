
#pragma once

#include "Core/Core.h"
#include "Core/SharedPointer.h"
#include "Renderer/RenderCore.h"


namespace Silex
{
    class UniformBuffer : public Object
    {
        SL_DECLARE_CLASS(UniformBuffer, Object)

    public:

        static Shared<UniformBuffer> Create(uint32 size, uint32 slot, void* data = nullptr);

        virtual ~UniformBuffer() = default;

        virtual void SetData(uint32 offset, uint32 size, const void* data) = 0;
        virtual uint32 GetID() const = 0;
    };
}
