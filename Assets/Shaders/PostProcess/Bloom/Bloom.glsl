//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords   = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 430 core

in  vec2 TexCoords;
out vec4 PixelColor;

uniform sampler2D srcTexture;
uniform sampler2D bloomBlur;
uniform float     intencity;

void main()
{
    vec3 color = vec3(1.0);

    // ブルームテクスチャサンプリング
    vec3 hdrColor   = texture(srcTexture, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur,  TexCoords).rgb;

    // 0.0 ~ 1.0 で線形補間
    color = mix(hdrColor, bloomColor, intencity);

    // ブルームが適応されなかったピクセルに対しても元の色が適応されるようにする
    color = max(color, hdrColor);

    PixelColor = vec4(color, 1.0);
}
