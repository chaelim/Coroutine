//
//  Compiler: cl /std:c++latest /await /Zi /Fe.\_build\ demo2.cpp 
//

#include <windows.h>
#include <future>
#include <cstdio>
#include <coroutine>
//using namespace std::experimental;

// Synchronous awaitable. Return 42 immediately to the calling thread.
struct SynchronousAwaitable
{
    bool await_ready() { return true; }
    int await_resume() { return 42; }
    void await_suspend(std::coroutine_handle<>) { }
};

struct ReturnObject
{
    struct promise_type
    {
        using handle_t = std::coroutine_handle<promise_type>;

        ReturnObject get_return_object()
        {
            return ReturnObject { handle_t::from_promise(*this) };
        }

        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
        void return_value(int v) noexcept { value = v; }

        int value = 0;
    };

    explicit ReturnObject(promise_type::handle_t coro) noexcept : m_coro(coro) { }

    int get() const noexcept { return m_coro.promise().value; }

    std::coroutine_handle<promise_type> m_coro;
};

ReturnObject f()
{
    printf("%x: Hello\n", GetCurrentThreadId());
    int ret = co_await SynchronousAwaitable();
    printf("%x: Return %d\n", GetCurrentThreadId(), ret);
    co_return ret;
}


int main()
{
    printf("%x: got: %d\n", GetCurrentThreadId(), f().get());

    return 0;
}
