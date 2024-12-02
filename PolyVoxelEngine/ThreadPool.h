#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#pragma once
#include <functional>

// thanks to Pezzas Work
namespace ThreadPoolSpace
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
		void addTask(TCallback&& task)
		{
			{
				std::lock_guard<std::mutex> lock(taskMutex);
				tasks.push(std::forward<TCallback>(task));
			}
			activeTasks++;
			taskAvailableSignal.notify_one();
		}

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

	class ThreadPool
	{
		std::vector<Worker> workers;
		size_t threadCount;
		TaskQueue taskQueue;
	public:
		ThreadPool(size_t threadCount);
		~ThreadPool();

		template<typename TCallback>
		void addTask(TCallback&& task)
		{
			taskQueue.addTask(std::forward<TCallback>(task));
		}

		void waitForCompletion();

		template<typename TCallback>
		void distribute(size_t count, TCallback&& task)
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

		void destroy();
	};
}

