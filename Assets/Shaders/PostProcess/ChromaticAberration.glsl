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
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 430 core

in  vec2 TexCoords;
out vec4 PixelColor;

uniform sampler2D screenTexture;

// 強度
const vec2 chromaticAberrationAmount = vec2(0.003, 0.003);


// TODO:
// スクリーン座標に応じてズレを変化させる（本来の色収縮はレンズの表現であり、中心から外に広がるにつれてRGBのズレが大きくなる）
void main()
{

    vec2 Roffset = TexCoords + chromaticAberrationAmount; // * vec2(cos(time), sin(time));
    vec2 Goffset = TexCoords;                             
    vec2 Boffset = TexCoords - chromaticAberrationAmount; // * vec2(cos(time), sin(time));
    
    float r = texture(screenTexture, Roffset).r;
    float g = texture(screenTexture, Goffset).g;
    float b = texture(screenTexture, Boffset).b;
    
    PixelColor = vec4(r, g, b, 1.0);
}
