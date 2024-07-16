#pragma VERTEX
{
    #version 450
    #include "vertex.glsl"

    VertexOutput Vertex(VertexInput vin)
    {
        VertexOutput vout;

        mat4 ts = (projection * view * world);
        vec4 pos = ts * vec4(vin.pos, 1.0);

        vout.pos    = pos;
        vout.normal = vin.normal;
        vout.uv     = vin.uv;
        vout.color  = vin.color;

        return vout;
    }
}


#pragma FRAGMENT
{
    #version 450
    #include "fragment.glsl"

    FragmentOutput Fragment(VertexOutput vout)
    {
        FragmentOutput fout;
        fout.color = vout.color;

        return fout;
    }
}
