#pragma once
#include "Network.h"
#include "Task.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <any>
class MasterControl
{
public:
	MasterControl() 
	{}
	void SignalDone()
	{
		bool needsNotification = false;
		{
			std::lock_guard lk{ mtx };
			++doneCount;
			needsNotification = doneCount == WorkerCount;
		}
		if (needsNotification)
		{
			cv.notify_one();
		}
	}
	void RegisterPlayer(const Task& t, char platform, char* region, std::any worker) {
		std::lock_guard lk{ mtx };

		switch (platform)
		{
		case 'M':
			clients.push({ t, worker });
			break;
		case 'P':
			servers.push(t);
			break;
		default:
			break;
		}
	}

	void WaitForAllDone()
	{
		std::unique_lock<std::mutex> lk;
		cv.wait(lk, [this] { return doneCount == WorkerCount; });
		doneCount = 0;
	}
	void SetTask(const Task& t)
	{
		std::lock_guard lk{ mtx };
		tasks.push(t);
		//for (const auto& w : workerPtrs)
		//{
		//	w->StartWork();
		//}
	}
	const Task* GetTask()
	{
		std::lock_guard lk{ mtx };
		if (tasks.empty()) { return nullptr; }
		else {
			const Task& t = tasks.front();
			tasks.pop();
			return &t;
		}
	}
	const Task* GetServer()
	{
		std::lock_guard lk{ mtx };
		if (servers.empty()) { return nullptr; }
		else {
			const Task& t = servers.front();
			servers.pop();
			return &t;
		}
	}
	const ClientTask* ForceClientAsServer()
	{
		std::lock_guard lk{ mtx };
		if (clients.empty()) { return nullptr; }
		else {
			const ClientTask& t = clients.front();
			clients.pop();
			return &t;
		}
	}
private:
	std::condition_variable cv;
	std::mutex mtx;
	// shared memory
	int doneCount = 0;
	std::queue<Task> tasks;
	std::queue<Task> servers;
	std::queue<ClientTask> clients;
};