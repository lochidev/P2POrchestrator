#pragma once
#include "Network.h"
#include "MasterControl.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <stdint.h>
class Worker {

public:
	Worker(MasterControl* pM)
		:
		pMaster(pM),
		thread(&Worker::Run_, this)
	{}

	~Worker() {
		Kill();
	}
	Worker(const Worker&) = delete;
	Worker& operator=(const Worker&) = delete;
	Worker(const Worker&&) = delete;
	Worker& operator=(const Worker&&) = delete;
	void StartWork()
	{
		//{
		//	std::lock_guard lk{ mtx };
		working = true;
		//}
		cv.notify_one();
	}
	void RemoveClient(std::shared_ptr<Task> pTask)
	{
		std::lock_guard lk{ mtx }; //deadlocked here, fixed
		removeTasks.emplace_back(pTask);
	}

	void Kill()
	{
		{
			std::lock_guard lk{ mtx };
			dying = true;
		}
		cv.notify_one();
	}
private:
	void ProcessData_()
	{
		while (true)
		{
			auto oTask = pMaster->GetTask();
			if (oTask) {
				auto pTask = *oTask;
				SOCKET cs = pTask->socket;
				int iResult = 0;
				char recvbuf[DEFAULT_BUFLEN] = { 0 };
				int recvbuflen = DEFAULT_BUFLEN;

				iResult = recv(cs, recvbuf, recvbuflen, 0);
				if (iResult >= DEFAULT_BUFLEN) {
					LOG_INFO("Bytes received: " << recvbuf << " size: " << iResult);
					// Ack back to the sender
					if (NotifyClient_(cs, "ack ", DEFAULT_BUFLEN)) {
						//if not client, break connection
						if (recvbuf[0] != 'M') {
							pMaster->RegisterPlayer(pTask, recvbuf[0], recvbuf, this);
						}
						tasks.insert(std::make_pair(pTask, (uint_fast8_t)0));
					}
					else {
						LogAndCloseSocket(pTask);
					}
				}
				else if (iResult == 0) {
					LOG_INFO("Connection closing...\n");
					LogAndCloseSocket(pTask);
				}
				else {
					LOG_INFO("recv failed: ") /*<< WSAGetLastError()*/;
					LogAndCloseSocket(pTask);
				}
			}
			else if (!tasks.empty()) {
				//unsigned int count = 0;
				//sleep for 120 seconds i.e. -> 2 min EDIT: NO SLEEPING, NONONONO
				if (!removeTasks.empty()) {
					for (auto tsk : removeTasks)
					{
						tasks.erase(tsk);
					}
					removeTasks.clear();
				}
				for (auto& pair : tasks)
				{
					auto& pTask = pair.first;
					if (pair.second >= 1000) {
						// register as a client server
						pMaster->RegisterPlayer(pTask, 'M', nullptr, this);
					}
					else if (pair.second >= 3000) {

						// we found no players
						NotifyClient_(pTask->socket, "nf  ", DEFAULT_BUFLEN);
						tasks.erase(pTask);
						LogAndCloseSocket(pTask);
					}
					else if (auto oServer = pMaster->GetServer()) {
						//found server
						auto& pServerTask = *oServer;
						NotifyClient_(pTask->socket, (const char*)&(pServerTask->addr), DEFAULT_BUFLEN);
						tasks.erase(pTask);
						LogAndCloseSocket(pTask);
					}
					else if (auto oServer = pMaster->GetClientAsServer()) {
						auto& pServerTask = *oServer;

						auto pWorker = std::any_cast<Worker*>(pServerTask->pWorker);
						if (pWorker == this) {
							if (auto search = tasks.find(pServerTask->pTask); search != tasks.end()) {
								NotifyClient_(pTask->socket, (const char*)&(pServerTask->pTask->addr), DEFAULT_BUFLEN);
								NotifyClient_(pServerTask->pTask->socket, (const char*)&(pTask->addr), DEFAULT_BUFLEN);
								tasks.erase(pTask);
								tasks.erase(pServerTask->pTask);
								LogAndCloseSocket(pTask);
								LogAndCloseSocket(pServerTask->pTask);
							}
						}
						else if (!IsIpEqual_(pServerTask->pTask.get(), pTask.get())) {
							NotifyClient_(pTask->socket, (const char*)&(pServerTask->pTask->addr), DEFAULT_BUFLEN);
							pWorker->RemoveClient(pTask);
							//goto SOCKETCLOSE;
						}
					}
					else if(pair.second % 1000 == 0) {
						if (!NotifyClient_(pTask->socket, "ack ", DEFAULT_BUFLEN)) {
							tasks.erase(pTask);
							LogAndCloseSocket(pTask);
						}
					}
				}
			}
			else {
				break;
			}
		}
	}

	void Run_()
	{
		std::unique_lock lk{ mtx };
		while (true)
		{
			cv.wait(lk, [this] {return working || dying; });
			if (dying)
			{
				break;
			}
			lk.unlock();
			ProcessData_();
			lk.lock();
			working = false;
			pMaster->SignalDone();
		}
	}
	bool IsIpEqual_(const Task* task1, const Task* task2) {
		const char* p1 = (const char*)&(task1->addr);
		const char* p2 = (const char*)&(task2->addr);
		for (size_t i = 0; i < 4; i++)
		{
			if (*(p1 + i) != *(p2 + i))
				return false;
		}
		return task1->port == task2->port;
	}
	bool NotifyClient_(const SOCKET& ClientSocket, const char* buf, int len) {
		auto iSendResult = send(ClientSocket, buf, len, 0);
		if (SOCKET_ERROR(iSendResult)) {
			LOG_INFO("Could not send buffer");
			return false;
		}
		LOG_INFO("Sent buffer: " << buf << " size: " << iSendResult);
		return true;
	}
	void LogAndCloseSocket(std::shared_ptr<Task> tsk) {
		if (N_SocketClose(tsk->socket)) {
			LOG_INFO("Socket closed: " << "port:" << tsk->port);
			return;
		}
		LOG_INFO("Could not close socket: " << "port:" << tsk->port);
	}
private:
	MasterControl* pMaster;
	std::jthread thread;
	std::condition_variable cv;
	std::mutex mtx;
	std::unordered_map<std::shared_ptr<Task>, uint_fast8_t> tasks;
	std::vector<std::shared_ptr<Task>> removeTasks;
	//shared memory
	std::atomic<bool> working = false;
	bool dying = false;
	Task removeTask = { 0 };
};
