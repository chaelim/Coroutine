//#define NDEBUG
#include <stdio.h>
#include <cassert>
#include <coroutine>
#include <exception>
#include <chrono>
//#include <windows.h>

using namespace std;

bool cancel = false;

struct goroutine
{
    static int const N = 10;
    static int count;
    static coroutine_handle<> stack[N];

    static void schedule(coroutine_handle<>& rh)
    {
        assert(count < N);
        stack[count++] = rh;
        rh = nullptr;
    }

    ~goroutine() {}

    static void go(goroutine){}

    static void run_one()
    {
        assert(count > 0);
        stack[--count]();
    }

    struct promise_type
    {
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
        goroutine get_return_object() { return{}; }
    };
};

int goroutine::count;
coroutine_handle<> goroutine::stack[N];

template <typename T>
class channel
{
    T const* pvalue = nullptr;
    coroutine_handle<> reader = nullptr;
    coroutine_handle<> writer = nullptr;

public:
    auto push(T const& value)
    {
        assert(pvalue == nullptr);
        assert(!writer);
        pvalue = &value;

        struct awaiter
        {
            channel* ch;
            bool await_ready() { return false; }
            void await_suspend(coroutine_handle<> rh)
            {
                ch->writer = rh;
                if (ch->reader) goroutine::schedule(ch->reader);
            }
            void await_resume() { }
        };

        return awaiter{ this };
    }

    auto pull()
    {
        assert(!reader);

        struct awaiter
        {
            channel * ch;

            bool await_ready() { return !!ch->writer; }
            void await_suspend(coroutine_handle<> rh) { ch->reader = rh; }
            auto await_resume()
            {
                auto result = *ch->pvalue;
                ch->pvalue = nullptr;
                if (ch->writer)
                {
                    //goroutine::schedule(ch->writer);
                    auto wr = ch->writer;
                    ch->writer = nullptr;
                    wr();
                }
                return result;
            }
        };

        return awaiter{ this };
    }

    void sync_push(T const& value)
    {
        assert(!pvalue);
        pvalue = &value;
        assert(reader);
        reader();
        assert(!pvalue);
        reader = nullptr;
    }

    auto sync_pull()
    {
        while (!pvalue)
        {
            goroutine::run_one();
        }

        auto result = *pvalue;
        pvalue = nullptr;
        if (writer)
        {
            auto wr = writer;
            writer = nullptr;
            wr();
        }
        return result;
    }
};

goroutine pusher(channel<int>& left, channel<int>& right)
{
    for (;;) {
        auto val = co_await left.pull();
        co_await right.push(val + 1);
    }
}

const int N = 100 * 1000;
const int repeat = 1;
//std::vector<channel<int>> c(N + 1);
//channel<int> c[N + 1];

channel<int>* c = new channel<int>[N + 1];

//created in 0.064742 secs. pass thru in 0.032365 secs
// 1000 * 1000 in 32ms
// 1000 in 32 us
// 1 in 32 ns

int main()
{
    
//    SetThreadAffinityMask(GetCurrentThread(),1);
    using clock = std::chrono::high_resolution_clock;
    using dur = std::chrono::duration<float, std::ratio<1, 1>>;

    auto start = clock::now();
    for (int i = 0; i < N; ++i)
    {
        goroutine::go(pusher(c[i], c[i + 1]));
    }

    auto stop = clock::now();
    auto secs1 = std::chrono::duration_cast<dur>(stop - start).count();

    start = clock::now();
    int result = 0;
    for (int j = 0; j < repeat; ++j)
    {
        c[0].sync_push(0);
        result = c[N].sync_pull();
    }
    
    stop = clock::now();
    auto secs2 = std::chrono::duration_cast<dur>(stop - start).count();
    cancel = true;
    printf("created in %f secs. pass thru in %f secs, result %d\n", secs1, secs2 / repeat, result);
    
    if (N == result)
        printf("Pass\n");
    else
        printf("Fail\n");
    return N - result;
}
