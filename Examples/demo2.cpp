//
//  Compiler: cl /await /Zi /Fe.\_build\ demo1.cpp 
//

#include <windows.h>
#include <future>
#include <cstdio>
#include <experimental/coroutine>
using namespace std::experimental;

struct Awaitable
{
    bool await_ready() { return true; }
    int await_resume() { return 42; }
    void await_suspend(coroutine_handle<>) { }
};

std::future<int> f()
{
    printf("%x: Hello\n", GetCurrentThreadId());
    printf("%d\n", co_await Awaitable());
    printf("%x: Hello\n", GetCurrentThreadId());
    co_return 42;
}


void main()
{
    printf("%x: got: %d\n", GetCurrentThreadId(), f().get());
}
