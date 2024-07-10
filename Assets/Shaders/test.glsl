#pragma VERTEX
{
    #version 450
    //#include "common.glsl"

    // IN
    // OUT


    void main()
    {
        // 反時計回り
        vec2 verticies[3] =
        { 
            vec2( 0.0,  1.0),
            vec2(-1.0, -1.0),
            vec2( 1.0, -1.0),
        };

        // 頂点座標
        gl_Position = vec4(verticies[gl_VertexIndex], 1.0, 1.0);

        // Y軸反転
        gl_Position.y = -gl_Position.y;
    }
}



#pragma FRAGMENT
{
    #version 450
    //#include "common.glsl"

    // IN
    // OUT
    layout (location = 0) out vec4 outFragColor;

    void main()
    {
        // ピクセルカラー
        outFragColor = vec4(1.0, 0.0, 1.0 ,1.0);
    }
}
