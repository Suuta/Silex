
#pragma once
#include "Core/Core.h"
#include "Core/SharedPointer.h"


namespace Silex
{
    class AssetImporter
    {
    public:

        template<class T>
        static Shared<T> Import(const std::string& filePath);
    };
}
