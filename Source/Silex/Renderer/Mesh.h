#pragma once

#include "Renderer/MeshBuffer.h"
#include "Renderer/Material.h"
#include "Asset/Asset.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>


namespace Silex
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        //glm::vec3 Tangent;
        //glm::vec3 Bitangent;
    };

    struct MeshTexture
    {
        uint32      Albedo = 0;
        std::string Path;
    };

    //============================================
    // メッシュの頂点情報クラス
    //--------------------------------------------
    // 頂点データ・インデックスデータの管理
    //============================================
    class MeshSource : public Object
    {
        SL_DECLARE_CLASS(MeshSource, Object)

    public:

        MeshSource(std::vector<Vertex>& vertices, std::vector<uint32>& indices, uint32 materialIndex);
        MeshSource(void* rawVertexData, uint64 vertexByteSize, void* rawIndexData, uint64 indexByteSize, const VertexBufferLayout& vertexAttribute, const VertexBufferLayout& instanceAttribute);
        ~MeshSource();

        void Bind()   const;
        void Unbind() const;

        void SetVertexLayout(const VertexBufferLayout& vertexAttribute, const VertexBufferLayout& instanceAttribute);

        uint32    GetID()            const { return m_ID;                }
        uint64    GetVertexCount()   const { return m_VertexCount;       }
        uint64    GetIndexCount()    const { return m_IndexCount;        }
        bool      HasIndex()         const { return m_HasIndex;          }
        uint32    GetMaterialIndex() const { return m_MaterialIndex;     }
        glm::mat4 GetTransform()     const { return m_RelativeTransform; }

        void SetTransform(const glm::mat4& matrix) { m_RelativeTransform = matrix; }

    private:

        // 頂点配列バッファID（OpenGL固有）
        uint32 m_ID;

        bool          m_HasIndex;
        uint32        m_MaterialIndex = 0;
        uint32        m_VertexCount   = 0;
        uint32        m_IndexCount    = 0;
        VertexBuffer* m_VertexBuffer  = nullptr;
        IndexBuffer*  m_IndexBuffer   = nullptr;
        glm::mat4     m_RelativeTransform;

    private:

        friend class Mesh;
    };


    //===========================================
    // メッシュソースのリスト保持するクラス
    //-------------------------------------------
    // Unity のようにインポート時に、メッシュソース単位で
    // エンティティの階層構造を構築して、エンティティ毎に
    // メッシュをアタッチし、描画を行うのが理想だが、
    // 現状は、1つのエンティティがソース全体を描画する
    //===========================================
    class Mesh : public Asset
    {
    public:

        Mesh();
        Mesh(const Mesh&) = default;

        ~Mesh();

        void Load(const std::filesystem::path& filePath);
        void Unload();
        void AddSource(MeshSource* source);

        // プリミティブ
        void SetPrimitiveType(RHI::PrimitiveType primitiveType) { m_PrimitiveType = primitiveType; }
        RHI::PrimitiveType GetPrimitiveType()                   { return m_PrimitiveType;          }

        // サブメッシュ
        std::vector<MeshSource*>& GetMeshSources() { return m_Meshes;        }
        MeshSource* GetMeshSource(uint32 index)    { return m_Meshes[index]; }

        // テクスチャ
        std::unordered_map<uint32, MeshTexture>& GetTextures() { return m_Textures;        }
        MeshTexture& GetTexture(uint32 index)                  { return m_Textures[index]; }

        uint32 GetMaterialSlotSize() const { return m_MaterialSlotSize; };

    private:

        void        ProcessNode(aiNode* node, const aiScene* scene);
        MeshSource* ProcessMesh(aiMesh* mesh, const aiScene* scene);
        void        LoadMaterialTextures(uint32 materialInddex, aiMaterial* mat, aiTextureType type);

    private:

        std::unordered_map<uint32, MeshTexture> m_Textures;
        std::vector<MeshSource*>                m_Meshes;
        uint32                                  m_MaterialSlotSize;

        RHI::PrimitiveType m_PrimitiveType = RHI::PrimitiveType::Triangle;

        friend class MeshSource;
    };
}
