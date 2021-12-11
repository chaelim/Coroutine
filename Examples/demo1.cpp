//
//  Compile commandline
//  cl /std:c++latest /Zi /Fe.\_build\ demo1.cpp
//

#include <coroutine>
#include <memory>
#include <cstdio>
#include <windows.h>

struct ReturnObject
{
    struct promise_type
    {
        ReturnObject get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
    };
};

template<typename T>
struct MyFuture {
    std::shared_ptr<T> value;                           // (3)
    MyFuture(std::shared_ptr<T> p) : value(p) {}
    ~MyFuture() { }
    T get() {                                          // (10)
        return *value;
    }

    struct promise_type {
        std::shared_ptr<T> ptr = std::make_shared<T>(); // (4)
        ~promise_type() { }
        MyFuture<T> get_return_object() {              // (7)
            return ptr;
        }
        void return_value(T v) {
            *ptr = v;
        }
        std::suspend_never initial_suspend() {          // (5)
            return {};
        }
        std::suspend_never final_suspend() noexcept {  // (6)
            return {};
        }
        void unhandled_exception() {
            std::exit(1);
        }
    };
};

MyFuture<int> f()
{
    printf("f(): ThreadId = 0x%X: Hello\n", GetCurrentThreadId());

    co_return 42;
}


int main()
{
    printf("main(): ThreadId = 0x%X: got: %d\n", GetCurrentThreadId(), f().get());
}
