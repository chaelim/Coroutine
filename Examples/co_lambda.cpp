#include <coroutine>
#include <iostream>
#include <optional>

class UserFacing {
  public:
    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    class promise_type {
      public:
        std::optional<int> yielded_value;

        UserFacing get_return_object() {
            auto handle = handle_type::from_promise(*this);
            return UserFacing{handle};
        }
        std::suspend_always initial_suspend() { return {}; }
        void return_void() {}
        void unhandled_exception() {}
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int value) {
            yielded_value = value;
            return {};
        }
    };

  private:
    handle_type handle;

    UserFacing(handle_type handle) : handle(handle) {}

    UserFacing(const UserFacing &) = delete;
    UserFacing &operator=(const UserFacing &) = delete;

  public:
    std::optional<int> next_value() {
        auto &promise = handle.promise();
        promise.yielded_value = std::nullopt;
        handle.resume();
        return promise.yielded_value;
    }

    ~UserFacing() {
        handle.destroy();
    }
};

int main() {
    auto lambda_coroutine = []() -> UserFacing {
        co_yield 100;
        for (int i = 1; i <= 3; i++)
            co_yield i;
        co_yield 200;
    };
    UserFacing demo_instance = lambda_coroutine();

    while (std::optional<int> value = demo_instance.next_value())
        std::cout << "got integer " << (*value) << std::endl;
}
