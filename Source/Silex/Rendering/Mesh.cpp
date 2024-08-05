
#include "PCH.h"

#include "Rendering/Mesh.h"
#include "Rendering/Texture.h"
#include "Rendering/RenderingDevice.h"
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
    MeshSource::MeshSource(std::vector<Vertex>& vertices, std::vector<uint32>& indices, uint32 materialIndex)
        : vertexCount(vertices.size())
        , indexCount(indices.size())
        , hasIndex(!indices.empty())
        , materialIndex(materialIndex)
    {
        vertexBuffer = RenderingDevice::Get()->CreateVertexBuffer(vertices.data(), sizeof(Vertex) * vertexCount);
        indexBuffer  = RenderingDevice::Get()->CreateIndexBuffer(indices.data(), sizeof(uint32) * indexCount);
    }

    MeshSource::MeshSource(uint64 numVertex, Vertex* vertices, uint64 numIndex, uint32* indices, uint32 materialIndex)
        : vertexCount(numVertex)
        , indexCount(numIndex)
        , hasIndex(indices != nullptr || numIndex == 0)
        , materialIndex(materialIndex)
    {
        vertexBuffer = RenderingDevice::Get()->CreateVertexBuffer(vertices, sizeof(Vertex) * vertexCount);
        indexBuffer  = RenderingDevice::Get()->CreateIndexBuffer(indices, sizeof(uint32) * indexCount);
    }

    MeshSource::~MeshSource()
    {
        if (vertexBuffer) RenderingDevice::Get()->DestroyBuffer(vertexBuffer);
        if (indexBuffer) RenderingDevice::Get()->DestroyBuffer(indexBuffer);
    }

    void MeshSource::Bind() const
    {
        // glBindVertexArray(m_ID);
    }

    void MeshSource::Unbind() const
    {
        // glBindVertexArray(0);
    }

    Mesh::Mesh()
    {
        SetAssetType(AssetType::Mesh);
        numMaterialSlot = 1;
    }

    Mesh::~Mesh()
    {
        Unload();
    }

    void Mesh::Load(const std::filesystem::path& filePath)
    {
        m_FilePath = filePath.string();

        uint32 flags = 0;
        flags |= aiProcess_OptimizeMeshes;
        flags |= aiProcess_Triangulate;
        flags |= aiProcess_GenSmoothNormals; // NOTE: すでに法線情報が存在する場合は無視される
        flags |= aiProcess_FlipUVs;
        flags |= aiProcess_GenUVCoords;
        flags |= aiProcess_CalcTangentSpace;

        // メッシュファイルを読み込み
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(m_FilePath, flags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            SL_LOG_ERROR("Assimp Error: {}", importer.GetErrorString());
        }

        // 各メッシュ情報を読み込み
        ProcessNode(scene->mRootNode, scene);

        // マテリアル数
        numMaterialSlot = scene->mNumMaterials;
    }

    // 明示的に呼び出したい場合に（デストラクタで呼び出されるため、不要）
    void Mesh::Unload()
    {
        for (auto source : subMeshes)
        {
            sldelete(source);
        }
    }

    void Mesh::AddSource(MeshSource* source)
    {
        subMeshes.push_back(source);
    }

    void Mesh::ProcessNode(aiNode* node, const aiScene* scene)
    {
        for (uint32 i = 0; i < node->mNumMeshes; i++)
        {
            uint32 subMeshIndex = node->mMeshes[i];
            aiMesh* mesh = scene->mMeshes[subMeshIndex];

            MeshSource* ms = ProcessMesh(mesh, scene);
            ms->relativeTransform = Internal::aiMatrixToGLMMatrix(node->mTransformation);

            subMeshes.emplace_back(ms);
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
            if (mesh->HasPositions())
            {
                vector.x = mesh->mVertices[i].x;
                vector.y = mesh->mVertices[i].y;
                vector.z = mesh->mVertices[i].z;
                vertex.Position = vector;
            }

            // ノーマル
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            // 接線
            if (mesh->HasTangentsAndBitangents())
            {
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;

                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
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
      //LoadMaterialTextures(mesh->mMaterialIndex, material, aiTextureType_DIFFUSE_ROUGHNESS); // 
      //LoadMaterialTextures(mesh->mMaterialIndex, material, aiTextureType_NORMALS);           // ノーマル
      //LoadMaterialTextures(mesh->mMaterialIndex, material, aiTextureType_AMBIENT_OCCLUSION); // AO
      //LoadMaterialTextures(mesh->mMaterialIndex, material, aiTextureType_SPECULAR);          // スペキュラ
      //LoadMaterialTextures(mesh->mMaterialIndex, material, aiTextureType_EMISSIVE);          // 
      //LoadMaterialTextures(mesh->mMaterialIndex, material, aiTextureType_EMISSION_COLOR);    // 

        //==============================================
        // メッシュソース生成
        //==============================================
        return slnew(MeshSource, vertices, indices, mesh->mMaterialIndex);
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

            MeshTexture& tex = textures[materialInddex];
            tex.Path   = path;
            tex.Albedo = 0;
        }
    }
}
