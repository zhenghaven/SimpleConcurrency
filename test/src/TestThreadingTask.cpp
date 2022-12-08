// Copyright (c) 2022 Haofan Zheng
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <gtest/gtest.h>

#ifdef _MSC_VER
#include <windows.h>
#endif // _MSC_VER
#include <SimpleConcurrency/Threading/LambdaTask.hpp>


namespace SimpleConcurrency_Test
{
	extern size_t g_numOfTestFile;
}


#ifndef SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE
using namespace SimpleConcurrency;
#else
using namespace SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE;
#endif


GTEST_TEST(Test_Threading_Task, CountTestFile)
{
	static auto tmp = ++SimpleConcurrency_Test::g_numOfTestFile;
	(void)tmp;
}

GTEST_TEST(Test_Threading_Task, LambdaTask)
{
	{
		std::string testStr1;
		std::string testStr2;
		std::string testStr3;

		std::string expVal = "Hello";

		auto threadFunc =
			[&testStr1, expVal](const std::atomic_bool& isTerminated)
			{
				if (!isTerminated)
				{
					testStr1 = expVal;
				}
			};

		auto task1 = Threading::MakeLambdaTask(
			threadFunc
		);
		task1->Run();
		EXPECT_EQ(testStr1, expVal);

		testStr1.clear();

		auto finishFunc =
			[&testStr2, expVal]()
			{
				testStr2 = expVal;
			};

		auto task2 = Threading::MakeLambdaTask(
			threadFunc,
			finishFunc
		);
		task2->Run();
		task2->Finishing();
		EXPECT_EQ(testStr1, expVal);
		EXPECT_EQ(testStr2, expVal);

		testStr1.clear();
		testStr2.clear();

		auto terminateFunc =
			[&testStr3, expVal]()
			{
				testStr3 = expVal;
			};

		auto task3 = Threading::MakeLambdaTask(
			threadFunc,
			finishFunc,
			terminateFunc
		);
		task3->Run();
		task3->Finishing();
		task3->Terminate();
		EXPECT_EQ(testStr1, expVal);
		EXPECT_EQ(testStr2, expVal);
		EXPECT_EQ(testStr3, expVal);

		testStr1.clear();
		testStr2.clear();
		testStr3.clear();

		auto task4 = Threading::MakeLambdaTask(
			threadFunc,
			finishFunc,
			terminateFunc
		);
		task4->Terminate();
		task4->Run();
		EXPECT_EQ(testStr1, std::string());
	}
}
