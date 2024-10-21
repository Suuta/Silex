
#pragma once

#include "Core/Core.h"
#include "Core/Ref.h"


namespace Silex
{
    //============================
    // 実体は "AssetCreator.cpp"
    //============================
    
    class AssetCreator
    {
    public:

        template<class T>
        static Ref<T> Create(const std::filesystem::path& directory);
    };
}
