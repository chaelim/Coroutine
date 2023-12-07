#include <system_error>
#include <chrono>
#include <cstdio>
#include <coroutine>
#include <assert.h>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <threadpoolapiset.h>
//using namespace std::experimental;

std::atomic<bool> g_signaled = false;

class Awaiter
{
private:
    static void TimerCallback(PTP_CALLBACK_INSTANCE, void* context, PTP_TIMER)
    {
        std::coroutine_handle<>::from_address(context).resume();
    }

    PTP_TIMER m_timer = nullptr;
    std::chrono::system_clock::duration m_duration;
    
public:
    explicit Awaiter(std::chrono::system_clock::duration d) : m_duration(d) { }

    bool await_ready() const { return m_duration.count() <= 0; }
    void await_resume() {}
    void await_suspend(std::coroutine_handle<> h)
    {
        int64_t relative_count = -m_duration.count();
        m_timer = CreateThreadpoolTimer(&TimerCallback, h.address(), nullptr);
        assert(m_timer && "terrible, just terrible");
        SetThreadpoolTimer(m_timer, (PFILETIME)&relative_count, 0, 0);
    }
    ~Awaiter()
    {
        if (m_timer)
            CloseThreadpoolTimer(m_timer);
    }
};

auto sleep_for(std::chrono::system_clock::duration duration)
{
    return Awaiter{duration};
}

struct ReturnObject
{
    struct promise_type
    {
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
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
    };

    std::coroutine_handle<promise_type> h_;
    operator std::coroutine_handle<promise_type>() const { return h_; }
    // A coroutine_handle<promise_type> converts to coroutine_handle<>
    operator std::coroutine_handle<>() const { return h_; }
};

ReturnObject test(int id)
{
    printf("%03d(%04x): entering test\n", id, GetCurrentThreadId());

    using namespace std::chrono_literals;

    // Wait until main() signals
    // NOTE:
    // Can not use thread blocking wait (i.e. conditional variable) because it's not threaded.
    // I can create awaitable wait_for_signal function but that still needs to create a thread
    // or using a thread in thread pool.
    // Can I use IOCP for this purpose? main() posts an signal event to IOCP and an IOCP thread's
    // callback will resume the wait_for_signal. Awaitable events? -CSLIM
    while (!g_signaled)
        co_await sleep_for(1ms);

    printf("%03d(%04x): sleeping...\n", id, GetCurrentThreadId());
    co_await sleep_for(10ms);
    printf("%03d(%04x): woke up ... going to sleep again...\n", id, GetCurrentThreadId());
    co_await sleep_for(10ms);;
    printf("%03d(%04x): ok. you woke me up again. I am out of here\n", id, GetCurrentThreadId());

    //co_return;
}

/*

https://devblogs.microsoft.com/oldnewthing/20210303-00/?p=104922

struct awaitable_event
{
  void set() const { shared->set(); }

  auto await_ready() const noexcept
  {
    return shared->await_ready();
  }

  auto await_suspend(
    std::experimental::coroutine_handle<> handle) const
  {
    return shared->await_suspend(handle);
  }

  auto await_resume() const noexcept
  {
    return shared->await_resume();
  }

private:
  struct state
  {
    std::atomic<bool> signaled = false;
    winrt::slim_mutex mutex;
    std::vector<std::experimental::coroutine_handle<>> waiting;

    void set()
    {
      std::vector<std::experimental::coroutine_handle<>> ready;
      {
        auto guard = winrt::slim_lock_guard(mutex);
        signaled.store(true, std::memory_order_relaxed);
        std::swap(waiting, ready);
      }
      for (auto&& handle : ready) handle();
    }

    bool await_ready() const noexcept
    { return signaled.load(std::memory_order_relaxed); }

    bool await_suspend(
      std::experimental::coroutine_handle<> handle)
    {
      auto guard = winrt::slim_lock_guard(mutex);
      if (signaled.load(std::memory_order_relaxed)) return false;
      waiting.push_back(handle);
      return true;
    }

    void await_resume() const noexcept { }
  };

  std::shared_ptr<state> shared = std::make_shared<state>();
};
*/

int main()
{
    std::vector< std::coroutine_handle<>> coro_handles;

    for (unsigned i = 0; i < 100; ++i)
        coro_handles.push_back(test(i));

    g_signaled = true;

    for (unsigned i = 0; i < 100; ++i)
    {
        while (!coro_handles[i].done())
        {
            Sleep(10);
        }

        coro_handles[i].destroy();

        printf("%03d: Done\n", i);
    }
    
    /*
    std::vector<ReturnObject> futures;
    
    printf("Main thread id = %04x\n", GetCurrentThreadId());

    for (int i = 0; i < 100; i++)
        futures.push_back(test(i));
    
    g_signaled = true;

    for (auto &f : futures)
        f.get();
    */

    return 0;
}
