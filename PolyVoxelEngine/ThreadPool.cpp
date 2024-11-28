#include "ThreadPool.h"
#include <iostream>

void ThreadPoolSpace::TaskQueue::getTask(std::function<void()>& task)
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

void ThreadPoolSpace::TaskQueue::waitForCompletion()
{
	std::unique_lock<std::mutex> lock(completionMutex);
	isWaitingForCompletion = true;
	completionSignal.wait(lock, [this]() { return activeTasks == 0; });
	isWaitingForCompletion = false;
}

void ThreadPoolSpace::TaskQueue::workDone()
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

void ThreadPoolSpace::TaskQueue::stop()
{
	stopThreads = true;
	taskAvailableSignal.notify_all();
}

ThreadPoolSpace::Worker::Worker(TaskQueue& taskQueue) : taskQueue(taskQueue)
{
	thread = std::thread([this]() {
		run();
	});
}

void ThreadPoolSpace::Worker::run()
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
		}
		else
		{
			task();
			taskQueue.workDone();
		}
	}
}

void ThreadPoolSpace::Worker::stop()
{
	thread.join();
}

ThreadPoolSpace::ThreadPool::ThreadPool(size_t threadCount) : threadCount(threadCount)
{
	workers.reserve(threadCount);
	for (size_t i = 0; i < threadCount; i++)
	{
		workers.emplace_back(taskQueue);
	}
}

ThreadPoolSpace::ThreadPool::~ThreadPool()
{
	taskQueue.stop();
	for (Worker& worker : workers)
	{
		worker.stop();
	}
}

void ThreadPoolSpace::ThreadPool::waitForCompletion()
{
	taskQueue.waitForCompletion();
}
