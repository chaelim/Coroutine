// cl /std:c++20 /await:strict /await:heapelide /O2 /Zi /EHsc /Fe.\_build\ IdleTasks.cpp

#include <array>
#include <coroutine>
#include <cstdio>

// The caller-level type
struct IdleTask
{
    // The coroutine level type
    struct promise_type
	{
        using Handle = std::coroutine_handle<promise_type>;

        IdleTask get_return_object()
		{
            return IdleTask{ Handle::from_promise(*this) };
        }
        
		std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int value) noexcept
		{
            current_value = value;
            return {};
        }
        void unhandled_exception() { }
		void return_void() { }

        int current_value;
    };

    explicit IdleTask(promise_type::Handle coroutine) : m_coroutine(coroutine) {}

    // Make move-only
	IdleTask() noexcept = delete;
    IdleTask(const IdleTask&) = delete;
    IdleTask& operator=(const IdleTask&) = delete;
	
	IdleTask(IdleTask&& other) noexcept :
        m_coroutine{other.m_coroutine}
    {
        other.m_coroutine = {};
    }

    IdleTask& operator=(IdleTask&& other) noexcept
    {
        if (this != &other)
        {
            if (m_coroutine)
                m_coroutine.destroy();
            m_coroutine = other.m_coroutine;
            other.m_coroutine = {};
        }
        return *this;
    }

    int Run()
	{
        m_coroutine.resume();
        return m_coroutine.promise().current_value;
    }

private:
    promise_type::Handle m_coroutine;
};

IdleTask IdleTask1()
{
    int x = 0;
    while (true)
	{
        co_yield x++;
    }
}

IdleTask IdleTask2()
{
    int x = 10;
    while (true)
	{
        co_yield x++;
    }
}

IdleTask IdleTask3()
{
    int x = 100;
    while (true)
	{
        co_yield x++;
    }
}

int main()
{
    std::array<IdleTask, 3> tasks { IdleTask1(), IdleTask2(), IdleTask3() };
	for (int i = 0; i < 10; ++i)
	{
		for (IdleTask& task : tasks)
		{
			printf("%d\n", task.Run());
		}
	}
}
