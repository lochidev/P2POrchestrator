#pragma once
#include "Network.h"
#include "MasterControl.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>



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
		{
			std::lock_guard lk{ mtx };
			working = true;
		}
		cv.notify_one();
	}
	void AddClient(const Task* tsk)
	{
		std::lock_guard lk{ mtx };
		clientTask = tsk;
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

		while (auto pTask = pMaster->GetTask())
		{
			SOCKET ClientSocket = pTask->socket;
			int iResult = 0;
			char recvbuf[DEFAULT_BUFLEN] = { 0 };
			int recvbuflen = DEFAULT_BUFLEN;

			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult == DEFAULT_BUFLEN) {
				LOG_INFO("Bytes received : " << iResult);
				LOG_INFO(recvbuf);
				// Ack back to the sender
				pMaster->RegisterPlayer((Task&)pTask, recvbuf[0], recvbuf, this);
				if (!NotifyClient_(ClientSocket, "ack", 3)) {
					goto SOCKETCLOSE; // yes ikik GOTO but it's convenient asf
				}
				//if not client, break connection
				if (recvbuf[0] != 'M') {
					goto SOCKETCLOSE;
				}
				unsigned int count = 0;
				//sleep for 120 seconds i.e. -> 2 min
				while (count < 12) { 
					if (clientTask != nullptr) {
						NotifyClient_(ClientSocket, (const char*)&(clientTask->addr), 32);
						clientTask = nullptr;
						goto SOCKETCLOSE;
						break;
					}
					else if (auto pServerTask = pMaster->GetServer()) {
						//found server
						NotifyClient_(ClientSocket, (const char*)&(pServerTask->addr), 32);
						goto SOCKETCLOSE;
						break;
					}
					else {
						if (!NotifyClient_(ClientSocket, "ack", 3)) {
							goto SOCKETCLOSE;
							break;
						}
						std::this_thread::sleep_for(std::chrono::seconds(10));
					}
				}
				//cannot talk to this client anymore, so get a new one
				

				// we found no server, force client 
				count = 0;
				while (count < 10) {
					if (clientTask != nullptr) {
						NotifyClient_(ClientSocket, (const char*)&(clientTask->addr), 32);
						clientTask = nullptr;
						goto SOCKETCLOSE;
						break;
					}
					else if (auto pServerTask = pMaster->GetServer()) {
						//found server
						NotifyClient_(ClientSocket, (const char*)&(pServerTask->addr), 32);
						goto SOCKETCLOSE;
						break;
					}
					else if (auto pServerTask = pMaster->ForceClientAsServer()) {
						auto pWorker = std::any_cast<Worker*>(pServerTask->pWorker);
						if (pWorker != this && !IsIpEqual_(pServerTask->t, (Task&)pTask)) {
							NotifyClient_(ClientSocket, (const char*)&(pServerTask->t.addr), 32);
							pWorker->AddClient(pTask);
							goto SOCKETCLOSE;
							break;
						}
					}
					else {
						if (!NotifyClient_(ClientSocket, "ack", 3)) {
							goto SOCKETCLOSE;
							break;
						}
						std::this_thread::sleep_for(std::chrono::seconds(10));
					}
				}
				// we found no players
				NotifyClient_(ClientSocket, "nf", 2);
			}
			else if (iResult > 7 || iResult < 7) {

			}
			else if (iResult == 0) {
				LOG_INFO("Connection closing...\n");
			}
			else {
				LOG_INFO("recv failed: ") /*<< WSAGetLastError()*/;
			}
SOCKETCLOSE:
			sockClose(ClientSocket);
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
			ProcessData_();

			working = false;
			pMaster->SignalDone();
		}
	}
	bool IsIpEqual_(const Task& task1, const Task& task2) {
		const char* p1 = (const char*) & task1.addr;
		const char* p2 = (const char*) & task2.addr;
		for (size_t i = 0; i < 4; i++)
		{
			if (*(p1 + i) != *(p2 + i))
				return false;
		}
		return task1.port == task2.port;
	}
	bool NotifyClient_(const SOCKET& ClientSocket, const char* buf, int len) {
		auto iSendResult = send(ClientSocket, buf, len, 0);
		if (SOCKET_ERROR(iSendResult)) {
			LOG_INFO("Could not ack");
			return false;
		}
		LOG_INFO("Bytes sent: " << iSendResult);
		return true;
	}
private:
	MasterControl* pMaster;
	std::jthread thread;
	std::condition_variable cv;
	std::mutex mtx;
	//shared memory
	bool working = false;
	bool dying = false;
	const Task* clientTask = nullptr;
};
