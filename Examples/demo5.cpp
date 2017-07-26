template <typename T> struct task {
  struct promise_type {
    task get_return_object() { return {this}; }
    suspend_never initial_suspend() { return {}; }
    suspend_always final_suspend() { return {}; }
    template <typename U> void return_value(U &&value) {}
  };
  ~task() { coro.destroy(); }

private:
  task(promise_type *p)
      : coro(coroutine_handle<promise_type>::from_promise(*p)) {}

  coroutine_handle<promise_type> coro;
};
