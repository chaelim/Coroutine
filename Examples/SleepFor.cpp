#include <system_error>
#include <chrono>
#include <future>
#include <cstdio>
#include <experimental/coroutine>
#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <threadpoolapiset.h>
using namespace std::experimental;

std::atomic<bool> g_signaled = false;

static void TimerCallback(PTP_CALLBACK_INSTANCE, void* Context, PTP_TIMER)
{
    coroutine_handle<>::from_address(Context).resume();
}

class sleep_for
{
private:
    PTP_TIMER m_timer = nullptr;
    std::chrono::system_clock::duration m_duration;
    
public:
    explicit sleep_for(std::chrono::system_clock::duration d) : m_duration(d) { }

    bool await_ready() const { return m_duration.count() <= 0; }
    void await_resume() {}
    void await_suspend(std::experimental::coroutine_handle<> coro)
    {
        int64_t relative_count = -m_duration.count();
        m_timer = CreateThreadpoolTimer(&TimerCallback, coro.address(), nullptr);
        assert(m_timer && "terrible, just terrible");
        SetThreadpoolTimer(m_timer, (PFILETIME)&relative_count, 0, 0);
    }
    ~sleep_for()
    {
        if (m_timer)
            CloseThreadpoolTimer(m_timer);
    }
};

std::future<void> test(int id)
{
    using namespace std::chrono_literals;


    // Wait until main() signals
    // NOTE:
    // Can not use thread blocking wait (i.e. conditional variable) because it's not threaded.
    // I can create awaitable wait_for_signal function but that still needs to create a thread
    // or using a thread in thread pool.
    // Can I use IOCP for this purpose? main() posts an signal event to IOCP and an IOCP thread's
    // callback will resume the wait_for_signal. -CSLIM
    // while (!g_signaled)
    // await sleep_for(1ms);

    printf("%03d(%04x): sleeping...\n", id, GetCurrentThreadId());
    co_await sleep_for(10ms);
    printf("%03d(%04x): woke up ... going to sleep again...\n", id, GetCurrentThreadId());
    co_await sleep_for(10ms);;
    printf("%03d(%04x): ok. you woke me up again. I am out of here\n", id, GetCurrentThreadId());
}

void main()
{
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < 100; i++)
        futures.push_back(test(i));
    
    g_signaled = true;
    
    for (auto &f : futures)
        f.get();
}
