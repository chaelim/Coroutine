//
//  Compile command:
//  cl /await /Zi /EHsc /Fe.\_build\ WaitFor.cpp
//

#include <experimental/resumable>
//#include <experimental/generator>
#include <system_error>
#include <chrono>
#include <future>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <threadpoolapiset.h>

#include <stdio.h>

// usage:  await sleep_for(100ms);
auto sleep_for(std::chrono::system_clock::duration duration)
{
    struct Awaiter
    {
        static void TimerCallback(PTP_CALLBACK_INSTANCE, void* Context, PTP_TIMER)
        {
            std::experimental::coroutine_handle<>::from_address(Context)();
        }

        PTP_TIMER m_timer { nullptr };
        std::chrono::system_clock::duration m_duration;

        Awaiter() = delete;
        explicit Awaiter(std::chrono::system_clock::duration d) : m_duration(d) { }
        ~Awaiter()
        {
            if (m_timer)
                CloseThreadpoolTimer(m_timer);
        }

        bool await_ready() const { return m_duration.count() <= 0; }

        void await_suspend(std::experimental::coroutine_handle<> resume_cb)
        {
            int64_t relative_count = -m_duration.count();
            m_timer = CreateThreadpoolTimer(TimerCallback, resume_cb.to_address(), nullptr);
            if (m_timer == nullptr)
                throw std::system_error(GetLastError(), std::system_category());

            SetThreadpoolTimer(m_timer, (PFILETIME)&relative_count, 0, 0);
        }

        void await_resume() { }
   };

   return Awaiter{ duration };
}

//auto operator await(std::chrono::system_clock::duration duration) {
//    return sleep_for{duration};
//}

/*
std::experimental::aync_generator<int> Ticks()
{
    using namespace std::chrono_literals;
    
    for (int tick = 0; ; ++tick) 
    {
        yield tick;
        await sleep_for(1ms);
    }
}

// As a transformer: (adds a timestamp to every observed value)
template<class T> 
//async_generator<pair<T, system_clock::time_point>>
auto Timestamp(async_read_stream<T> S) 
{
    for await(v: S) yield { v, system_clock::now() };
}
*/

std::atomic<bool> g_signaled = false;

auto test(int id) -> std::future<void>
{
    using namespace std::chrono_literals;

    // Wait until main() signals
    // NOTE:
    // Can not use thread blocking wait (i.e. conditional variable) because it's not threaded.
    // I can create awaitable wait_for_signal function but that still needs to create a thread
    // or using a thread in thread pool.
    // Can I use IOCP for this purpose? main() posts an signal event to IOCP and an IOCP thread's
    // callback will resume the wait_for_signal. -CSLIM
    while (!g_signaled)
        await sleep_for(1ms);

    printf("%3d(%04x): sleeping...\n", id, GetCurrentThreadId());
    await sleep_for(10ms);
    printf("%3d(%04x): woke up ... going to sleep again...\n", id, GetCurrentThreadId());
    await sleep_for(10ms);;
    printf("%3d(%04x): ok. you woke me up again. I am out of here\n", id, GetCurrentThreadId());
}

void main()
{
    using namespace std::chrono_literals;
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < 100; i++)
        futures.push_back(test(i));
    
    g_signaled = true;
    
    for (auto &f : futures)
        f.get();
}
