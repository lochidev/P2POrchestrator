#pragma once
#include "Network.h"
#include <thread>
#include <any>
#include <memory>
static inline unsigned int WorkerCount = std::thread::hardware_concurrency() == 0 ? 1 : std::thread::hardware_concurrency();

struct Task
{
    SOCKET socket;
    in_addr addr;
    unsigned short port;
};

struct ClientTask {
    const std::shared_ptr<Task> pTask;
    std::any pWorker; // expecting type Worker
};