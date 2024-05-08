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
uniform float     threshold = 1.0;


// ブルームにしきい値を適応
//https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/
vec3 Prefilter (vec3 color, float threshold)
{
    float brightness   = max(color.r, max(color.g, color.b));
    float contribution = max(0.0, brightness - threshold);
    contribution /= max(brightness, 0.0001);

    return color * contribution;
}

void main()
{
    vec3  color        = texture(srcTexture,  TexCoords).rgb;
    float brightness   = max(color.r, max(color.g, color.b));
    float contribution = max(0.0, brightness - threshold);
    contribution /= max(brightness, 0.0001);

    color *= contribution;
    PixelColor = vec4(color, 1.0);
}
