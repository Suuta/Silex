//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;

layout (location = 0) out vec3 outPosAsUV;

layout(set = 0, binding = 0) uniform Transform
{
    mat4 view;
    mat4 projection;
};

void main()
{
    outPosAsUV = inPos;

    mat4 rotView = mat4(mat3(view));
    vec4 clipPos = projection * rotView * vec4(outPosAsUV, 1.0);

    gl_Position = clipPos;
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 450

layout(location = 0)  in vec3 inPosAsUV;
layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform samplerCube environmentMap;

void main()
{
    // ローカル頂点座標がそのままキューブのテクスチャ座標になる
    vec3 pos      = inPosAsUV;
    vec3 envColor = texture(environmentMap, pos).rgb;

    outColor = vec4(envColor, 1.0);
    outColor.rgb = pow(outColor.rgb, vec3(2.2));
}