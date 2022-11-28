#include "Network.h"
#include "MasterControl.h"
#include "Worker.h"
#include "Task.h"
#include <ranges>
#include <vector>
#include <memory>
int main()
{
	try
	{
		sockInit();
		SOCKET server_fd, ClientSocket, valread;
		struct sockaddr_in address;
		int opt = 1;
		int addrlen = sizeof(address);
		//const char* hello = "Hello from server";

		// Creating socket file descriptor
		if (SOCKET_ERROR((server_fd = socket(AF_INET, SOCK_STREAM, 0)))) {
			perror("socket failed");
			exit(EXIT_FAILURE);
		}

		//// Forcefully attaching socket to the port 8080
		//if (setsockopt(server_fd, SOL_SOCKET,
		//    SO_REUSEADDR | SO_REUSEPORT, &opt,
		//    sizeof(opt))) {
		//    perror("setsockopt");
		//    exit(EXIT_FAILURE);
		//}
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(PORT);

		// Forcefully attaching socket to the port 8080
		if (SOCKET_ERROR(bind(server_fd, (struct sockaddr*)&address,
			sizeof(address)))) {
			perror("bind failed");
			exit(EXIT_FAILURE);
		}
		MasterControl master;
		std::vector<std::unique_ptr<Worker>> workerPtrs(WorkerCount);
		std::ranges::generate(workerPtrs, [pMctrl = &master] { return std::make_unique<Worker>(pMctrl); });
		while (true) {
			if (SOCKET_ERROR(listen(server_fd, 3))) {
				perror("listen");
				exit(EXIT_FAILURE);
			}
			ClientSocket
				= accept(server_fd, (struct sockaddr*)&address,
					(socklen_t*)&addrlen);
			if (SOCKET_ERROR(ClientSocket)) {
				perror("accept");
				exit(EXIT_FAILURE);
			}
			struct in_addr ipAddr = address.sin_addr;
			unsigned short port = address.sin_port;
			//char* ipStr = inet_ntoa(ipAddr);;
			master.SetTask({ ClientSocket, ipAddr, port });
			master.ResetCount();
			for (auto& w : workerPtrs)
			{
				w->StartWork(); // work available
			}
		}
		master.WaitForAllDone();
		sockQuit();
		return 0;
	}
	catch (const std::exception& e)
	{
		LOG_INFO(e.what());
	}
	
}
