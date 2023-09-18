/*
* https://www.scs.stanford.edu/~dm/blog/c++-coroutines.html
* g++ -fcoroutines -std=c++20 -Wall -Werror corodemo.cc
* clang++ -std=c++20 -stdlib=libc++ -fcoroutines-ts -Wall -Werror corodemo.cc
* msvc cl /O2 /GS- /std:c++20 /await:strict /await:heapelide /Zi /EHsc /Fe.\_build\ corodemo.cc
*/

// References
//  * https://en.cppreference.com/w/cpp/language/coroutines
//  * https://lewissbaker.github.io/2022/08/27/understanding-the-compiler-transform

/* 
  D:\Github\Coroutine\Examples>_build\corodemo.exe

  In main1 function
  counter: 0
  In main1 function
  counter: 1
  In main1 function
  counter: 2
  
  In main2 function
  counter2: 0
  In main2 function
  counter2: 1
  In main2 function
  counter2: 2
  counter3: 0
  counter3: 1
  counter3: 2
  counter4: 0
  counter4: 1
  counter4: 2
  counter5: 0
  counter5: 1
  counter5: 2
  promise_type destroyed
  counter6: 0
  counter6: 1
  counter6: 2
*/

//#include <concepts>
#include <coroutine>
//#include <exception>
#include <iostream>

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
  std::coroutine_handle<> *hp_;

  constexpr bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) noexcept
  {
      if (hp_ != nullptr) {
          *hp_ = h;
          hp_ = nullptr;
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

void main1()
{
  std::coroutine_handle<> h;
  counter(&h);
  
  for (int i = 0; i < 3; ++i) {
    std::cout << "In main1 function\n";
    h(); // calling coroutine_handle will resume the counter coroutine. Resume 3 time.
  }
  
  h.destroy();
}

struct ReturnObject2
{
  struct promise_type
  {
    ReturnObject2 get_return_object()
    {
      // Instantiates a ReturnObject2
      return {
        // Uses C++20 designated initializer
        .h_ = std::coroutine_handle<promise_type>::from_promise(*this)
      };
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };

  std::coroutine_handle<promise_type> h_;
  operator std::coroutine_handle<promise_type>() const { return h_; }
  // A coroutine_handle<promise_type> converts to coroutine_handle<>
  operator std::coroutine_handle<>() const { return h_; }
};

ReturnObject2 counter2()
{
  for (unsigned i = 0;; ++i) {
    co_await std::suspend_always{};
    std::cout << "counter2: " << i << std::endl;
  }
}

void
main2()
{
  std::coroutine_handle<> h = counter2();
  for (int i = 0; i < 3; ++i) {
    std::cout << "In main2 function\n";
    h();
  }
  h.destroy();
}

struct ReturnObject3 {
  struct promise_type {
    unsigned value_;

    ReturnObject3 get_return_object() {
      return ReturnObject3 {
        .h_ = std::coroutine_handle<promise_type>::from_promise(*this)
      };
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };

  std::coroutine_handle<promise_type> h_;
  operator std::coroutine_handle<promise_type>() const { return h_; }
};

template<typename PromiseType>
struct GetPromise {
  PromiseType *p_;
  bool await_ready() { return false; } // says yes call await_suspend
  bool await_suspend(std::coroutine_handle<PromiseType> h) {
    p_ = &h.promise();
    return false;         // says no don't suspend coroutine after all
  }
  PromiseType *await_resume() { return p_; }
};

ReturnObject3
counter3()
{
  auto pp = co_await GetPromise<ReturnObject3::promise_type>{};

  for (unsigned i = 0;; ++i) {
    pp->value_ = i;
    co_await std::suspend_always{};
  }
}

void
main3()
{
  std::coroutine_handle<ReturnObject3::promise_type> h = counter3();
  ReturnObject3::promise_type &promise = h.promise();
  for (int i = 0; i < 3; ++i) {
    std::cout << "counter3: " << promise.value_ << std::endl;
    h();
  }
  h.destroy();
}

struct ReturnObject4 {
  struct promise_type {
    unsigned value_;

    ReturnObject4 get_return_object() {
      return {
        .h_ = std::coroutine_handle<promise_type>::from_promise(*this)
      };
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
    std::suspend_always yield_value(unsigned value) {
      value_ = value;
      return {};
    }
  };

  std::coroutine_handle<promise_type> h_;
};

ReturnObject4
counter4()
{
  for (unsigned i = 0;; ++i)
    co_yield i;       // co yield i => co_await promise.yield_value(i)
}

void
main4()
{
  auto h = counter4().h_;
  auto &promise = h.promise();
  for (int i = 0; i < 3; ++i) {
    std::cout << "counter4: " << promise.value_ << std::endl;
    h();
  }
  h.destroy();
}

struct ReturnObject5 {
  struct promise_type {
    unsigned value_;

    ~promise_type() { std::cout << "promise_type destroyed" << std::endl; }
    ReturnObject5 get_return_object() {
      return {
        .h_ = std::coroutine_handle<promise_type>::from_promise(*this)
      };
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    std::suspend_always yield_value(unsigned value) {
      value_ = value;
      return {};
    }
    void return_void() {}
  };

  std::coroutine_handle<promise_type> h_;
};

ReturnObject5
counter5()
{
  for (unsigned i = 0; i < 3; ++i)
    co_yield i;
  // falling off end of function or co_return; => promise.return_void();
  // (co_return value; => promise.return_value(value);)
}

void
main5()
{
  auto h = counter5().h_;
  auto &promise = h.promise();
  while (!h.done()) { // Do NOT use while(h) (which checks h non-NULL)
    std::cout << "counter5: " << promise.value_ << std::endl;
    h();
  }
  h.destroy();
}

template<typename T>
struct Generator {
  struct promise_type;
  using handle_type = std::coroutine_handle<promise_type>;

  struct promise_type {
    T value_;
    std::exception_ptr exception_;

    Generator get_return_object() {
      return Generator(handle_type::from_promise(*this));
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { exception_ = std::current_exception(); }
    
    template<std::convertible_to<T> From> // C++20 concept
    std::suspend_always yield_value(From &&from) {
      value_ = std::forward<From>(from);
      return {};
    }

    void return_void() {}
  };

  handle_type h_;

  Generator(handle_type h) : h_(h) {}
  ~Generator() { h_.destroy(); }
  explicit operator bool() {
    fill();
    return !h_.done();
  }
  T operator()() {
    fill();
    full_ = false;
    return std::move(h_.promise().value_);
  }

private:
  bool full_ = false;

  void fill() {
    if (!full_) {
      h_();
      if (h_.promise().exception_)
        std::rethrow_exception(h_.promise().exception_);
      full_ = true;
    }
  }
};

Generator<unsigned>
counter6()
{
  for (unsigned i = 0; i < 3;)
    co_yield i++;
}

void
main6()
{
  auto gen = counter6();
  while (gen)
    std::cout << "counter6: " << gen() << std::endl;
}

int
main()
{
  main1();
  main2();
  main3();
  main4();
  main5();
  main6();
}
