#pragma once

#include "Core/OS.h"
#include "Core/Logger.h"
#include "Core/TypeInfo.h"
#include <atomic>


namespace Silex
{
    //========================================
    // クラス情報インターフェース
    //========================================
    class Class
    {
    public:

        Class()          {};
        virtual ~Class() {};

        virtual const char* GetRuntimeClassName() const = 0;
        virtual uint64      GetRuntimeHashID()    const = 0;

    public:

        // 自身と'T'が一致しているかどうかを調べる
        template<class T>
        bool IsClassOf()
        {
            static_assert(Traits::IsBaseOf<Class, T>(), "T は Class を継承する必要があります");
            return T::staticHashID == GetRuntimeHashID();
        }

        // T と T2 を比較する
        template<class T, class T2>
        static bool IsSameClassOf(T* a, T2* b)
        {
            static_assert(Traits::IsBaseOf<Class, T>() && Traits::IsBaseOf<Class, T2>(), "T / T2 は Class を継承する必要があります");
            return a->GetRuntimeHashID() == b->GetRuntimeHashID();
        }
    };


    //===========================================
    // Class クラスを継承した全てのデータ型の情報を保持
    //===========================================
    class GlobalClassDataBase
    {
    public:

        template<typename T>
        static uint64 Register(const char* className)
        {
            auto type = TypeInfo::Query<T>();

            SL_ASSERT(!classInfoMap.contains(className));
            classInfoMap.emplace(className, type);

            return type.hashID;
        }

        static void DumpClassInfoList()
        {
            for (const auto& [name, info] : classInfoMap)
            {
                SL_LOG_DEBUG("{:32}: {:4}, {}", name, info.typeSize, info.hashID);
            }
        }

    private:

        static inline std::unordered_map<const char*, TypeInfo> classInfoMap;
    };


    //========================================
    // 参照カウントオブジェクト
    //========================================
    class Object : public Class
    {
        SL_CLASS(Object, Class)

    public:

        Object()              {};
        Object(const Object&) {};

    public:

        uint32 GetRefCount() const { return refCount; }

    private:

        //=================================================================
        // メタデータとしての参照カウントが内部で変更されているだけということを示しており、
        // Object データ自体への操作は 不変(const) であるかのようにふるまう。
        // また、const / mutable がデータ競合に関する属性を持つわけではない
        //=================================================================
        void IncRefCount() const { ++refCount; }
        void DecRefCount() const { --refCount; }

        mutable std::atomic<uint32> refCount = 0;

    private:

        // 参照カウント操作は参照カウントポインタからのみ操作可能にする
        template<class T>
        friend class Shared;
    };
}
