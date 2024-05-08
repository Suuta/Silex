
#pragma once

#include "Core/CoreType.h"
#include "Core/SharedPointer.h"


namespace Silex
{
    class Asset;


    template<class T>
    class AssetSerializer
    {
    public:

        AssetSerializer()          = default;
        virtual ~AssetSerializer() = default;

        static void      Serialize(const Shared<T>& aseet, const std::string& filePath);
        static Shared<T> Deserialize(const std::string& filePath);
    };
}