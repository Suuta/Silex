
#pragma once

#include "Core/Hash.h"


namespace Silex
{
#define QUERY_TYPE_INFO(T) TypeInfo{ sizeof(T), alignof(T), Hash::StaticFNV(__FUNCSIG__) } 


    struct TypeInfo
    {
        template<typename T>
        static consteval TypeInfo Query()
        {
            return QUERY_TYPE_INFO(T);
        }

        uint64 typeSize;
        uint64 alignSize;
        uint64 hashID;
    };

}