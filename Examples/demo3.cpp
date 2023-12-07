//
//  Compiler: cl /EHsc /await /Zi /Fe.\_build\ demo1.cpp 
//

#include <windows.h>
#include <future>
#include <cstdio>
#include <coroutine>
//using namespace std::experimental;

DWORD g_mainThread;

struct NotOnMainThread
{
    static void callback(_Inout_ PTP_CALLBACK_INSTANCE Instance, _Inout_opt_ PVOID Context)
    {
        printf("inside callback: %x\n", GetCurrentThreadId());
        std::coroutine_handle<>::from_address(Context).resume();
    }
    
    DWORD m_error;

    bool await_ready() { return GetCurrentThreadId() != g_mainThread; }
    DWORD await_resume() { return m_error; }
    bool await_suspend(std::coroutine_handle<> h)
    {
        bool submitted = TrySubmitThreadpoolCallback(&callback, h.address(), nullptr);
        if (!submitted)
            m_error = GetLastError();
        else
            m_error = ERROR_SUCCESS;

        printf("await_suspend submitted %d\n", submitted);

        return submitted;
    }
};

struct ReturnObject
{
    struct promise_type
    {
        DWORD m_error = 0;

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
        void return_value(DWORD error) noexcept { m_error = error; }
        void unhandled_exception() noexcept {}
    };

    DWORD get() const noexcept { return h_.promise().m_error; }

    std::coroutine_handle<promise_type> h_;
    operator std::coroutine_handle<promise_type>() const { return h_; }
    // A coroutine_handle<promise_type> converts to coroutine_handle<>
    operator std::coroutine_handle<>() const { return h_; }
};


ReturnObject f()
{
    printf("entered on thread %x\n", GetCurrentThreadId());
    auto err = co_await NotOnMainThread();
    if (err)
        co_return err;
    printf("resumed on thread %x\n", GetCurrentThreadId());
    co_return ERROR_SUCCESS;
}


int main()
{
    g_mainThread = GetCurrentThreadId();
    std::coroutine_handle<> coro_handle = f();
    while (!coro_handle.done())
    {
        Sleep(10);
    }

    printf("%d\n", f().get());

    coro_handle.destroy();

    return 0;
}
