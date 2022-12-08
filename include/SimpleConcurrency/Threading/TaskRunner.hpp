// Copyright (c) 2022 Haofan Zheng
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include "Task.hpp"


#ifndef SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE
namespace SimpleConcurrency
#else
namespace SIMPLECONCURRENCY_CUSTOMIZED_NAMESPACE
#endif
{
namespace Threading
{


class TaskRunner
{
public:
	TaskRunner() :
		m_taskMutex(),
		m_taskCV(),
		m_task(),
		m_isTerminated(false),
		m_isTerminating(false),
		m_isThreadTaskFinished(false)
	{}

	// LCOV_EXCL_START
	virtual ~TaskRunner()
	{
		// terminate the task
		TerminateTask();
	}
	// LCOV_EXCL_STOP


	template<typename _FinishedCallback>
	void ThreadRunner(_FinishedCallback finishCallback)
	{
		while(!m_isTerminating)
		{
			// wait until there is a task to run
			std::unique_lock<std::mutex> lock(m_taskMutex);
			m_taskCV.wait(
				lock,
				[this]() {
					return
						(m_task != nullptr) ||
						m_isTerminating;
				}
			);

			// mutex is locked again here

			if (!m_isTerminating)
			{
				std::unique_ptr<Task> task;
				try
				{
					// It's not terminating, so it must be a task to run
					RunThreadTask();

					// this task is finished, notify the caller,
					// and try to get a new task
					task = finishCallback(
						this, std::move(m_task)
					);
				}
				catch(...)
				{
					// `RunThreadTask` should handle the exception already,
					// so we don't handle exception here

					// it is going to throw, so it must be terminating
					m_isTerminated = true;
					throw;
				}

				// reset the task
				ResetTaskNonLocking();

				// assign the new task
				// if the task is nullptr, we will try to wait for a new task
				// in the next loop
				// otherwise, instead of waiting, we will run the new task
				// in the next loop
				m_task = std::move(task);
			}

			// back to waiting for new task
		}

		// exit the while loop, so it must be terminating
		m_isTerminated = true;
	}


	void TerminateTask()
	{
		// first let other thread know that it's terminating
		m_isTerminating = true;
		// in case the other thread is waiting for a task, notify it
		m_taskCV.notify_all();
		// in case the thread is already running the task, terminate it
		if (m_task)
		{
			m_task->Terminate();
		}
	}


	void AssignTask(std::unique_ptr<Task> task)
	{
		std::lock_guard<std::mutex> lock(m_taskMutex);
		// the mutex is locked by this thread (i.e., not locked by other thread)
		// so the other thread must be waiting for a task

		// assign the task
		m_task = std::move(task);

		// notify the other thread that there is a task to run
		m_taskCV.notify_all();
	}


	bool IsTerminated() const
	{
		return m_isTerminated;
	}


protected:


	void ResetTaskNonLocking()
	{
		m_task.reset();
		m_isThreadTaskFinished = false;
	}


	void RunThreadTask()
	{
		if (m_task)
		{
			try
			{
				m_task->Run();
			}
			catch(...)
			{
				m_isThreadTaskFinished = true;

				m_task->OnException(std::current_exception());
			}
		}
		m_isThreadTaskFinished = true;
	}


private:

	mutable std::mutex m_taskMutex;
	mutable std::condition_variable m_taskCV;
	std::unique_ptr<Task> m_task;
	std::atomic_bool m_isTerminated;
	std::atomic_bool m_isTerminating;
	std::atomic_bool m_isThreadTaskFinished;

}; // class TaskRunner


} // namespace Threading
} // namespace SimpleConcurrency
