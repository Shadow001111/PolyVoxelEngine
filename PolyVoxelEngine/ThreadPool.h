#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>

// thanks to Pezzas Work

class ThreadPool
{
	class TaskQueue
	{
		bool isWaitingForCompletion = false;
		std::condition_variable completionSignal;
		std::queue<std::function<void()>> tasks;
		std::mutex taskMutex;
		std::mutex completionMutex;
		std::atomic<uint8_t> activeTasks = 0;
	public:
		std::condition_variable taskAvailableSignal;
		bool stopThreads = false;

		template<typename TCallback>
		void addTask(TCallback&& task);

		template<typename TCallbackContainer>
		void addTasks(const TCallbackContainer& taskContainer);

		void getTask(std::function<void()>& task);

		void waitForCompletion();

		void workDone();

		void stop();
	};

	class Worker
	{
		std::thread thread;
		TaskQueue& taskQueue;
	public:
		Worker() = default;

		Worker(TaskQueue& taskQueue);

		void run();

		void joinThread();
	};

	std::vector<Worker> workers;
	size_t threadCount;
	TaskQueue taskQueue;
public:
	ThreadPool(size_t threadCount);
	~ThreadPool();

	template<typename TCallback>
	void addTask(TCallback&& task);

	template<typename TCallbackContainer>
	void addTasks(const TCallbackContainer& taskContainer);

	void waitForCompletion();

	template<typename TCallback>
	void distribute(size_t count, TCallback&& task);

	void destroy();
};

template<typename TCallback>
inline void ThreadPool::TaskQueue::addTask(TCallback&& task)
{
	{
		std::lock_guard<std::mutex> lock(taskMutex);
		tasks.push(std::forward<TCallback>(task));
	}
	activeTasks++;
	taskAvailableSignal.notify_one();
}

template<typename TCallbackContainer>
inline void ThreadPool::TaskQueue::addTasks(const TCallbackContainer& taskContainer)
{
	{
		std::lock_guard<std::mutex> lock(taskMutex);
		for (const auto& task : taskContainer) 
		{
			tasks.push(task);
			activeTasks++;
		}
	}
	taskAvailableSignal.notify_all();
}

template<typename TCallback>
inline void ThreadPool::addTask(TCallback&& task)
{
	taskQueue.addTask(std::forward<TCallback>(task));
}

template<typename TCallbackContainer>
inline void ThreadPool::addTasks(const TCallbackContainer& taskContainer)
{
	taskQueue.addTasks(taskContainer);
}

template<typename TCallback>
inline void ThreadPool::distribute(size_t count, TCallback&& task)
{
	size_t batchSize = count / threadCount;
	for (size_t i = 0; i < threadCount; i++)
	{
		size_t start = i * batchSize;
		size_t end;
		if (i == threadCount - 1)
		{
			end = count;
		}
		else
		{
			end = start + batchSize;
		}
		addTask([&task, start, end]() { task(start, end); });
	}
}