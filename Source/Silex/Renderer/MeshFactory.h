
#pragma once

#include "Renderer/Mesh.h"
#include "OpenGL/GLMeshBuffer.h"


namespace Silex
{
    struct MeshFactory
    {
        static Mesh* Quad();
        static Mesh* Cube();
        static Mesh* Sphere();
    };
}