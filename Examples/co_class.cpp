#include <coroutine>
#include <iostream>
#include <optional>
#include <utility>

class Promise {
  public:
    std::coroutine_handle<Promise> get_return_object() {
        return std::coroutine_handle<Promise>::from_promise(*this);
    }

    std::suspend_always initial_suspend() { return {}; }
    void return_void() {}
    void unhandled_exception() {}
    std::suspend_always final_suspend() noexcept { return {}; }

    std::optional<int> yielded_value;
    std::suspend_always yield_value(int value) {
        yielded_value = value;
        return {};
    }
};

template<typename... ArgTypes>
struct std::coroutine_traits<std::coroutine_handle<Promise>, ArgTypes...> {
    using promise_type = Promise;
};

class CoroutineHolder {
    using Handle = std::coroutine_handle<Promise>;

    int limit;
    int total = 0;

    Handle coroutine() {
        for (int i = 0; i < limit; i++) {
            co_yield i;
            total += i;

            co_yield i*i;
            total += i*i;
        }
    }

    Handle handle;

  public:
    CoroutineHolder(int limit) : limit(limit), handle(coroutine()) {}
    ~CoroutineHolder() { handle.destroy(); }

    std::optional<int> next_value() {
        handle.promise().yielded_value = std::nullopt;
        if (!handle.done())
            handle.resume();
        return handle.promise().yielded_value;
    }

    int total_so_far() { return total; }
};

int main() {
    CoroutineHolder ch(100);

    while (auto vopt = ch.next_value())
        std::cout << *vopt << ", total = " << ch.total_so_far() << std::endl;
}