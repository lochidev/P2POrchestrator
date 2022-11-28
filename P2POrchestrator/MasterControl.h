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
#include <optional>
#include "Lobby.h"
class MasterControl
{
public:
	MasterControl()
	{}
	void SignalDone()
	{
		bool needsNotification = false;
		++doneCount;
		needsNotification = doneCount == WorkerCount;
		if (needsNotification)
		{
			cv.notify_one();
		}
	}
	void ResetCount() {
		doneCount = 0;
	}
	void RegisterPlayer(const std::shared_ptr<Task>& t, char platform, char* region, std::any worker) {

		/*std::lock_guard lk{ mtx };
		if (lobbies.empty()) {
			Lobby l;
			lobbies.emplace(l);
		}
		else {
			const auto t = lobbies.front();
			lobbies.pop();
			return t;
		}*/
		switch (platform)
		{
		case 'M':
			clients.push(std::make_unique<ClientTask>(t, worker));
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
		tasks.push(std::make_shared<Task>(t));
		//for (const auto& w : workerPtrs)
		//{
		//	w->StartWork();
		//}
	}
	const std::optional<std::shared_ptr<Task>> GetTask()
	{
		std::lock_guard lk{ mtx };
		if (tasks.empty()) { return std::nullopt; }
		else {
			auto t = std::move(tasks.front());
			tasks.pop();
			return t;
		}
	}
	const std::optional<std::shared_ptr<Task>> GetServer()
	{
		std::lock_guard lk{ mtx };
		if (servers.empty()) { return std::nullopt; }
		else {
			auto t = std::move(servers.front());
			servers.pop();
			return t;
		}
	}
	const std::optional<std::unique_ptr<ClientTask>> GetClientAsServer()
	{
		std::lock_guard lk{ mtx };
		if (clients.empty()) { return std::nullopt; }
		else {
			auto t = std::move(clients.front());
			clients.pop();
			return t;
		}
	}
	/*const std::optional<std::shared_ptr<ClientTask>> ForceClientAsServer()
	{
		std::lock_guard lk{ mtx };
		if (clients.empty()) { return std::nullopt; }
		else {
			auto t = std::move(clients.front());
			clients.pop();
			return t;
		}
	}*/
private:
	std::condition_variable cv;
	std::mutex mtx;
	// shared memory
	std::atomic<int> doneCount = 0;
	std::queue<std::shared_ptr<Task>> tasks;
	std::queue<std::shared_ptr<Task>> servers;
	std::queue<std::unique_ptr<ClientTask>> clients;
	//std::queue<Lobby> lobbies;
};