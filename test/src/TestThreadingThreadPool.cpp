// Copyright (c) 2022 Haofan Zheng
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <gtest/gtest.h>

#ifdef _MSC_VER
#include <windows.h>
#endif // _MSC_VER
#include <SimpleConcurrency/Threading/LambdaTask.hpp>
#include <SimpleConcurrency/Threading/ThreadPool.hpp>


namespace SimpleConcurrency_Test
{
	extern size_t g_numOfTestFile;
}


#ifndef SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE
using namespace SimpleConcurrency;
#else
using namespace SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE;
#endif


GTEST_TEST(Test_Threading_ThreadPool, CountTestFile)
{
	static auto tmp = ++SimpleConcurrency_Test::g_numOfTestFile;
	(void)tmp;
}


GTEST_TEST(Test_Threading_ThreadPool, CreationAndDestruction)
{
	Threading::ThreadPool pool(1);

	pool.Terminate();
}


GTEST_TEST(Test_Threading_ThreadPool, SingleTask)
{
	Threading::ThreadPool pool(1);


	std::atomic_uint64_t count(0);
	auto threadFunc =
		[&count](const std::atomic_bool& isTerminated)
		{
			if(!isTerminated)
			{
				++count;
			}
		};


	auto task1 = Threading::MakeLambdaTask(
		threadFunc
	);

	pool.AddTask(std::move(task1));

	// Wait for the task to finish
	while(count == 0) {}

	EXPECT_EQ(count, 1);

	std::cout << "TEST" << " " << "SingleTask" << std::endl;
	pool.Terminate();
}


GTEST_TEST(Test_Threading_ThreadPool, PendingTaskList)
{
	Threading::ThreadPool pool(1);

	std::thread::id mainThreadId = std::this_thread::get_id();


	std::atomic_uint64_t count(0);
	// In this test, two tasks should be executed one after another
	// so we don't need to use a mutex to protect the threadIds
	std::vector<std::thread::id> threadIds;
	auto threadFunc =
		[&count, &threadIds](const std::atomic_bool& isTerminated)
		{
			if(!isTerminated)
			{
				threadIds.push_back(std::this_thread::get_id());
				++count;
			}
		};
	auto finishFunc =
		[mainThreadId, &count]()
		{
			// this function should be called in the main thread
			EXPECT_EQ(std::this_thread::get_id(), mainThreadId);
			++count;
		};

	// ====== first task ======

	auto task1 = Threading::MakeLambdaTask(
		threadFunc,
		finishFunc
	);
	pool.AddTask(std::move(task1));

	// Wait for the task to finish
	while(count == 0) {}
	ASSERT_EQ(count, 1);

	// ====== second task ======

	auto task2 = Threading::MakeLambdaTask(
		threadFunc,
		finishFunc
	);
	pool.AddTask(std::move(task2));

	// Wait for the task to finish
	while(count == 1) {}
	ASSERT_EQ(count, 2);


	ASSERT_EQ(threadIds.size(), 2);
	EXPECT_EQ(threadIds[0], threadIds[1]);


	// wait for the finish jobs to finish
	while (count < 4)
	{
		// run finish jobs
		pool.Update();
	}


	std::cout << "TEST" << " " << "PendingTaskList" << std::endl;
	pool.Terminate();
}


GTEST_TEST(Test_Threading_ThreadPool, CreateNewThreadForNewTask)
{
	Threading::ThreadPool pool(2);

	std::thread::id mainThreadId = std::this_thread::get_id();

	std::atomic_uint64_t count(0);
	// In this test, two tasks will be executed in parallel
	// so we need to use a mutex to protect the threadIds
	std::mutex threadIdsMutex;
	std::vector<std::thread::id> threadIds;
	std::atomic_bool stopThread1(false);
	auto threadFunc1 =
		[&count, &stopThread1, &threadIdsMutex, &threadIds](const std::atomic_bool& isTerminated)
		{
			if (!isTerminated)
			{
				std::lock_guard<std::mutex> lock(threadIdsMutex);
				threadIds.push_back(std::this_thread::get_id());
			}
			while (!isTerminated && !stopThread1)
			{
				// make this thread busy
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			++count;
		};
	auto threadFunc2 =
		[&count, &threadIdsMutex, &threadIds](const std::atomic_bool& isTerminated)
		{
			if (!isTerminated)
			{
				std::lock_guard<std::mutex> lock(threadIdsMutex);
				threadIds.push_back(std::this_thread::get_id());
			}
			++count;
		};
	auto finishFunc =
		[mainThreadId, &count]()
		{
			// this function should be called in the main thread
			EXPECT_EQ(std::this_thread::get_id(), mainThreadId);
			++count;
		};


	// ====== first task ======

	auto task1 = Threading::MakeLambdaTask(
		threadFunc1,
		finishFunc
	);
	pool.AddTask(std::move(task1));

	// ====== second task ======

	auto task2 = Threading::MakeLambdaTask(
		threadFunc2,
		finishFunc
	);
	pool.AddTask(std::move(task2));


	// now we can free the first thread
	stopThread1 = true;
	// wait for the tasks to finish
	while (count < 2) {}


	ASSERT_EQ(threadIds.size(), 2);
	EXPECT_NE(threadIds[0], threadIds[1]);

	// wait for the finish jobs to finish
	while (count < 4)
	{
		// run finish jobs
		pool.Update();
	}


	pool.Terminate();
}
