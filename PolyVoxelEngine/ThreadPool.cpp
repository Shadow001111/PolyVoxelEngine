#include "ThreadPool.h"
#include <iostream>

void ThreadPool::TaskQueue::getTask(std::function<void()>& task)
{
	std::unique_lock<std::mutex> lock(taskMutex);
	taskAvailableSignal.wait(lock, [this]() { return !tasks.empty() || stopThreads; });
	if (tasks.empty() || stopThreads)
	{
		task = nullptr;
		return;
	}
	task = std::move(tasks.front());
	tasks.pop();
}

void ThreadPool::TaskQueue::waitForCompletion()
{
	std::unique_lock<std::mutex> lock(completionMutex);
	isWaitingForCompletion = true;
	completionSignal.wait(lock, [this]() { return activeTasks == 0; });
	isWaitingForCompletion = false;
}

void ThreadPool::TaskQueue::workDone()
{
	activeTasks--;
	if (activeTasks > 0)
	{
		return;
	}
	std::unique_lock<std::mutex> lock(completionMutex);
	if (isWaitingForCompletion)
	{
		completionSignal.notify_one();
	}
}

void ThreadPool::TaskQueue::stop()
{
	stopThreads = true;
	taskAvailableSignal.notify_all();
}

ThreadPool::Worker::Worker(TaskQueue& taskQueue) : taskQueue(taskQueue)
{
	thread = std::thread([this]() {
		run();
	});
}

void ThreadPool::Worker::run()
{
	std::function<void()> task = nullptr;
	while (true)
	{
		taskQueue.getTask(task);
		if (task == nullptr)
		{
			if (taskQueue.stopThreads)
			{
				break;
			}
			else
			{
				std::cerr << "Thread busy waiting" << std::endl;
			}
		}
		else
		{
			task();
			taskQueue.workDone();
		}
	}
}

void ThreadPool::Worker::joinThread()
{
	thread.join();
}

ThreadPool::ThreadPool(size_t threadCount) : threadCount(threadCount)
{
	workers.reserve(threadCount);
	for (size_t i = 0; i < threadCount; i++)
	{
		workers.emplace_back(taskQueue);
	}
}

ThreadPool::~ThreadPool()
{
	if (!workers.empty())
	{
		destroy();
	}
}

void ThreadPool::waitForCompletion()
{
	taskQueue.waitForCompletion();
}

void ThreadPool::destroy()
{
	taskQueue.stop();
	for (Worker& worker : workers)
	{
		worker.joinThread();
	}
	workers.clear();
}
