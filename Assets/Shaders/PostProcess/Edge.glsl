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

uniform sampler2D screenTexture;



// Godot Shader - Edge Detection
// https://godotshaders.com/shader/edge-detection-sobel-filter-and-gaussian-blur/
vec3 convolution(sampler2D tex, vec2 uv, vec2 pixel_size)
{
    vec3 conv = vec3(0.0);

    // Gaussian blur kernel
    float gauss[25] =
    {
        0.00390625, 0.015625, 0.0234375, 0.015625,  0.00390625,
        0.015625,   0.0625,   0.09375,   0.0625,    0.015625,
        0.0234375,  0.09375,  0.140625,  0.09375,   0.0234375,
        0.015625,   0.0625,   0.09375,   0.0625,    0.015625,
        0.00390625, 0.015625, 0.0234375, 0.015625,  0.00390625
    };

    for (int row = 0; row < 5; row++)
    {
        for (int col = 0; col < 5; col++)
        {
            conv += texture(tex, uv + vec2(float(col - 2), float(row - 2)) * pixel_size).rgb * gauss[row * 5 + col];
        }
    }

    return conv;
}

void main()
{
    vec3 color      = vec3(1.0);
    vec2 pixcelSize = 1.0 / textureSize(screenTexture, 0);
    vec3 pixels[9];

    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            vec2 uv = TexCoords + vec2(float(col - 1), float(row - 1)) * pixcelSize;
            pixels[row * 3 + col] = convolution(screenTexture, uv, pixcelSize);
        }
    }

    // Sobel operator
    vec3 gx = pixels[0] * -1.0 + pixels[3] * -2.0 + pixels[6] * -1.0 + pixels[2] * 1.0 + pixels[5] * 2.0 + pixels[8] * 1.0;
    vec3 gy = pixels[0] * -1.0 + pixels[1] * -2.0 + pixels[2] * -1.0 + pixels[6] * 1.0 + pixels[7] * 2.0 + pixels[8] * 1.0;

    color = sqrt(gx * gx + gy * gy);

    PixelColor = vec4(color, 1.0);
}
