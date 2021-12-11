/*
* https://www.scs.stanford.edu/~dm/blog/c++-coroutines.html
* g++ -fcoroutines -std=c++20 -Wall -Werror tutorial1.cpp
* clang++ -std=c++20 -stdlib=libc++ -fcoroutines-ts -Wall -Werror tutorial1.cpp
* msvc: cl /O2 /GS- /std:c++20 /await:strict /await:heapelide /Zi /EHsc /Fe.\_build\ tutorial1.cpp
*/

#include <coroutine>
#include <iostream>

// 

struct ReturnObject
{
    struct promise_type
    {
        ReturnObject get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };
};

struct Awaiter
{
    std::coroutine_handle<> *m_pHandle;

    constexpr bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) noexcept
    {
        // handle does not change in each iteration.
        if (m_pHandle != nullptr)
        {
            *m_pHandle = handle;
            m_pHandle = nullptr;
        }
    }
    constexpr void await_resume() const noexcept {}
};

ReturnObject counter(std::coroutine_handle<> *continuation_out) noexcept
{
  Awaiter a{continuation_out};

  for (unsigned i = 0;; ++i)
  {
    co_await a;
    std::cout << "counter: " << i << std::endl;
  }
}

int main()
{
    // Hack: Get the coroutine handle from the corountine
    std::coroutine_handle<> h;
    counter(&h);

    for (int i = 0; i < 3; ++i)
    {
        std::cout << "In main1 function\n";
        h(); // calling coroutine_handle will resume the counter coroutine
    }

    h.destroy();

    return 0;
}
