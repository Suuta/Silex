
#include "PCH.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingStructures.h"


namespace Silex
{
    //==============================================================
    // テクスチャ
    //==============================================================
    void Texture2D::Resize(uint32 width, uint32 height)
    {
        Renderer::Get()->DestroyTexture(handle[0]);

        textureInfo.width  = width;
        textureInfo.height = width;
        handle[0] = Renderer::Get()->CreateTexture(textureInfo);
    }

    //==============================================================
    // ユニフォームバッファ
    //==============================================================
    void UniformBuffer::SetData(const void* data, uint64 writeByteSize)
    {
        uint32 frameindex = Renderer::Get()->GetCurrentFrameIndex();
        Renderer::Get()->UpdateBufferData(handle[frameindex], data, writeByteSize);
    }

    //==============================================================
    // ストレージバッファ
    //==============================================================
    void StorageBuffer::SetData(const void* data, uint64 writeByteSize)
    {
        uint32 frameindex = Renderer::Get()->GetCurrentFrameIndex();
        Renderer::Get()->UpdateBufferData(handle[frameindex], data, writeByteSize);
    }

    //==============================================================
    // デスクリプターセット
    //==============================================================

    DescriptorSet::DescriptorSet()
    {
        descriptorSetInfo.resize(handle.size());
    }

    void DescriptorSet::Flush()
    {
        for (uint32 i = 0; i < handle.size(); i++)
        {
            Renderer::Get()->UpdateDescriptorSet(handle[i], descriptorSetInfo[i]);
        }
    }

    void DescriptorSet::SetResource(uint32 setIndex, TextureView* view, Sampler* sampler)
    {
        descriptorSetInfo[0].BindTexture(setIndex, DESCRIPTOR_TYPE_IMAGE_SAMPLER, view->GetHandle(), sampler->GetHandle());
    }

    void DescriptorSet::SetResource(uint32 setIndex, UniformBuffer* uniformBuffer)
    {
        for (uint32 i = 0; i < descriptorSetInfo.size(); i++)
        {
            descriptorSetInfo[i].BindBuffer(setIndex, DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffer->GetHandle(i));
        }
    }

    void DescriptorSet::SetResource(uint32 setIndex, StorageBuffer* storageBuffer)
    {
        for (uint32 i = 0; i < descriptorSetInfo.size(); i++)
        {
            descriptorSetInfo[i].BindBuffer(setIndex, DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBuffer->GetHandle(i));
        }
    }
}
