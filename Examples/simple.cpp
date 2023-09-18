// cl /await:strict /await:heapelide /O2 /Zi /Fe.\_build\ simple.cpp

#include <coroutine>
#include <cstdio>

struct simple
{
	struct promise_type
	{
		static void* operator new(std::size_t s)
		{
			printf("new operator size=%zd\n", s);
			return ::operator new(s);
		}

		static void operator delete(void* ptr, std::size_t s)
		{
			printf("delete operator size=%zd\n", s);
			::operator delete(ptr);
		}

		int value = 0;

		auto get_return_object() noexcept { return simple(std::coroutine_handle<promise_type>::from_promise(*this)); }
		auto initial_suspend() noexcept { return std::suspend_never{}; }
		auto final_suspend() noexcept { return std::suspend_always{}; }
		void unhandled_exception() noexcept { }
		void return_value(int v) noexcept { value = v; }
	};

	simple(std::coroutine_handle<promise_type> coro) noexcept : m_coro(coro) { }
	simple(simple&& other) noexcept : m_coro(other.m_coro) { other.m_coro = nullptr; }
	~simple()
	{
		if (m_coro)
			m_coro.destroy();
	}

	int value() const noexcept { return m_coro.promise().value; }

	std::coroutine_handle<promise_type> m_coro;
};

simple Simple() noexcept
{
	co_return 42;
}

int main()
{
	auto t = Simple();
	printf("Return value=%d\n", t.value());
}