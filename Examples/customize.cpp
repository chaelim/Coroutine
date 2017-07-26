#include <cstdio>
#include <experimental/coroutine>
using namespace std::experimental;

struct coro
{
    struct promise_type
    {
        coro get_return_object() { return {}; }
        suspend_never initial_suspend() { return {}; }
        suspend_never final_suspend() { return {}; }
        void return_void() {}
    };
};

coro f()
{
    puts("Hello");
    co_return;
}

int main() { f(); }
