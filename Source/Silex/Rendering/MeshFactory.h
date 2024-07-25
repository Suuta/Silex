
#pragma once

#include "Rendering/Mesh.h"
#include "Rendering/OpenGL/GLMeshBuffer.h"


namespace Silex
{
    struct MeshFactory
    {
        //static Mesh* Quad();
        static Mesh* Cube();
        static Mesh* Sphere();
    };
}