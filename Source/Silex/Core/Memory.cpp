
#include "PCH.h"

#include "Core/Memory.h"
#include "Core/Logger.h"

#include <filesystem>


namespace Silex
{
    static bool s_Initialized = false;


    void PoolAllocator::Init()
    {
        pool.Init();
    }

    void PoolAllocator::Shutdown()
    {
        pool.Shutdown();
    }

    void* PoolAllocator::Allocate(uint64 sizeByte)
    {
        return pool.Allocate(sizeByte);
    }

    void PoolAllocator::Deallocate(void* pointer)
    {
        pool.Deallocate(pointer);
    }




    void MemoryTracker::Init()
    {
        if (s_Initialized)
            return;

        allocationData = Memory::Construct<AllocationMap>(Memory::Malloc(sizeof(AllocationMap)));;
        allocationData->totalAllocationSize = 0;

        s_Initialized = true;
    }

    void MemoryTracker::Shutdown()
    {
        s_Initialized = false;

        Memory::Destruct(allocationData);
        Memory::Free(allocationData);

        allocationData = nullptr;
    }

    void MemoryTracker::RecordAllocate(void* allocatedPtr, size_t size, const char* desc, const char* file, size_t line)
    {
        // Init() 呼び出し前に、静的変数がoperator new() を呼び出す可能性があるため
        if (!s_Initialized)
            Init();

        {
            std::scoped_lock lock(allocationData->mutex);

            AllocationElement& element = allocationData->memoryMap[allocatedPtr];
            element.size = size;
            element.file = file;
            element.line = line;
            element.desc = desc;

            allocationData->totalAllocationSize += size;
        }
    }

    void MemoryTracker::RecordDeallocate(void* ptr)
    {
        if (ptr == nullptr)
            return;

        // メモリートラッカー解放後のアクセスを抑制
        if (s_Initialized)
        {
            std::scoped_lock lock(allocationData->mutex);

            auto itr = allocationData->memoryMap.find(ptr);
            if (itr != allocationData->memoryMap.end())
            {
                auto& [p, element] = *itr;

                allocationData->totalAllocationSize -= element.size;
                allocationData->memoryMap.erase(ptr);
            }
        }
    }

    void MemoryTracker::DumpMemoryStats()
    {
#if SL_ENABLE_TRACK_HEAP_ALLOCATION
        SL_LOG_DEBUG("***************************************************************************************************");
        SL_LOG_DEBUG("MemoryUsage: {} byte", s_AllocationData->TotalAllocationSize);

        if (!s_AllocationData->memoryMap.empty())
        SL_LOG_DEBUG("***************************************************************************************************");

        for (auto& [ptr, info] : s_AllocationData->memoryMap)
        {
            SL_LOG_DEBUG(" {:>8} byte | {:<16} | {} [{}]", info.size, info.desc, info.file, info.line);
        }
        SL_LOG_DEBUG("***************************************************************************************************");
#endif
    }
}
