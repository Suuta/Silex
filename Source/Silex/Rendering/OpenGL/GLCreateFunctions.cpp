
#include "PCH.h"

#include "Rendering/OpenGL/GLFramebuffer.h"
#include "Rendering/OpenGL/GLTexture.h"
#include "Rendering/OpenGL/GLMeshBuffer.h"
#include "Rendering/OpenGL/GLEditorUI.h"
#include "Rendering/OpenGL/GLUniformBuffer.h"
#include "Rendering/OpenGL/GLSkyLight.h"
#include "Rendering/OpenGL/GLRenderer.h"
#include "Rendering/OpenGL/GLStorageBuffer.h"


namespace Silex
{
#if SL_PLATFORM_OPENGL

    //========================================
    // レンダラー
    //========================================
    RendererPlatform* RendererPlatform::Create()
    {
        return Memory::Allocate<GLRenderer>();
    }

    //========================================
    // エディター
    //========================================
    EditorUI* EditorUI::Create()
    {
        return Memory::Allocate<GLEditorUI>();
    }

    //========================================
    // テクスチャ
    //========================================
    Texture2D* Texture2D::Create(const RHI::TextureDesc& desc)
    {
        return Memory::Allocate<GLTexture2D>(desc);
    }

    Shared<Texture2D> Texture2D::Create(const RHI::TextureDesc& desc, const std::string& path)
    {
        return CreateShared<GLTexture2D>(desc, path);
    }

    Texture2DArray* Texture2DArray::Create(const RHI::TextureDesc& desc, uint32 size)
    {
        return Memory::Allocate<GLTexture2DArray>(desc, size);
    }

    TextureCube* TextureCube::Create(const RHI::TextureDesc& desc, const std::string& filePath)
    {
        return Memory::Allocate<GLTextureCube>(desc, filePath);
    }

    //========================================
    // 頂点バッファ
    //========================================
    VertexBuffer* VertexBuffer::Create(void* data, uint32 byteSize)
    {
        return Memory::Allocate<GLVertexBuffer>(data, byteSize);
    }

    //========================================
    // インデックスバッファ
    //========================================
    IndexBuffer* IndexBuffer::Create(void* data, uint32 byteSize)
    {
        return Memory::Allocate<GLIndexBuffer>(data, byteSize);
    }

    //========================================
    // インスタンスバッファ
    //========================================
#if 0
    InstanceBuffer* InstanceBuffer::Create(void* data, uint32 size)
    {
        return Memory::Allocate<GLInstanceBuffer>(data, size);
    }
#endif

    //========================================
    // フレームバッファ
    //========================================
    Shared<Framebuffer> Framebuffer::Create(const RHI::FramebufferDesc& desc)
    {
        return CreateShared<GLFramebuffer>(desc);
    }

    //========================================
    // ユニフォームバッファ
    //========================================
    Shared<UniformBuffer> UniformBuffer::Create(uint32 size, uint32 slot, void* data)
    {
        return CreateShared<GLUniformBuffer>(size, slot, data);
    }

    //========================================
    // ストレージバッファ
    //========================================
    Shared<StorageBuffer> StorageBuffer::Create(uint32 size, uint32 slot, void* data)
    {
        return CreateShared<GLStorageBuffer>(size, slot, data);
    }

    //========================================
    // スカイボックス
    //========================================
    Shared<SkyLight> SkyLight::Create(const std::string& filePath)
    {
        return CreateShared<GLSkyLight>(filePath);
    }

#endif
}