#include <coroutine>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// The value type we're going to pass along our coroutine chain. This
// is wrapped in a further std::optional so that we can signal the end
// of the data stream by delivering std::nullopt.
struct Value {
    // Our example case is FizzBuzz, so our value contains the current
    // integer in our count, and a string containing the fizzes and
    // buzzes we've accumulated so far.
    int number;
    std::vector<std::string> fizzes;
};

class UserFacing {
  public:
    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    class InputAwaiter {
        promise_type *promise;
      public:
        InputAwaiter(promise_type *);

        bool await_ready() { return false; }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<>);
        std::optional<Value> await_resume();
    };

    class OutputAwaiter {
        promise_type *promise;
      public:
        OutputAwaiter(promise_type *);

        bool await_ready() { return false; }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<>);
        void await_resume() {}
    };

    class promise_type {
        promise_type *consumer = nullptr;

        // Prevent accidentally copying the promise type
        promise_type(const promise_type &) = delete;
        promise_type &operator=(const promise_type &) = delete;

      public:
        std::optional<Value> yielded_value;

        promise_type() = default;

        UserFacing get_return_object() {
            auto handle = handle_type::from_promise(*this);
            return UserFacing{handle};
        }
        std::suspend_always initial_suspend() { return {}; }
        void return_void() {}
        void unhandled_exception() {}
        std::suspend_always final_suspend() noexcept { return {}; }

        OutputAwaiter yield_value(Value value) {
            yielded_value = value;
            return OutputAwaiter{consumer};
        }

        InputAwaiter await_transform(UserFacing &uf);
    };

  private:
    handle_type handle;

    UserFacing(handle_type handle) : handle(handle) {}

    UserFacing(const UserFacing &) = delete;
    UserFacing &operator=(const UserFacing &) = delete;

  public:
    std::optional<Value> next_value() {
        auto &promise = handle.promise();
        promise.yielded_value = std::nullopt;
        if (!handle.done())
            handle.resume();
        return promise.yielded_value;
    }

    UserFacing(UserFacing &&rhs) : handle(rhs.handle) {
        rhs.handle = nullptr;
    }
    UserFacing &operator=(UserFacing &&rhs) {
        if (handle)
            handle.destroy();
        handle = rhs.handle;
        rhs.handle = nullptr;
        return *this;
    }
    ~UserFacing() {
        if (handle)
            handle.destroy();
    }
};

// ----------------------------------------------------------------------
// Out-of-line method definitions, which couldn't be written until
// all the types were complete.

UserFacing::InputAwaiter::InputAwaiter(promise_type *promise)
    : promise(promise) {}
UserFacing::OutputAwaiter::OutputAwaiter(promise_type *promise)
    : promise(promise) {}

std::coroutine_handle<>
UserFacing::InputAwaiter::await_suspend(std::coroutine_handle<>) {
    promise->yielded_value = std::nullopt;
    return handle_type::from_promise(*promise);
}
std::coroutine_handle<>
UserFacing::OutputAwaiter::await_suspend(std::coroutine_handle<>) {
    if (promise)
        return handle_type::from_promise(*promise);
    else
        return std::noop_coroutine();
}

std::optional<Value> UserFacing::InputAwaiter::await_resume() {
    return promise->yielded_value;
}

auto UserFacing::promise_type::await_transform(UserFacing &uf) -> InputAwaiter {
    promise_type &producer = uf.handle.promise();
    producer.consumer = this;
    return InputAwaiter{&producer};
}

// ----------------------------------------------------------------------
// Now for the example code that actually makes and uses some coroutines.

UserFacing generate_numbers(int limit) {
    for (int i = 1; i <= limit; i++) {
        Value v;
        v.number = i;
        co_yield v;
    }
}

UserFacing check_multiple(UserFacing source, int divisor, std::string fizz) {
    while (std::optional<Value> vopt = co_await source) {
        Value &v = *vopt;

        if (v.number % divisor == 0)
            v.fizzes.push_back(fizz);

        co_yield v;
    }
}

int main() {
    UserFacing c = generate_numbers(200);
    c = check_multiple(std::move(c), 3, "Fizz");
    c = check_multiple(std::move(c), 5, "Buzz");
    while (std::optional<Value> vopt = c.next_value()) {
        Value v = *vopt;

        if (v.fizzes.empty()) {
            std::cout << v.number << std::endl;
        } else {
            for (auto &fizz: v.fizzes)
                std::cout << fizz;
            std::cout << std::endl;
        }
    }
}
