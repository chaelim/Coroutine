//
// https://www.chiark.greenend.org.uk/~sgtatham/quasiblog/coroutines-c++20/co_demo.return.cpp
//
// Coroutine that yield values and also return a final result.
//
// cl /std:c++20 /await:strict /await:heapelide /O2 /Zi /EHsc /Fe.\_build\ co_demo.return.cpp
// Add "/FAcs" for .asm
#include <coroutine>
#include <iostream>
#include <optional>
#include <string>

class UserFacing {
  public:
    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    class promise_type {
      public:
        std::optional<int> yielded_value;
        std::optional<std::string> returned_value;

        UserFacing get_return_object() {
            auto handle = handle_type::from_promise(*this);
            return UserFacing{handle};
        }
        std::suspend_always initial_suspend() { return {}; }
        void return_value(std::string value) {
            returned_value = value;
        }
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

    std::optional<std::string> final_result() {
        return handle.promise().returned_value;
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
        handle.destroy();
    }
};

UserFacing demo_coroutine()
{
    co_yield 100;
    for (int i = 1; i <= 3; i++)
        co_yield i;
    co_yield 200;
    co_return "hello, world";
}

int main()
{
    UserFacing demo_instance = demo_coroutine();

    while (std::optional<int> value = demo_instance.next_value())
        std::cout << "got integer " << (*value) << std::endl;

    if (std::optional<std::string> result = demo_instance.final_result())
        std::cout << "final result = \"" << *result << "\"" << std::endl;
}