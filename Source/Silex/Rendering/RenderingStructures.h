
#pragma once

#include "Rendering/RenderingCore.h"
#include "Rendering/Renderer.h"


namespace Silex
{
    class Buffer;
    class IndexBuffer;
    class VertexBuffer;
    class UniformBuffer;
    class StorageBuffer;
    class Sampler;
    class Texture;
    class Texture2D;
    class Texture2DArray;
    class TextureCube;
    class TextureView;
    class DescriptorSet;
    class CommandQueue;
    class CommandBuffer;
    class CommandPool;
    class Fence;
    class Semaphore;
    class Surface;
    class Shader;
    class SwapChain;
    class RenderPass;
    class Framebuffer;
    class Pipeline;

    template<class T> struct RenderingTypeTraits {};
    template<> struct RenderingTypeTraits<Sampler>        { using InternalType = SamplerHandle;       };
    template<> struct RenderingTypeTraits<Texture>        { using InternalType = TextureHandle;       };
    template<> struct RenderingTypeTraits<Texture2D>      { using InternalType = TextureHandle;       };
    template<> struct RenderingTypeTraits<Texture2DArray> { using InternalType = TextureHandle;       };
    template<> struct RenderingTypeTraits<TextureCube>    { using InternalType = TextureHandle;       };
    template<> struct RenderingTypeTraits<TextureView>    { using InternalType = TextureViewHandle;   };
    template<> struct RenderingTypeTraits<Buffer>         { using InternalType = BufferHandle;        };
    template<> struct RenderingTypeTraits<IndexBuffer>    { using InternalType = BufferHandle;        };
    template<> struct RenderingTypeTraits<VertexBuffer>   { using InternalType = BufferHandle;        };
    template<> struct RenderingTypeTraits<UniformBuffer>  { using InternalType = BufferHandle;        };
    template<> struct RenderingTypeTraits<StorageBuffer>  { using InternalType = BufferHandle;        };
    template<> struct RenderingTypeTraits<DescriptorSet>  { using InternalType = DescriptorSetHandle; };
    template<> struct RenderingTypeTraits<Shader>         { using InternalType = ShaderHandle;        };
    template<> struct RenderingTypeTraits<RenderPass>     { using InternalType = RenderPassHandle;    };
    template<> struct RenderingTypeTraits<Framebuffer>    { using InternalType = FramebufferHandle;   };
    template<> struct RenderingTypeTraits<Pipeline>       { using InternalType = PipelineHandle;      };
    template<> struct RenderingTypeTraits<Surface>        { using InternalType = SurfaceHandle;       };
    template<> struct RenderingTypeTraits<SwapChain>      { using InternalType = SwapChainHandle;     };
    template<> struct RenderingTypeTraits<Semaphore>      { using InternalType = SemaphoreHandle;     };
    template<> struct RenderingTypeTraits<Fence>          { using InternalType = FenceHandle;         };
    template<> struct RenderingTypeTraits<CommandQueue>   { using InternalType = CommandQueueHandle;  };
    template<> struct RenderingTypeTraits<CommandBuffer>  { using InternalType = CommandBufferHandle; };
    template<> struct RenderingTypeTraits<CommandPool>    { using InternalType = CommandPoolHandle;   };


    //==============================================================
    // レンダリング構造体 ベース
    //==============================================================
    template<class T>
    class RenderingStructure : public Class
    {
        friend class Renderer;

    public:
        
        // 実体化時にハッシュ値が衝突するので宣言できない（現状問題ないので使用しないことで対応）
        // SL_CLASS(RenderingStructure, Class)

        using ResourceType = typename RenderingTypeTraits<T>::InternalType;

        RenderingStructure()
        {
            uint32 numFrameResouces = Renderer::Get()->GetFrameCountInFlight();
            handle.resize(numFrameResouces);
        }

        ResourceType* GetHandle(uint32 index = 0)
        {
            return handle[index];
        }

        void SetHandle(ResourceType* resource, uint32 index = 0)
        {
            handle[index] = resource;
        }

    protected:

        std::vector<ResourceType*> handle = {};
    };


    //==============================================================
    // テクスチャ
    //==============================================================
    class Texture : public RenderingStructure<Texture>
    {
        friend class Renderer;

    public:

        SL_CLASS(Texture, RenderingStructure<Texture>)

        const TextureInfo& GetInfo()       { return textureInfo; }
        const TextureInfo& GetInfo() const { return textureInfo; }

        uint32          GetWidth()  { return textureInfo.width;  }
        uint32          GetHeight() { return textureInfo.height; }
        RenderingFormat GetFormat() { return textureInfo.format; }

    protected:

        TextureInfo textureInfo;
    };

    //==============================================================
    // テクスチャ2D
    //==============================================================
    class Texture2D : public Texture
    {
        friend class Renderer;

    public:

        SL_CLASS(Texture2D, Texture)
        void Resize(uint32 width, uint32 height);
    };

    //==============================================================
    // テクスチャ2Dアレイ
    //==============================================================
    class Texture2DArray : public Texture
    {
        friend class Renderer;

    public:

        SL_CLASS(Texture2DArray, Texture)

        // 現状リサイズはサポートしていない
        // 使用しているのはシャドウマップであり、固定サイズなので
        //void Resize(uint32 width, uint32 height);
    };

    //==============================================================
    // テクスチャキューブ
    //==============================================================
    class TextureCube : public Texture
    {
        friend class Renderer;

    public:

        SL_CLASS(TextureCube, Texture)

        // 現状リサイズはサポートしていない
        // 使用しているのは環境マップであり、固定サイズなので
        //void Resize(uint32 width, uint32 height);
    };




    //==============================================================
    // バッファ
    //==============================================================
    class Buffer : public RenderingStructure<Buffer>
    {
        friend class Renderer;

    public:

        SL_CLASS(Buffer, RenderingStructure<Buffer>)

        uint64 GetByteSize()      { return byteSize;  }
        bool   IsMapped()         { return isMapped;  }
        bool   IsCPU()            { return isInCPU;   }
        void*  GetMappedPointer() { return mappedPtr; }

    private:

        uint64 byteSize  = 0;
        bool   isMapped  = false;
        bool   isInCPU   = false;
        void*  mappedPtr = nullptr;
    };

    //==============================================================
    // 頂点バッファ
    //==============================================================
    class VertexBuffer : public Buffer
    {
        friend class Renderer;

    public:

        SL_CLASS(VertexBuffer, Buffer)

        uint64 GetVertexCount()    { return vertexCount;    }
        uint64 GetVertexByteSize() { return vertexByteSize; }

    private:

        uint64 vertexCount    = 0;
        uint64 vertexByteSize = 0;
    };

    //==============================================================
    // インデックスバッファ
    //==============================================================
    class IndexBuffer : public Buffer
    {
        friend class Renderer;

    public:
        
        SL_CLASS(IndexBuffer, Buffer)

        uint64 GetIndexCount()    { return indexCount;    }
        uint64 GetIndexByteSize() { return indexByteSize; }

    private:

        uint64 indexCount    = 0;
        uint64 indexByteSize = 0;
    };

    //==============================================================
    // ユニフォームバッファ
    //==============================================================
    class UniformBuffer : public Buffer
    {
        friend class Renderer;

    public:
        
        SL_CLASS(UniformBuffer, Buffer)
        void SetData(const void* data, uint64 writeByteSize);

    private:

        //void* data = nullptr;
    };

    //==============================================================
    // ストレージバッファ
    //==============================================================
    class StorageBuffer : public Buffer
    {
        friend class Renderer;

    public:

        SL_CLASS(StorageBuffer, Buffer)

        void SetData(const void* data, uint64 writeByteSize);

    private:

        //void* data = nullptr;
    };



    class TextureView : public RenderingStructure<TextureView>
    {
        friend class Renderer;

    public:

        SL_CLASS(TextureView, RenderingStructure<TextureView>)
    };

    class Sampler : public RenderingStructure<Sampler>
    {
        friend class Renderer;

    public:

        SL_CLASS(Sampler, RenderingStructure<Sampler>)
    };


    //==============================================================
    // デスクリプターセット
    //==============================================================
    class DescriptorSet : public RenderingStructure<DescriptorSet>
    {
        friend class Renderer;

    public:
        
        SL_CLASS(DescriptorSet, RenderingStructure<DescriptorSet>)
        DescriptorSet();

        void Flush();

        //TODO: ビュー/サンプラー もラップクラスが実装後に変更する
        void SetResource(uint32 binding, TextureViewHandle* view, SamplerHandle* sampler);

        void SetResource(uint32 binding, UniformBuffer* uniformBuffer);
        void SetResource(uint32 binding, StorageBuffer* storageBuffer);

    private:

        //=====================================================================================
        // TODO: デスクリプターセットのベイクを実装
        //=====================================================================================
        // デスクリプターセットの構成は基本的には静的なので（シェーダーをランタイムで書き換えない限り）
        // このデスクリプターセットと依存関係を持つリソース（イメージ・バッファ）のデスクリプタをラップするクラス
        // の参照を保持（ベイク）する。リソースに変更があった場合、そのデスクリプタを更新（updateDescriptorSet）
        // したい。また、シェーダーの変更やデスクリプターへの入力による構成の変更があれば、再度ベイクすればよい
        // 
        // 現状は、この仕組みができていないのに加えて、マルチバッファリングしているリソースが変更された時に
        // N-1 フレーム（GPUで処理中）が実行中のリソースに対してデスクリプターを更新できないため、
        // 一度デスクリプタを削除し、両フレームのリソースを更新することで対応している（削除は N+1 フレームに実行）
        // 
        // godot エンジンの実装では リソースのダブルバッファリングではなく、コピーキューによる転送と
        // コピーステージ待機によって同期しているようなので、変更があるたびにデスクリプター自体を再生成している
        // ようにみえる。
        // 
        // hazel エンジンでは、リソースをダブルバッファリングし、毎フレーム変更点を検知し、変更部分を更新する
        //=====================================================================================

        std::vector<DescriptorSetInfo> descriptorSetInfo;
    };
}
