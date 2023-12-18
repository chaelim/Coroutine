#include <coroutine>
#include <iostream>

class UserFacing {
  public:
    class promise_type {
      public:
        UserFacing get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        void return_void() {}
        void unhandled_exception() {}
        std::suspend_always final_suspend() noexcept { return {}; }
    };
};

UserFacing demo_coroutine() {
    std::cout << "we're about to suspend this coroutine" << std::endl;
    co_await std::suspend_always{};
    std::cout << "this won't be printed until after we resume" << std::endl;
    co_return;
}

int main() {
    UserFacing demo_instance = demo_coroutine();
}