#pragma once

#include "Core/Macros.h"
#include "Core/CoreType.h"
#include "Core/MemoryPool.h"

#include <unordered_map>
#include <mutex>


//===============================================================
// 最小構成 STL アロケータ
//---------------------------------------------------------------
// メモリーアロケーター内の STL による operator new/delete の
// 再帰呼び出しを防ぐため。アロケーター最小構成
//===============================================================
template <typename T>
class DefaultAllocator
{
public:

    using value_type = T;

    DefaultAllocator() = default;

    template <typename U>
    DefaultAllocator(const DefaultAllocator<U>&) noexcept {}

    T* allocate(std::size_t n)
    {
        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
            return p;
        else
            throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t) noexcept
    {
        std::free(p);
        p = nullptr;
    }
};




namespace Silex
{
    //===============================================================
    // プールロケータ
    //===============================================================
    class PoolAllocator
    {
    public:

        static void Init();
        static void Shutdown();

        static void* Allocate(uint64 sizeByte);
        static void  Deallocate(void* pointer);

        static const std::array<MemoryPoolStatus, 6>& GetStatus()
        {
            return pool.GetStatus();
        }

    private:

        static inline MemoryPool pool;
    };


    //===============================================================
    // デバッグ用メモリトラッカー
    //===============================================================
    class MemoryTracker
    {
    public:

        static void Init();
        static void Shutdown();

        static void RecordAllocate(void* allocatedPtr, size_t size, const char* typeName, const char* file, size_t line);
        static void RecordDeallocate(void* ptr);
        static void DumpMemoryStats();

    private:

        // メモリデータ
        struct AllocationElement
        {
            size_t      size = {};
            const char* desc = {};
            const char* file = {};
            size_t      line = {};
        };

        // メモリデータコンテナ
        struct AllocationMap
        {
            using Type              = void*;
            using AllocatorType     = DefaultAllocator<std::pair<const Type, AllocationElement>>;
            using AllocationHashMap = std::unordered_map<Type, AllocationElement, std::hash<Type>, std::equal_to<>, AllocatorType>;

            AllocationHashMap memoryMap;
            std::mutex        mutex;
            uint64            totalAllocationSize;
        };


        static inline AllocationMap* allocationData = nullptr;
    };


    namespace Memory
    {
        //=======================================
        // コンストラクタ / デストラクタ
        //=======================================
        template<typename T, typename ... Args>
        T* Construct(void* ptr, Args&& ... args)
        {
            return std::construct_at(static_cast<T*>(ptr), Traits::Forward<Args>(args)...);
        }

        template<typename T>
        void Destruct(T* ptr)
        {
            std::destroy_at(ptr);
        }

        //=======================================
        // ヒープ確保
        //=======================================
        SL_FORCEINLINE void* Malloc(uint64 size)
        {
            return std::malloc(size);
        }

        SL_FORCEINLINE void Free(void* ptr)
        {
            std::free(ptr);
        }

        //=======================================
        // プールから確保
        //=======================================
        template<typename T, typename... Args>
        T* Allocate(Args&& ... args)
        {
            static_assert(sizeof(T) <= 1024);

            void* ptr = PoolAllocator::Allocate(sizeof(T));
            return Memory::Construct<T>(ptr, Traits::Forward<Args>(args)...);
        }

        template<typename T>
        void Deallocate(T* ptr)
        {
            Memory::Destruct(ptr);
            PoolAllocator::Deallocate((void*)ptr);
        }


#if 0
        //=======================================
        // カスタム new / delete
        //=======================================
        template<typename T, typename ... Args>
        T* New(Args&& ... args)
        {
            void* ptr = Memory::Malloc(sizeof(T));
            return new(ptr, SLEmpty()) T(Traits::Forward<Args>(args)...);
        }

        template<typename T, typename ... Args>
        T* NewArray(size_t arraySize, Args&& ... args)
        {
            T* ptrHead = (T*)Memory::Malloc(sizeof(T) * arraySize);
            T* cursor  = ptrHead;

            for (uint32 i = 0; i < arraySize; i++)
            {
                new(cursor++, SLEmpty()) T(Traits::Forward<Args>(args)...);
            }

            return ptrHead;
        }

        template<typename T>
        void Delete(T* ptr)
        {
            if (ptr)
            {
                std::destroy_at(ptr);
                Memory::Free(ptr);
            }
        }

        template<typename T>
        void DeleteArray(T* ptr, size_t arraySize)
        {
            if (ptr)
            {
                std::destroy(&ptr[0], &ptr[arraySize - 1]);
                Memory::Free(ptr);
            }
        }
#endif
    }
}
