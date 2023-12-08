//
//  Compiler: cl /EHsc /await /Zi /Fe.\_build\ demo1.cpp 
//

#include <windows.h>
#include <future>
#include <cstdio>
#include <coroutine>
//using namespace std::experimental;

DWORD g_mainThread;

// Awaiter type
struct NotOnMainThread
{
    static void callback(_Inout_ PTP_CALLBACK_INSTANCE Instance, _Inout_opt_ PVOID Context)
    {
        printf("inside callback: %x\n", GetCurrentThreadId());
        std::coroutine_handle<>::from_address(Context).resume();
    }
    
    DWORD m_error;

    // The purpose of the await_ready() method is to allow you to avoid the cost of the <suspend-coroutine> operation
    // in cases where it is known that the operation will complete synchronously without needing to suspend.
    bool await_ready() { return GetCurrentThreadId() != g_mainThread; }

    // It is the responsibility of the `await_suspend()` method to schedule the coroutine
    // for resumption (or destruction) at some point in the future once the operation has completed.
    // Note that returning false from `await_suspend()` counts as scheduling the coroutine
    // for immediate resumption on the current thread.
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

    // The return-value of the await_resume() method call becomes the result of the co_await expression.
    // The await_resume() method can also throw an exception in which case the exception propagates out of the co_await expression.
    // Note that if an exception propagates out of the await_suspend() call then the coroutine is automatically resumed
    // and the exception propagates out of the co_await expression without calling await_resume().
    DWORD await_resume() { return m_error; }
};

// The caller-level type
struct Task
{
    struct promise_type
    {
        using handle_t = std::coroutine_handle<promise_type>;

        Task get_return_object()
        {
            return Task { handle_t::from_promise(*this) };
        }

        std::suspend_never initial_suspend() const noexcept { return {}; }

        // final_suspend() needs to return std::suspend_always to use the coroutine_handle
        // without this, the promise would be destructed as the corountine finishes running.
        // We also need to write a Task destructor since the coroutine must be explicitly destroyed.
        // See co_return section at https://www.scs.stanford.edu/~dm/blog/c++-coroutines.html#the-co_return-operator
        std::suspend_always final_suspend() noexcept { return {}; }

        // This is called on co_return 
        void return_value(DWORD result) noexcept { m_result = result; }
        void unhandled_exception() noexcept {}
        DWORD result() noexcept
        {
            return m_result;
        }

    private:
        DWORD m_result = 0;
    };

    explicit Task(promise_type::handle_t coroutine) : m_coroutine(coroutine) { }

    // move only
    Task(Task&& task ) noexcept : m_coroutine( std::exchange(task.m_coroutine, {} ) ) { }
    Task() noexcept = delete;
    Task(const Task&) = delete;
    Task& operator = (const Task&) = delete;
    Task& operator = (Task&& task) noexcept = delete;

    ~Task() noexcept
    {
        // it's necessary to destroy when initial_suspend returns std::suspend_always
        // See https://en.cppreference.com/w/cpp/coroutine/coroutine_handle#Example
        //if (m_coroutine)
        //{
        //    m_coroutine.destroy();
        //}
    }

    DWORD result() const noexcept { return m_coroutine.promise().result(); }
    bool done() const noexcept { return m_coroutine.done(); }

    //operator std::coroutine_handle<promise_type>() const { return m_coroutine; }
    // A coroutine_handle<promise_type> converts to coroutine_handle<>
    //operator std::coroutine_handle<>() const { return m_coroutine; }

private:
    promise_type::handle_t m_coroutine;
};


Task f()
{
    printf("entered on thread %x\n", GetCurrentThreadId());
    DWORD err = co_await NotOnMainThread();
    if (err != ERROR_SUCCESS)
    {
        co_return err;
    }

    printf("resumed on thread %x\n", GetCurrentThreadId());
    co_return ERROR_SUCCESS;
}


int main()
{
    g_mainThread = GetCurrentThreadId();
    Task task = f();

    // Wait until the task is done
    while (!task.done())
    {
        printf(">>> Sleep 1ms\n");
        Sleep(1);
    }

    printf("Result: %d\n", task.result());

    return 0;
}
