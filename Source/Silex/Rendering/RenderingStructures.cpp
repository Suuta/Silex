
#include "PCH.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingStructures.h"


namespace Silex
{
    void Texture2D::Resize(uint32 width, uint32 height)
    {
        Renderer::Get()->DestroyTexture(handle[0]);

        textureInfo.width  = width;
        textureInfo.height = width;
        handle[0] = Renderer::Get()->CreateTexture(textureInfo);
    }

    void* UniformBuffer::GetData()
    {
        return data;
    }

    void UniformBuffer::SetData(const void* data, uint64 writeByteSize)
    {
        uint32 frameindex = Renderer::Get()->GetCurrentFrameIndex();
        Renderer::Get()->UpdateBufferData(handle[frameindex], data, writeByteSize);
    }

    void* StorageBuffer::GetData()
    {
        return data;
    }

    void StorageBuffer::SetData(const void* data, uint64 writeByteSize)
    {
        uint32 frameindex = Renderer::Get()->GetCurrentFrameIndex();
        Renderer::Get()->UpdateBufferData(handle[frameindex], data, writeByteSize);
    }

    void DescriptorSet::Flush()
    {
        Renderer::Get()->UpdateDescriptorSet(handle[0], descriptorSetInfo[0]);
        Renderer::Get()->UpdateDescriptorSet(handle[1], descriptorSetInfo[1]);
    }

    void DescriptorSet::SetResource(uint32 index, TextureView* view, Sampler* sampler)
    {
        descriptorSetInfo[0].BindTexture(index, DESCRIPTOR_TYPE_IMAGE_SAMPLER, view->GetHandle(), sampler->GetHandle());
    }

    void DescriptorSet::SetResource(uint32 index, UniformBuffer* uniformBuffer)
    {
        descriptorSetInfo[0].BindBuffer(index, DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffer->GetHandle(0));
        descriptorSetInfo[1].BindBuffer(index, DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffer->GetHandle(1));
    }

    void DescriptorSet::SetResource(uint32 index, StorageBuffer* storageBuffer)
    {
        descriptorSetInfo[0].BindBuffer(index, DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBuffer->GetHandle(0));
        descriptorSetInfo[1].BindBuffer(index, DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBuffer->GetHandle(1));
    }
}
