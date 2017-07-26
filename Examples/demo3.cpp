//
//  Compiler: cl /EHsc /await /Zi /Fe.\_build\ demo1.cpp 
//

#include <windows.h>
#include <future>
#include <cstdio>
#include <experimental/coroutine>
using namespace std::experimental;

DWORD g_mainThread;

struct NotOnMainThread
{
    static void callback(_Inout_ PTP_CALLBACK_INSTANCE Instance, _Inout_opt_ PVOID Context)
    {
        printf("inside callback: %x\n", GetCurrentThreadId());
        coroutine_handle<>::from_address(Context).resume();
    }
    
    DWORD m_error;

    bool await_ready() { return GetCurrentThreadId() != g_mainThread; }
    DWORD await_resume() { return m_error; }
    bool await_suspend(coroutine_handle<> h)
    {
        bool submitted = TrySubmitThreadpoolCallback(&callback, h.address(), nullptr);
        if (!submitted)
            m_error = GetLastError();
        else
            m_error = ERROR_SUCCESS;
        return submitted;
    }
};

std::future<DWORD> f()
{
    printf("entered on thread %x\n", GetCurrentThreadId());
    auto err = co_await NotOnMainThread();
    if (err)
        co_return err;
    printf("resumed on thread %x\n", GetCurrentThreadId());
    co_return ERROR_SUCCESS;
}


void main()
{
    g_mainThread = GetCurrentThreadId();
    printf("%d\n", f().get());
}
