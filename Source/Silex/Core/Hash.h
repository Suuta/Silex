#pragma once

#include "Core/CoreType.h"


namespace Silex
{
    template<typename>
    struct FNV1A_Traits;

    template<>
    struct FNV1A_Traits<uint32>
    {
        using type = uint32;
        static constexpr uint32 offset = 2166136261u;
        static constexpr uint32 prime  = 16777619u;
    };

    template<>
    struct FNV1A_Traits<uint64>
    {
        using Type = uint64;
        static constexpr uint64 offset = 14695981039346656037ull;
        static constexpr uint64 prime  = 1099511628211ull;
    };

    struct Hash
    {
        // 長さが不確定な文字配列のハッシュ
        template<typename T = uint64>
        static T FNV(const char* str, size_t length)
        {
            T value = FNV1A_Traits<T>::offset;
            for (uint64 i = 0; i < length; ++i)
            {
                value ^= *str++;
                value *= FNV1A_Traits<T>::prime;
            }

            return value;
        }

        // コンパイル時定数な文字列リテラルのハッシュ
        template<typename T = uint64>
        static consteval T StaticFNV(const char* str)
        {
            T value = FNV1A_Traits<T>::offset;
            while (*str != 0)
            {
                value ^= *str++;
                value *= FNV1A_Traits<T>::prime;
            }

            return value;
        }
    };
}