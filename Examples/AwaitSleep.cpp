// Originally from https://mail.google.com/mail/u/0/#search/gor/1502d819c028beb9
// cl /std:c++latest /await /EHsc /O2 /Ox /EHsc /DNDEBUG /Fe.\_build\ <Cpp FileName>

#include <cstdio>
#include <experimental/coroutine>
#include <future>
#include <chrono>
#include <thread>

#include <windows.h>
#include <Synchapi.h>
#include <threadpoolapiset.h>

using namespace std::experimental;
using namespace std::chrono_literals;

class awaiter
{
    static void CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE, void *Context, PTP_TIMER) {
        coroutine_handle<>::from_address(Context)();
    }
    PTP_TIMER timer = nullptr;
    std::chrono::system_clock::duration duration;

public:
    explicit awaiter(std::chrono::system_clock::duration d) : duration(d) {}

    bool await_ready() const { return duration.count() <= 0; }
    bool await_suspend(std::experimental::coroutine_handle<> resume_cb) {
        printf("Create timer to wait for %lld ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        int64_t relative_count = -duration.count();
        timer = CreateThreadpoolTimer(TimerCallback, resume_cb.to_address(), nullptr);
        SetThreadpoolTimer(timer, (PFILETIME)&relative_count, 0, 0);
        return timer != 0;
    }
    void await_resume() {}

    ~awaiter() {
        if (timer)
          CloseThreadpoolTimer(timer);
    }
};

auto sleep_for(std::chrono::system_clock::duration duration) {
    return awaiter{duration};
}

auto operator await(std::chrono::system_clock::duration duration) {
    return awaiter{duration};
}

SRWLOCK lock = {SRWLOCK_INIT};

struct coro {
    struct promise_type {
        auto get_return_object() { return coro{}; }
        bool initial_suspend() { return false; }
        bool final_suspend() { return false; }
        void return_void() {}
    };
};

// coro test()
// or
// std::future<void> test()

std::future<void> test()
{
    puts("sleeping 200ms...");
    co_await sleep_for(200ms);
    puts(" woke up ... going to sleep again 1000ms...");
    await 1000ms;
    puts(" ok. you woke me up again. I am out of here\n");
    ReleaseSRWLockExclusive(&lock);
}

/*

One thing that is somewhat awkward:

   operator await(1ms);

is not the same as

   await 1ms;

The first one, gets me an awaitable from 1ms. Another is await-ing 1ms.

The behavior of (x || y) is different from calling operator||(x,y).
I don't like it much. I don't mind operator await to be renamed to get_awaiter or something, but, I do think that operator await is prettier and it is easier to think that language synthesizes operator await for some types rather than synthesizing a function.

Again, I am slightly leaning toward operator await, rather than get_awaiter, but if Core/Evolution/LWG/LEWG wants something else. Absolutely.
*/

int main()
{
    //puts("sleeping 100ms...");
    //operator await(100ms); <== this doesnot work.

    auto start = std::chrono::steady_clock::now();

    AcquireSRWLockExclusive(&lock);
    test();
    AcquireSRWLockExclusive(&lock);
    
    auto diff = std::chrono::steady_clock::now() - start;
    printf("Time elapsed %lld ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(diff).count());
}