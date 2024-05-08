//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450 core

layout (location = 0) in vec3 aPos;       // 頂点: 座標
layout (location = 1) in vec3 aNormal;    // 頂点: ノーマル
layout (location = 2) in vec2 aTexCoords; // 頂点: UV

//layout (location = 3) in vec4 aTransform0; // インスタンス: トランスフォーム0
//layout (location = 4) in vec4 aTransform1; // インスタンス: トランスフォーム1
//layout (location = 5) in vec4 aTransform2; // インスタンス: トランスフォーム2

//layout (location = N) in vec3 aTangent;
//layout (location = N) in vec3 aBitangent;

uniform mat4 model;
uniform int  instanceOffset;


struct InstanceParameter
{
    mat4  transformMatrix;
    mat4  normalMatrix;
    ivec4 pixelID;
};

// インスタンスバッファ
layout (std430, binding = 0) buffer InstanceParameterStorage
{
    InstanceParameter parameter[];
};


void main()
{
    // SSBOを使う場合
    mat4 worldMatrix = parameter[instanceOffset + gl_InstanceID].transformMatrix;
    gl_Position = worldMatrix * vec4(aPos, 1.0);

    // 頂点インスタンスを使った場合
    //gl_Position = aTransform0 * vec4(aPos, 1.0);
    //gl_Position = model * vec4(aPos, 1.0);
}


//===================================================================================
// ジオメトリシェーダ
//===================================================================================
#pragma GEOMETRY
#version 450 core

layout(triangles,      invocations  = 4) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[4];
};

// 頂点シェーダーは頂点に対して1度しか処理できない
// 各レイヤーに対して1回の描画で複数レイヤーに書き込むため
void main()
{
    for (int i = 0; i < 3; ++i)
    {
        // gl_in[i] 三角形の各頂点に対して各カスケードの変換行列を適応
        gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        
        // カスケード Texture2DArray の書き込み先レイヤーを指定
        gl_Layer = gl_InvocationID;
    
        // プリミティブを構築する頂点に設定する
        EmitVertex();
    }
    
    // 次のプリミティブに移行
    EndPrimitive();
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 450 core
    
void main()
{
    // 実際には、フラグメントシェーダーが内部で実行するので行う必要はない
    // gl_FragDepth = gl_FragCoord.z;
}
