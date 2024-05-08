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
uniform float     exposure        = 1.0;
uniform float     gammaCorrection = 2.2;
uniform float     toneLimit       = 1.0;


void main()
{
    vec3 color = texture(srcTexture,  TexCoords).rgb;

    // ガンマ
    color = pow(color, vec3(1.0 / gammaCorrection));

    // 露出 ?
    //color = vec3(1.0) - exp(-color * exposure);
    color *= exposure;

    // トーンマップ
    color = color / (color + 1.0);

    PixelColor = vec4(color, 1.0);
}
