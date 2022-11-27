/*
* msvc: cl /O2 /GS- /std:c++20 /await:strict /await:heapelide /Zi /EHsc /Fe.\_build\ tutorial1.cpp
* Add "/FAcs" for .asm
*/

#include <algorithm>
#include <atomic>
#include <coroutine>
#include <cstdio>
#include <thread>
#include <windows.h>

inline void PrintThreadIdLog(const char* msg)
{
    printf("Coroutine %s on thread: %u\n", msg, GetCurrentThreadId());
}

// std::thread::hardware_concurrency()
static unsigned GetIdealThreadCount() noexcept
{
    DWORD_PTR dwAffinity, dwAffinitySystem;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &dwAffinity, &dwAffinitySystem))
        return 1;

    unsigned threadCount = 0;

    while (dwAffinity != 0)
    {
        threadCount++;
        dwAffinity &= (dwAffinity - 1);
    }

    return threadCount;
}

class CThreadPool final
{
private:
    static constexpr unsigned MAX_THREAD_COUNT = 32;

public:
    CThreadPool() noexcept :
        m_hIOPort(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0))
    {
        const unsigned idealThreadCount = GetIdealThreadCount();
        m_threadCount = std::min<unsigned>(idealThreadCount, MAX_THREAD_COUNT);

        for (unsigned i = 0; i < m_threadCount; i++)
        {
            m_threads[i] = CreateThread(
                NULL,
                0,
                &CThreadPool::ThreadProc,
                (LPVOID)this,
                0,
                NULL
            );
        }
    }

    ~CThreadPool() noexcept
    {
        // At this point, all thread pool threads should have exited
        CloseHandle(m_hIOPort);
    }

    void ScheduleAwaiter(std::coroutine_handle<>* pHandle) noexcept
    {
        ::PostQueuedCompletionStatus(
            m_hIOPort,
            0,
            reinterpret_cast<ULONG_PTR>(pHandle->address()), // dwCompletionKey
            nullptr);
    }

    void StartShutDown() noexcept
    {
        m_shutdown = true;

        for (unsigned i = 0; i < m_threadCount; i++)
        {
            // Queue null op to wake up a thread
            ::PostQueuedCompletionStatus(
                m_hIOPort,
                0,
                NULL,
                NULL
            );
        }
    }

    void Shutdown() noexcept
    {
        for (unsigned i = 0; i < m_threadCount; i++)
        {
            WaitForSingleObject(m_threads[i], INFINITE);
            m_threads[i] = NULL;
        }

        m_threadCount = 0;
    }

    void ForceShutdown() noexcept
    {
        for (unsigned i = 0; i < m_threadCount; i++)
        {
            if (WaitForSingleObject(m_threads[i], 200) == WAIT_TIMEOUT)
            {
                TerminateThread(m_threads[i], 0);
            }

            m_threads[i] = NULL;
        }

        m_threadCount = 0;
    }

    static DWORD WINAPI ThreadProc(LPVOID lpParameter)
    {
        CThreadPool* threadPool = (CThreadPool*)lpParameter;

        while (!threadPool->m_shutdown.load(std::memory_order::relaxed))
        {
            OVERLAPPED* overlapped;
            ULONG_PTR completionKey;
            DWORD bytes;
            if (GetQueuedCompletionStatus(
                threadPool->m_hIOPort,
                &bytes,
                (PULONG_PTR)&completionKey,
                &overlapped,
                INFINITE))
            {
                if (completionKey != 0)
                    std::coroutine_handle<>::from_address(reinterpret_cast<void*>(completionKey)).resume();
            }
            /*
            else
            {
                DWORD hr = GetLastError();
                if (hr == ERROR_MORE_DATA)
                {
                    appErrorf(TEXT("Not enough pipe read buffer"));
                }
                else if (hr == ERROR_BROKEN_PIPE)
                {
                    debugf(
                        NAME_IpcRpc,
                        TEXT("Pipe closed")
                    );
                }
            }
            */
        }

        return 0;
    }

private:
    HANDLE m_hIOPort;
    unsigned m_threadCount{};
    std::atomic<bool> m_shutdown{ false };

    HANDLE m_threads[MAX_THREAD_COUNT];
};


struct ReturnObject
{
    struct promise_type
    {
        ReturnObject get_return_object() { return {}; }
        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
    };
};

struct Awaiter
{
    CThreadPool* m_pThreadpool;

    constexpr bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept
    {
        m_pThreadpool->ScheduleAwaiter(&h);
    }
    constexpr void await_resume() const noexcept {}
};

ReturnObject Counter(CThreadPool* pThreadpool, unsigned counter) noexcept
{
    PrintThreadIdLog("STARTED");

    Awaiter a { pThreadpool };
    co_await a;

    PrintThreadIdLog("RESUMED");
    printf(">>> counter: %d\n", counter);

    // awaiter destroyed here
}

int main()
{
    CThreadPool threadPool;

    for (unsigned i = 0; i < 1000; ++i)
        Counter(&threadPool, i);

    threadPool.StartShutDown();
    threadPool.Shutdown();

    return 0;
}
