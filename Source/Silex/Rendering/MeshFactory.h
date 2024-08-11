
#pragma once

#include "Rendering/Mesh.h"

namespace Silex
{
    struct MeshFactory
    {
        static Mesh* Cube();
        static Mesh* Sphere();
        static Mesh* Monkey();
        static Mesh* Sponza();
    };
}