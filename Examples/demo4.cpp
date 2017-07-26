#include <Windows.h>
#include <Msp.h>
#include <Msputils.h>
#include <cstdio>
#include <future>
using namespace std::experimental;

LIST_ENTRY g_someQueue;

struct Dequeue
{
    coroutine_handle<> m_hdl;
    LIST_ENTRY& m_q;
    LIST_ENTRY m_link;
    int m_result;  

    explicit Dequeue(LIST_ENTRY &q) : m_q(q) { }
    bool await_ready() { return false; }
    void await_suspend(coroutine_handle<> h)
    {
        m_hdl = h;
        InsertTailList(&m_q, &m_link);
    }

    int await_resume() { return m_result; }
};

void ReleaseAll(int result)
{
    while (!IsListEmpty(&g_someQueue)) {
        PLIST_ENTRY entry = RemoveHeadList(&g_someQueue);
        auto awaitable = CONTAINING_RECORD(entry, Dequeue, m_link);
        awaitable->m_result = result;
        awaitable->m_hdl.resume();
    }
}

auto operator co_await(LIST_ENTRY &q) { return Dequeue(q); }

std::future<void> f(int no)
{
    auto result = co_await g_someQueue;
    printf("%d: got %d\n", no, result);
}

int main() {
    InitializeListHead(&g_someQueue);

    for (int i = 0; i < 5; ++i)
        f(i);

    ReleaseAll(42);
}
