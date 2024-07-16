#pragma VERTEX
{
    #version 450

    layout (location = 0) out vec2 outUV;

    void main()
    {
        // 三角形でフルスクリーン描画
        // https://stackoverflow.com/questions/2588875/whats-the-best-way-to-draw-a-fullscreen-quad-in-opengl-3-2


        const vec2 triangle[3] =
        {
            vec2(-1.0,  1.0), // 左上
            vec2(-1.0, -3.0), // 左下
            vec2( 3.0,  1.0), // 右上
        };

        vec4 pos = vec4(triangle[gl_VertexIndex], 0.0, 1.0);
        vec2 uv = (0.5 * pos.xy) + vec2(0.5);

        // uv 反転
        uv.y = 1.0 - uv.y;

        outUV       = uv;
        gl_Position = pos;
    }
}


#pragma FRAGMENT
{
    #version 450

    layout (location = 0) in  vec2 uv;
    layout (location = 0) out vec4 piexl;

    layout (set = 0, binding = 0) uniform sampler2D inputAttachment;

    // ACES Filmic Tone Mapping
    // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
    vec3 ACES(vec3 color)
    {
        const float a = 2.51;
        const float b = 0.03;
        const float c = 2.43;
        const float d = 0.59;
        const float e = 0.14;

        color = clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
        return color;
    }

    void main()
    {
        vec4 color = vec4(1.0);

        color     = texture(inputAttachment, uv); // シーンカラー読み込み
        color.rgb = ACES(color.rgb).rgb;          // 0.0 ~ 1.0 補間

        piexl = color;
    }
}
