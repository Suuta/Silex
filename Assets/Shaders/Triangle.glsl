#pragma VERTEX
{
    #version 450
    #include "vertex.glsl"

    VertexOutput Vertex(VertexInput vin)
    {
        VertexOutput vout;

        mat4 ts = (projection * view * world);
        vec4 pos = ts * vec4(vin.pos, 1.0);

        vout.pos      = pos;
        vout.normal   = vin.normal;
        vout.texcoord = vin.texcoord;

        return vout;
    }
}


#pragma FRAGMENT
{
    #version 450
    #include "fragment.glsl"

    FragmentOutput Fragment(VertexOutput vout)
    {
        vec3 n = (vout.normal * 0.5) + vec3(0.5);
        vec4 c = texture(mainTexture, vout.texcoord);

        FragmentOutput fout;
        fout.texcoord = vout.texcoord;
        fout.normal   = n;
        fout.color    = c;

        return fout;
    }
}
