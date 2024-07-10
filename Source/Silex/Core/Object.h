#pragma once

#include "Core/OS.h"
#include "Core/Logger.h"
#include "Core/TypeInfo.h"
#include <atomic>


namespace Silex
{
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

    //============================================
    // 抽象化汎用ハンドル
    //============================================
    class Handle : public Class
    {
        SL_CLASS(Handle, Class)

        uint64 pointer = 0;

        Handle() : pointer(uint64(this)) {}
        Handle(void* ptr) : pointer(uint64(ptr)) {}

        virtual ~Handle() {};
    };
}
