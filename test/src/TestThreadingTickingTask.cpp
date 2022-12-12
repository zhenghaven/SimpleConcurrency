// Copyright (c) 2022 Haofan Zheng
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <gtest/gtest.h>

#ifdef _MSC_VER
#include <windows.h>
#endif // _MSC_VER
#include <SimpleConcurrency/Threading/TickingTask.hpp>
#include <SimpleConcurrency/Threading/ThreadPool.hpp>


namespace SimpleConcurrency_Test
{
	extern size_t g_numOfTestFile;
} // namespace SimpleConcurrency_Test


namespace
{


class TestTickingTask :
	public SimpleConcurrency::Threading::TickingTask<int64_t>
{
public: // static members:

	using Base = SimpleConcurrency::Threading::TickingTask<int64_t>;

public:

	TestTickingTask(
		std::atomic_uint64_t& testCounter1,
		std::atomic_uint64_t& testCounter2
	) :
		Base(),
		m_testCounter1(testCounter1),
		m_testCounter2(testCounter2)
	{}


	TestTickingTask(
		std::atomic_uint64_t& testCounter1,
		std::atomic_uint64_t& testCounter2,
		int64_t updInterval,
		int64_t tickInterval
	) :
		Base(updInterval, tickInterval),
		m_testCounter1(testCounter1),
		m_testCounter2(testCounter2)
	{}


	using Base::SetInterval;


	using Base::DisableTickInterval;


protected:

	virtual void Tick() override
	{
		++m_testCounter1;
	}


	virtual void SleepFor(int64_t interval) const override
	{
		++m_testCounter2;
		std::this_thread::sleep_for(std::chrono::milliseconds(interval));
	}


private:
	std::atomic_uint64_t& m_testCounter1;
	std::atomic_uint64_t& m_testCounter2;

}; // class TestTickingTask


} // namespace


#ifndef SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE
using namespace SimpleConcurrency;
#else
using namespace SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE;
#endif


GTEST_TEST(Test_Threading_TickingTask, CountTestFile)
{
	static auto tmp = ++SimpleConcurrency_Test::g_numOfTestFile;
	(void)tmp;
}


GTEST_TEST(Test_Threading_TickingTask, NoInterval)
{
	{
		std::atomic_uint64_t testCounter1(0);
		std::atomic_uint64_t testCounter2(0);

		std::unique_ptr<TestTickingTask> task(
			new TestTickingTask(testCounter1, testCounter2)
		);

		Threading::ThreadPool pool(1);

		pool.AddTask(std::move(task));

		// wait for the tick to be executed for more than once
		while(testCounter1 <= 1) {}

		pool.Terminate();
		pool.Update();

		EXPECT_GT(testCounter1, 1);
		EXPECT_EQ(testCounter2, 0);
	}

	{
		std::atomic_uint64_t testCounter1(0);
		std::atomic_uint64_t testCounter2(0);

		std::unique_ptr<TestTickingTask> task(
			new TestTickingTask(
				testCounter1,
				testCounter2,
				10,
				100
			)
		);
		task->DisableTickInterval();

		Threading::ThreadPool pool(1);

		pool.AddTask(std::move(task));

		// wait for the tick to be executed for more than once
		while(testCounter1 <= 1) {}

		pool.Terminate();
		pool.Update();

		EXPECT_GT(testCounter1, 1);
		EXPECT_EQ(testCounter2, 0);
	}
}


GTEST_TEST(Test_Threading_TickingTask, WithInterval)
{
	{
		std::atomic_uint64_t testCounter1(0);
		std::atomic_uint64_t testCounter2(0);

		std::unique_ptr<TestTickingTask> task(
			new TestTickingTask(
				testCounter1,
				testCounter2,
				1,
				10
			)
		);

		Threading::ThreadPool pool(1);

		pool.AddTask(std::move(task));

		// wait for the tick to be executed for more than once
		while(testCounter1 <= 1) {}

		pool.Terminate();
		pool.Update();

		EXPECT_GT(testCounter1, 1);
		EXPECT_GT(testCounter2, testCounter1);
	}

	{
		std::atomic_uint64_t testCounter1(0);
		std::atomic_uint64_t testCounter2(0);

		std::unique_ptr<TestTickingTask> task(
			new TestTickingTask(
				testCounter1,
				testCounter2
			)
		);
		task->SetInterval(1, 10);

		Threading::ThreadPool pool(1);

		pool.AddTask(std::move(task));

		// wait for the tick to be executed for more than once
		while(testCounter1 <= 1) {}

		pool.Terminate();
		pool.Update();

		EXPECT_GT(testCounter1, 1);
		EXPECT_GT(testCounter2, testCounter1);
	}
}
