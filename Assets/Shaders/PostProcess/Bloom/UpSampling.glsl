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

uniform sampler2D srcTexture;
uniform float     filterRadius;

in  vec2 TexCoords;
out vec4 PixelColor;


// 参照
// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

void main()
{
    float x = filterRadius;
    float y = filterRadius;

    // a - b - c
    // d - e - f
    // g - h - i
    vec3 a = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
    vec3 b = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y + y)).rgb;
    vec3 c = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;

    vec3 d = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y    )).rgb;
    vec3 e = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y    )).rgb;
    vec3 f = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y    )).rgb;

    vec3 g = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
    vec3 h = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y - y)).rgb;
    vec3 i = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |

    vec3 color = vec3(1.0);

    color =  e * 4.0;
    color += (b + d + f + h) * 2.0;
    color += (a + c + g + i);
    color *= 1.0 / 16.0;

    PixelColor = vec4(color, 1.0);
}