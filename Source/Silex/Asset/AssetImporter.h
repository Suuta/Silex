
#pragma once
#include "Core/CoreType.h"
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
