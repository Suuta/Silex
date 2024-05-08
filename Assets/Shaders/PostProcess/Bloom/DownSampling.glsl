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
uniform vec2      srcResolution;

// 参照
// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

// |a| |b| |c|
// | |j| |k| |
// |d| |e| |f|
// | |l| |m| |
// |g| |h| |i|

void main()
{
    vec2 srcTexelSize = 1.0 / srcResolution;
    vec3 color        = vec3(1.0);
    float x           = srcTexelSize.x;
    float y           = srcTexelSize.y;

    vec3 a = texture(srcTexture, vec2(TexCoords.x - 2 * x, TexCoords.y + 2 * y)).rgb;
    vec3 b = texture(srcTexture, vec2(TexCoords.x,         TexCoords.y + 2 * y)).rgb;
    vec3 c = texture(srcTexture, vec2(TexCoords.x + 2 * x, TexCoords.y + 2 * y)).rgb;

    vec3 d = texture(srcTexture, vec2(TexCoords.x - 2 * x, TexCoords.y        )).rgb;
    vec3 e = texture(srcTexture, vec2(TexCoords.x,         TexCoords.y        )).rgb;
    vec3 f = texture(srcTexture, vec2(TexCoords.x + 2 * x, TexCoords.y        )).rgb;

    vec3 g = texture(srcTexture, vec2(TexCoords.x - 2 * x, TexCoords.y - 2 * y)).rgb;
    vec3 h = texture(srcTexture, vec2(TexCoords.x,         TexCoords.y - 2 * y)).rgb;
    vec3 i = texture(srcTexture, vec2(TexCoords.x + 2 * x, TexCoords.y - 2 * y)).rgb;

    vec3 j = texture(srcTexture, vec2(TexCoords.x - x,     TexCoords.y + y    )).rgb;
    vec3 k = texture(srcTexture, vec2(TexCoords.x + x,     TexCoords.y + y    )).rgb;
    vec3 l = texture(srcTexture, vec2(TexCoords.x - x,     TexCoords.y - y    )).rgb;
    vec3 m = texture(srcTexture, vec2(TexCoords.x + x,     TexCoords.y - y    )).rgb;

    color =   e * 0.125;
    color += (a + c + g + i) * 0.03125;
    color += (b + d + f + h) * 0.0625;
    color += (j + k + l + m) * 0.125;

    float EPSILON = 0.0001;
    color = max(color, EPSILON);

    PixelColor = vec4(color, 1.0);
}