/*
* msvc: cl /O2 /GS- /std:c++20 /await:strict /await:heapelide /Zi /EHsc /Fe.\_build\ tutorial1.cpp
* Add "/FAcs" for .asm
*/

#include <algorithm>
#include <atomic>
#include <coroutine>
#include <cstdio>
#include <system_error>
#include <thread>
#include <vector>
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
    static constexpr unsigned MIN_THREAD_COUNT = 2;
    static constexpr unsigned MAX_THREAD_COUNT = 32;

public:
    CThreadPool() :
        m_threadPool(::CreateThreadpool(nullptr))
    {
        if (m_threadPool == nullptr)
        {
            DWORD errorCode = ::GetLastError();
            throw(std::system_error{static_cast<int>(errorCode), std::system_category(), "CreateThreadPool()"});
        }

        const unsigned idealThreadCount = GetIdealThreadCount();
        auto maxThreadCount = std::max<unsigned>(idealThreadCount, MIN_THREAD_COUNT);

        ::SetThreadpoolThreadMaximum(m_threadPool, maxThreadCount);
        if (!::SetThreadpoolThreadMinimum(m_threadPool, MIN_THREAD_COUNT))
        {
            DWORD errorCode = ::GetLastError();
            ::CloseThreadpool(m_threadPool);
            throw(std::system_error{static_cast<int>(errorCode), std::system_category(), "SetThreadpoolThreadMinimum()"});
        }

        //m_hIOPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
        //if (m_hIOPort == nullptr)
        //{
        //    DWORD errorCode = ::GetLastError();
        //    ::CloseThreadpool(m_threadPool);
        //    throw(std::system_error{static_cast<int>(errorCode), std::system_category(), "CreateIoCompletionPort()"});
       // }
    }

    ~CThreadPool() noexcept
    {
        // At this point, all thread pool threads should have exited
        //if (m_hIOPort != nullptr)
        //    ::CloseHandle(m_hIOPort);

        if (m_threadPool == nullptr)
            ::CloseThreadpool(m_threadPool);
    }

    void ScheduleAwaiter(std::coroutine_handle<>* pHandle) noexcept
    {
        // TODO: Use CloseThreadpoolWork
        //::PostQueuedCompletionStatus(
        //    m_hIOPort,
        //    0,
        //    reinterpret_cast<ULONG_PTR>(pHandle->address()), // dwCompletionKey
        //    nullptr);
    }

    void StartShutDown() noexcept
    {
    }

    void Shutdown() noexcept
    {
    }

private:
    HANDLE m_hIOPort;
    TP_CALLBACK_ENVIRON environ_;
    PTP_WORK work_;
    PTP_POOL m_threadPool;
    std::atomic<bool> m_shutdown{ false };
};


struct ReturnObject
{
    struct promise_type
    {
        ReturnObject get_return_object()
        {
            return {
              .h_ = std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_never initial_suspend() const noexcept { return {}; }

        // final_suspend() needs to return std::suspend_always to use the coroutine_handle
        // See co_return section at https://www.scs.stanford.edu/~dm/blog/c++-coroutines.html#the-co_return-operator
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
    };

    std::coroutine_handle<promise_type> h_;
    operator std::coroutine_handle<promise_type>() const { return h_; }
    // A coroutine_handle<promise_type> converts to coroutine_handle<>
    operator std::coroutine_handle<>() const { return h_; }
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
    std::vector<std::coroutine_handle<>> coro_handles;

    for (unsigned i = 0; i < 1000; ++i)
        coro_handles.push_back(Counter(&threadPool, i));

    for (unsigned i = 0; i < 1000; ++i)
    {
        while (!coro_handles[i].done())
        {
            Sleep(10);
        }

        coro_handles[i].destroy();

        printf("%03d: Done\n", i);
    }

    threadPool.StartShutDown();
    threadPool.Shutdown();

    return 0;
}
