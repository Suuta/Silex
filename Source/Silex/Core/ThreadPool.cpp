
#include "PCH.h"
#include "ThreadPool.h"

namespace Silex
{
    static uint32                   threadCount        = 0;
    static std::atomic<uint32>      workingThreadCount = 0;
    static std::mutex               taskMutex;
    static std::condition_variable  condition;
    static std::vector<std::thread> threads;
    static std::deque<Task>         taskQueue;
    static bool                     isStopping;

    //--------------------------------------------------------------------------------
    // std::condition_variable::wait()
    //--------------------------------------------------------------------------------
    // 条件が指定されているなら、満たされるまで
    // そうでないならなら、notify_one() / notify_all() が呼び出されるまで 待機する
    //--------------------------------------------------------------------------------

    static void ThreadLoop()
    {
        while (true)
        {
            Task task;

            {
                std::unique_lock<std::mutex> lock(taskMutex);

                // キューにタスクが追加される(キューが空ではない)まで待機
                condition.wait(lock, []{ return !taskQueue.empty() || isStopping; });

                // タスクが空で、終了要求(スレッドプール自体)があればこのスレッド(関数)は終了する
                if (isStopping && taskQueue.empty())
                    return;

                task = taskQueue.front();
                taskQueue.pop_front();

                lock.unlock();
            }


            // タスク実行
            workingThreadCount++;
            task();
            workingThreadCount--;
        }
    }

    void ThreadPool::Initialize()
    {
        //---------------------------
        // SL_ASSERT(IsMainThread());
        //---------------------------


        isStopping  = false;
        threadCount = std::thread::hardware_concurrency() - 1;

        // スレッドループを予約
        for (uint32 i = 0; i < threadCount; i++)
            threads.emplace_back(std::thread(&Silex::ThreadLoop));

        SL_LOG_INFO("Launch {} threads (ThreadPool)", threadCount);
    }

    void ThreadPool::Finalize()
    {
        //---------------------------
        // SL_ASSERT(IsMainThread());
        //---------------------------

        // 実行中の
        Flush(true);

        {
            // 終了フラグを立てる
            std::unique_lock<std::mutex> lock(taskMutex);
            isStopping = true;
            lock.unlock();
        }

        // 全ての待機スレッド起動
        condition.notify_all();

        // 全スレッドが終了するまで待機
        for (auto& thread : threads)
            thread.join();

        threads.clear();
    }

    void ThreadPool::AddTask(Task&& task)
    {
        //---------------------------
        // SL_ASSERT(IsMainThread());
        //---------------------------

        {
            // キューにタスクを追加
            std::unique_lock<std::mutex> lock(taskMutex);
            taskQueue.emplace_back(std::bind(std::forward<Task>(task)));
            lock.unlock();
        }

        // 待機中のスレッドを1つだけ起動させる
        condition.notify_one();
    }

    void ThreadPool::Flush(bool removeTaskQueue)
    {
        //---------------------------
        // SL_ASSERT(IsMainThread());
        //---------------------------

        if (removeTaskQueue)
            taskQueue.clear();

        while (HasRunningTask())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }




    uint32 ThreadPool::GetThreadCount()
    { 
        return threadCount;
    }

    uint32 ThreadPool::GetWorkingThreadCount()
    { 
        return workingThreadCount;
    }

    uint32 ThreadPool::GetIdleThreadCount()
    { 
        return threadCount - workingThreadCount;
    }

    bool ThreadPool::HasRunningTask()
    { 
        return GetIdleThreadCount() != GetThreadCount();
    }
}
