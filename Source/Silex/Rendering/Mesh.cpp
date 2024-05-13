
#include "PCH.h"

#include "Rendering/Mesh.h"
#include "Rendering/Texture.h"
#include "Rendering/OpenGL/OpenGLCore.h"
#include "Asset/TextureReader.h"
#include "Editor/SplashImage.h"


namespace Silex
{
    namespace Internal
    {
        // a,b,c,d 行(Row) 1,2,3,4 列(Column)
        static inline glm::mat4 aiMatrixToGLMMatrix(const aiMatrix4x4& from)
        {
            glm::mat4 m;

            m[0][0] = from.a1; m[1][0] = from.a2; m[2][0] = from.a3; m[3][0] = from.a4;
            m[0][1] = from.b1; m[1][1] = from.b2; m[2][1] = from.b3; m[3][1] = from.b4;
            m[0][2] = from.c1; m[1][2] = from.c2; m[2][2] = from.c3; m[3][2] = from.c4;
            m[0][3] = from.d1; m[1][3] = from.d2; m[2][3] = from.d3; m[3][3] = from.d4;

            return m;
        }
    }

    //===========================================
    // 頂点データから生成
    //===========================================
    MeshSource::MeshSource(void* rawVertexData, uint64 vertexByteSize, void* rawIndexData, uint64 indexByteSize, const VertexBufferLayout& vertexAttribute, const VertexBufferLayout& instanceAttribute)
        : m_RelativeTransform(glm::mat4(1.0f))
    {
        glGenVertexArrays(1, &m_ID);

        if (rawVertexData)
        {
            m_VertexBuffer = VertexBuffer::Create(rawVertexData, vertexByteSize);
            m_HasIndex     = false;
            m_VertexCount  = vertexByteSize / vertexAttribute.Stride;
        }

        if (rawIndexData)
        {
            m_IndexBuffer = IndexBuffer::Create(rawIndexData, indexByteSize);
            m_HasIndex    = true;
            m_IndexCount  = indexByteSize / sizeof(uint32);
        }

        SetVertexLayout(vertexAttribute, instanceAttribute);
    }

    //===========================================
    // モデルデータから生成
    //===========================================
    MeshSource::MeshSource(std::vector<Vertex>& vertices, std::vector<uint32>& indices, uint32 materialIndex)
        : m_HasIndex(indices.size() != 0)
        , m_MaterialIndex(materialIndex)
    {
        glGenVertexArrays(1, &m_ID);

        m_VertexBuffer = VertexBuffer::Create(vertices.data(), sizeof(Vertex) * vertices.size());
        m_IndexBuffer  = IndexBuffer::Create(indices.data(), sizeof(uint32) * indices.size());

        m_VertexCount = vertices.size();
        m_IndexCount  = indices.size();


        // 頂点属性
        VertexBufferLayout vertex;
        vertex.Add(0, "Position", RHI::ShaderDataType::Float3);
        vertex.Add(1, "Normal",   RHI::ShaderDataType::Float3);
        vertex.Add(2, "Texcoord", RHI::ShaderDataType::Float2);
        //layout.Add("Tangent",   RHI::ShaderDataType::Float3);
        //layout.Add("Bitangent", RHI::ShaderDataType::Float3);

        // インスタンス属性
        VertexBufferLayout instance;
        //instance.Add(3, "Transform_0", RHI::ShaderDataType::Float4);
        //instance.Add(4, "Transform_1", RHI::ShaderDataType::Float4);
        //instance.Add(5, "Transform_2", RHI::ShaderDataType::Float4);
        //instance.Add(6, "Transform_3", RHI::ShaderDataType::Float4);

        SetVertexLayout(vertex, instance);
    }

    MeshSource::~MeshSource()
    {
        glDeleteVertexArrays(1, &m_ID);

        if (m_VertexBuffer) Memory::Deallocate(m_VertexBuffer);
        if (m_IndexBuffer)  Memory::Deallocate(m_IndexBuffer);
    }

    //===========================================
    // 頂点属性の設定
    //===========================================
    void MeshSource::SetVertexLayout(const VertexBufferLayout& vertexAttribute, const VertexBufferLayout& instanceAttribute)
    {
        Bind();

        if (m_VertexBuffer) m_VertexBuffer->Bind();
        if (m_IndexBuffer ) m_IndexBuffer->Bind();

        // 頂点属性
        for (auto& element : vertexAttribute.Elements)
        {
            uint32 componentSize  = RHI::ShaderDataTypeComponentSize(element.Type);
            uint32 stride         = vertexAttribute.Stride;
            uint32 offsetFromHead = element.Offset;

            glEnableVertexAttribArray(element.LayoutIndex);
            glVertexAttribPointer(element.LayoutIndex, componentSize, GL_FLOAT, GL_FALSE, stride, (void*)offsetFromHead);
        }

        // インスタンス属性
        for (auto& element : instanceAttribute.Elements)
        {
            uint32 componentSize = RHI::ShaderDataTypeComponentSize(element.Type);
            uint32 stride         = instanceAttribute.Stride;
            uint32 offsetFromHead = element.Offset;
        
            glEnableVertexAttribArray(element.LayoutIndex);
            glVertexAttribPointer(element.LayoutIndex, componentSize, GL_FLOAT, GL_FALSE, stride, (void*)offsetFromHead);
        
            glVertexAttribDivisor(element.LayoutIndex, 1);
        }

        Unbind();
    }

    void MeshSource::Bind() const
    {
        glBindVertexArray(m_ID);
    }

    void MeshSource::Unbind() const
    {
        glBindVertexArray(0);
    }

    Mesh::Mesh()
    {
        SetAssetType(AssetType::Mesh);

        //m_MaterialTable.resize(1);
        m_MaterialSlotSize = 1;
    }

    Mesh::~Mesh()
    {
        Unload();
    }

    void Mesh::Load(const std::filesystem::path& filePath)
    {
        m_FilePath = filePath.string();

        uint32 flags =
            aiProcess_OptimizeMeshes   |
            aiProcess_Triangulate      |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs          |
            aiProcess_GenUVCoords      |
            aiProcess_CalcTangentSpace;

        // メッシュファイルを読み込み
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(m_FilePath, flags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            SL_LOG_ERROR("Assimp Error: {}", importer.GetErrorString());
        }

        // 各メッシュ情報を処理
        ProcessNode(scene->mRootNode, scene);

        // マテリアルテーブル構成
        //m_MaterialTable.resize(scene->mNumMaterials);
        m_MaterialSlotSize = scene->mNumMaterials;
    }

    void Mesh::Unload()
    {
        for (auto mesh : m_Meshes)
        {
            Memory::Deallocate(mesh);
        }
    }

    void Mesh::AddSource(MeshSource* source)
    {
        m_Meshes.push_back(source);
    }

    void Mesh::ProcessNode(aiNode* node, const aiScene* scene)
    {
        for (uint32 i = 0; i < node->mNumMeshes; i++)
        {
            uint32 subMeshIndex = node->mMeshes[i];
            aiMesh* mesh = scene->mMeshes[subMeshIndex];

            MeshSource* ms = ProcessMesh(mesh, scene);
            ms->m_RelativeTransform = Internal::aiMatrixToGLMMatrix(node->mTransformation);

            m_Meshes.emplace_back(ms);
        }

        for (uint32 i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene);
        }
    }
    
    MeshSource* Mesh::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32> indices;

        //==============================================
        // 頂点
        //==============================================
        for (uint32 i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;

            // 座標
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // ノーマル
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;

                //vector.x = mesh->mTangents[i].x;
                //vector.y = mesh->mTangents[i].y;
                //vector.z = mesh->mTangents[i].z;
                //vertex.Tangent = vector;

                //vector.x = mesh->mBitangents[i].x;
                //vector.y = mesh->mBitangents[i].y;
                //vector.z = mesh->mBitangents[i].z;
                //vertex.Bitangent = vector;
            }

            // テクスチャ座標
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;

                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        //==============================================
        // インデックス
        //==============================================
        for (uint32 i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32 j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        //==============================================
        // テクスチャ
        //==============================================
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        LoadMaterialTextures(mesh->mMaterialIndex, material, aiTextureType_DIFFUSE);           // ディフューズ
      //LoadMaterialTextures(mesh->mMaterialIndex, material, aiTextureType_NORMALS);           // ノーマル
      //LoadMaterialTextures(mesh->mMaterialIndex, material, aiTextureType_AMBIENT_OCCLUSION); // AO

        //==============================================
        // メッシュソース生成
        //==============================================
        return Memory::Allocate<MeshSource>(vertices, indices, mesh->mMaterialIndex);
    }
    
    void Mesh::LoadMaterialTextures(uint32 materialInddex, aiMaterial* material, aiTextureType type)
    {
        for (uint32 i = 0; i < material->GetTextureCount(type); i++)
        {
            aiString str;
            material->GetTexture(type, i, &str);
            std::string aspath = str.C_Str();
            std::replace(aspath.begin(), aspath.end(), '\\', '/');

            // テクスチャファイルのディレクトリに変換
            std::filesystem::path modelFilePath = m_FilePath;
            std::string parebtPath = modelFilePath.parent_path().string();
            std::string path       = parebtPath + '/' + aspath;

            MeshTexture& tex = m_Textures[materialInddex];
            tex.Path   = path;
            tex.Albedo = 0;
        }
    }
}
