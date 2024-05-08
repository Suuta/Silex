//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 430 core

layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 LocalPos;


void main()
{
    LocalPos = aPos;

    mat4 rotView = mat4(mat3(view));
    vec4 clipPos = projection * rotView * vec4(LocalPos, 1.0);

    gl_Position = clipPos;
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 430 core

in vec3 LocalPos;

layout(location = 0) out vec4 PixelColor;

uniform samplerCube environmentMap;

void main()
{
    // ローカル頂点座標がそのままキューブのテクスチャ座標になる
    vec3 pos      = LocalPos;
    vec3 envColor = texture(environmentMap, pos).rgb;

    PixelColor = vec4(envColor, 1.0);
}