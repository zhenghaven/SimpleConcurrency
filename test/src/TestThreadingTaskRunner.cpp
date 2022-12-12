// Copyright (c) 2022 Haofan Zheng
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <thread>

#include <gtest/gtest.h>

#ifdef _MSC_VER
#include <windows.h>
#endif // _MSC_VER
#include <SimpleConcurrency/Threading/LambdaTask.hpp>
#include <SimpleConcurrency/Threading/TaskRunner.hpp>


namespace SimpleConcurrency_Test
{
	extern size_t g_numOfTestFile;
}

namespace
{

class TestException1 :
	public std::runtime_error
{
	using std::runtime_error::runtime_error;
}; // class TestException1

class TestException2 :
	public std::runtime_error
{
	using std::runtime_error::runtime_error;
}; // class TestException2

} // namespace


#ifndef SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE
using namespace SimpleConcurrency;
#else
using namespace SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE;
#endif


GTEST_TEST(Test_Threading_TaskRunner, CountTestFile)
{
	static auto tmp = ++SimpleConcurrency_Test::g_numOfTestFile;
	(void)tmp;
}

GTEST_TEST(Test_Threading_TaskRunner, RunAssignedTask)
{
	std::string testStr1;

	std::string expVal = "Hello";

	auto threadFunc =
		[&testStr1, expVal](const std::atomic_bool& isTerminated)
		{
			if (!isTerminated)
			{
				testStr1 += expVal;
			}
		};


	// ===== Run once =====
	// This should run the assigned task immediately, and then terminate itself
	{
		testStr1.clear();

		auto task1 = Threading::MakeLambdaTask(
			threadFunc
		);
		Threading::Task* taskPtr = task1.get();

		Threading::TaskRunner taskRunner;
		taskRunner.AssignTask(std::move(task1));

		taskRunner.ThreadRunner(
			[&taskRunner, taskPtr]
			(Threading::TaskRunner*, std::unique_ptr<Threading::Task> task)
			{
				EXPECT_EQ(task.get(), taskPtr);
				taskRunner.TerminateTask();
				return nullptr;
			}
		);

		ASSERT_EQ(testStr1, expVal);
	}

	// ===== Run multiple times =====
	// This should run the assigned task, and then re-assigned the finished
	// task to itself, and repeat multiple times
	// After `numOfRuns` reaches, the task runner should terminate itself
	{
		testStr1.clear();

		auto task1 = Threading::MakeLambdaTask(
			threadFunc
		);
		Threading::Task* taskPtr = task1.get();

		Threading::TaskRunner taskRunner;
		taskRunner.AssignTask(std::move(task1));

		size_t numOfRuns = 5;
		taskRunner.ThreadRunner(
			[&taskRunner, taskPtr, &numOfRuns]
			(Threading::TaskRunner*, std::unique_ptr<Threading::Task> task) ->
				std::unique_ptr<Threading::Task>
			{
				EXPECT_EQ(task.get(), taskPtr);
				if (numOfRuns > 0)
				{
					--numOfRuns;
					return task;
				}
				else
				{
					taskRunner.TerminateTask();
					return nullptr;
				}
			}
		);

		ASSERT_EQ(
			testStr1,
			expVal + // first assigned task
				expVal + expVal + expVal + expVal + expVal // `numOfRuns` times
		);
	}
}

GTEST_TEST(Test_Threading_TaskRunner, RunAfterTerminated)
{
	std::string testStr1;

	std::string expVal = "Hello";

	auto threadFunc =
		[&testStr1, expVal](const std::atomic_bool& isTerminated)
		{
			if (!isTerminated)
			{
				testStr1 += expVal;
			}
		};


	{
		testStr1.clear();

		auto task1 = Threading::MakeLambdaTask(
			threadFunc
		);

		Threading::TaskRunner taskRunner;
		taskRunner.TerminateTask();
		taskRunner.AssignTask(std::move(task1));

		// This should return immediately, since the task runner is already
		// terminated
		taskRunner.ThreadRunner(
			[]
			(Threading::TaskRunner*, std::unique_ptr<Threading::Task>) ->
				std::unique_ptr<Threading::Task>
			{
				throw std::runtime_error("This should not be called");
			}
		);

		ASSERT_EQ(testStr1, std::string());
	}
}


GTEST_TEST(Test_Threading_TaskRunner, TaskExceptionHandling)
{
	auto threadFunc =
		[](const std::atomic_bool&)
		{
			throw TestException1("This is a test exception");
		};

	// ===== Re-throw exception =====
	{
		auto task1 = Threading::MakeLambdaTask(
			threadFunc,
			[]() {},
			[]() {},
			[](std::exception_ptr ePtr) { std::rethrow_exception(ePtr); }
		);

		Threading::TaskRunner taskRunner;
		taskRunner.AssignTask(std::move(task1));

		auto testProg = [&taskRunner] ()
		{
			taskRunner.ThreadRunner(
				[]
				(Threading::TaskRunner*, std::unique_ptr<Threading::Task>) ->
					std::unique_ptr<Threading::Task>
				{
					throw std::runtime_error("This should not be called");
				}
			);
		};

		EXPECT_THROW(testProg(), TestException1);
		EXPECT_TRUE(taskRunner.IsTerminated());
	}

	// ===== Ignore exception =====
	{
		bool exceptionCaught = false;

		auto task1 = Threading::MakeLambdaTask(
			threadFunc,
			[]() {},
			[]() {},
			[&exceptionCaught](std::exception_ptr ePtr)
			{
				if (ePtr)
				{
					try
					{
						std::rethrow_exception(ePtr);
					}
					catch (const TestException1&)
					{
						exceptionCaught = true;
					}
				}
			}
		);

		Threading::TaskRunner taskRunner;
		taskRunner.AssignTask(std::move(task1));

		auto testProg = [&taskRunner] ()
		{
			taskRunner.ThreadRunner(
				[]
				(Threading::TaskRunner*, std::unique_ptr<Threading::Task>) ->
					std::unique_ptr<Threading::Task>
				{
					throw TestException2("This should be called");
				}
			);
		};

		EXPECT_THROW(testProg(), TestException2);
		EXPECT_TRUE(exceptionCaught);
		EXPECT_TRUE(taskRunner.IsTerminated());
	}
}


GTEST_TEST(Test_Threading_TaskRunner, TerminateRunningTask)
{
	std::atomic_uint64_t loopCount(0);
	auto threadFunc =
		[&loopCount](const std::atomic_bool& isTerminated)
		{
			while(!isTerminated)
			{
				if (loopCount > 5000)
				{
					throw std::runtime_error("This should not be called");
				}
				++loopCount;
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		};


	{
		auto task1 = Threading::MakeLambdaTask(
			threadFunc,
			[]() {},
			[]() {},
			[](std::exception_ptr ePtr) { std::rethrow_exception(ePtr); }
		);

		Threading::TaskRunner taskRunner;
		taskRunner.AssignTask(std::move(task1));

		auto finishCallback =
			[]
			(Threading::TaskRunner*, std::unique_ptr<Threading::Task>) ->
				std::unique_ptr<Threading::Task>
			{
				return nullptr;
			};

		std::thread testThread(
			[&taskRunner, finishCallback]()
			{
				taskRunner.ThreadRunner(finishCallback);
			}
		);

		// Wait for the task to start running
		while(loopCount == 0) {}

		taskRunner.TerminateTask();

		// wait until the task runner is fully terminated
		while (!taskRunner.IsTerminated()) {}

		testThread.join();

		std::cout << "Loop count: " << loopCount << std::endl;
	}
}
