
#pragma once

#include "Core/Core.h"
#include "Core/Ref.h"


namespace Silex
{
    class Asset;


    template<class T>
    class AssetSerializer
    {
    public:

        AssetSerializer()          = default;
        virtual ~AssetSerializer() = default;

        static void   Serialize(const Ref<T>& aseet, const std::string& filePath);
        static Ref<T> Deserialize(const std::string& filePath);
    };
}