
#include "PCH.h"
#include "Rendering/MeshFactory.h"
#include "Rendering/Renderer.h"
#include "Rendering/Mesh.h"


namespace Silex
{
    Mesh* MeshFactory::Quad()
    {
        float quadVertices[] =
        {
            -1.0f,  1.0f,  0.0f,   0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f,   0.0f, 0.0f,
             1.0f,  1.0f,  0.0f,   1.0f, 1.0f,
             1.0f, -1.0f,  0.0f,   1.0f, 0.0f,
        };

        VertexBufferLayout vertex;
        vertex.Add(0, "Position", RHI::ShaderDataType::Float3);
        vertex.Add(1, "Texcoord", RHI::ShaderDataType::Float2);

        VertexBufferLayout instance;
        //instance.Add(2, "Transform_0", RHI::ShaderDataType::Float4);
        //instance.Add(3, "Transform_1", RHI::ShaderDataType::Float4);
        //instance.Add(4, "Transform_2", RHI::ShaderDataType::Float4);
        //instance.Add(5, "Transform_3", RHI::ShaderDataType::Float4);

        MeshSource* source = Memory::Allocate<MeshSource>(&quadVertices, sizeof(quadVertices), nullptr, 0, vertex, instance);
        Mesh*       mesh   = Memory::Allocate<Mesh>();
        mesh->AddSource(source);
        mesh->SetAssetType(AssetType::Mesh);

        return mesh;
    }

    Mesh* MeshFactory::Cube()
    {
        // 頂点
        float vertices[] =
        {
            -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,
             0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,

            -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f,
             0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,

             0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f,

            -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
        };

        // インデックス
        uint32 indices[] =
        {
            // 前面
            0, 1, 2,
            2, 1, 3,

            // 背面
            4, 6, 5,
            5, 6, 7,

            // 上面
            8, 10,  9,
            9, 10, 11,

            // 下面
            12, 13, 14,
            14, 13, 15,

            // 右面
            17, 18, 16,
            19, 18, 17,

            // 左面
            21, 20, 22,
            21, 22, 23,
        };

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

        MeshSource* source = Memory::Allocate<MeshSource>(vertices, sizeof(vertices), indices, sizeof(indices), vertex, instance);
        Mesh*       mesh   = Memory::Allocate<Mesh>();
        mesh->AddSource(source);
        mesh->SetAssetType(AssetType::Mesh);

        return mesh;
    }

    Mesh* MeshFactory::Sphere()
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uv;
        std::vector<uint32>    indices;

        const uint32 X_SEGMENTS = 32;
        const uint32 Y_SEGMENTS = 32;
        const float PI = 3.14159265359f;

        for (uint32 x = 0; x <= X_SEGMENTS; ++x)
        {
            for (uint32 y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI) * 0.5f;
                float yPos = std::cos(ySegment * PI) * 0.5f;
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI) * 0.5f;

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (uint32 y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow)
            {
                for (uint32 x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int32 x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }

        std::vector<float> data;
        for (uint32 i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);

            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }

        
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

        MeshSource* source = Memory::Allocate<MeshSource>(data.data(), sizeof(float) * data.size(), indices.data(), sizeof(uint32) * indices.size(), vertex, instance);
        Mesh*       mesh   = Memory::Allocate<Mesh>();
        mesh->AddSource(source);
        mesh->SetAssetType(AssetType::Mesh);

        return mesh;
    }
}
