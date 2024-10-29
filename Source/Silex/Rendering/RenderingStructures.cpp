
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
        Renderer::Get()->DestroyTexture(this);

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

    void DescriptorSet::SetResource(uint32 binding, TextureViewHandle* view, SamplerHandle* sampler)
    {
        // 現状、サンプラーはCPUからの書き込みをせず、ダブルバッファリングをしていないが、
        // 他のデスクリプター（ubo, ssbo）がダブルバッファリングでCPUからの書き込みを行っているので
        // 空の参照デスクリプターを含む状態でデスクリプターを更新できないので、
        // 他のデスクリプターに合わせる形で、サンプラーも同じデスクリプターの参照で更新する
        for (uint32 i = 0; i < descriptorSetInfo.size(); i++)
        {
            descriptorSetInfo[i].BindTexture(binding, DESCRIPTOR_TYPE_IMAGE_SAMPLER, view, sampler);
        }
    }

    void DescriptorSet::SetResource(uint32 binding, UniformBuffer* uniformBuffer)
    {
        for (uint32 i = 0; i < descriptorSetInfo.size(); i++)
        {
            descriptorSetInfo[i].BindBuffer(binding, DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffer->GetHandle(i));
        }
    }

    void DescriptorSet::SetResource(uint32 binding, StorageBuffer* storageBuffer)
    {
        for (uint32 i = 0; i < descriptorSetInfo.size(); i++)
        {
            descriptorSetInfo[i].BindBuffer(binding, DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBuffer->GetHandle(i));
        }
    }
}
