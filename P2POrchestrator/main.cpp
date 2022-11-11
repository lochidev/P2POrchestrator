// P2POrchestrator.cpp : Defines the entry point for the application.
//

#include "main.h"
#define PORT 5432
#define DEFAULT_BUFLEN 7 // 1st byte: (M)obile or (P)C, 2 and 3: Region ex: 'US', 4 : IP Address 
#define LOG_INFO(i) std::cout << i << std::endl;

int main()
{
	sockInit();
    SOCKET server_fd, ClientSocket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    const char* hello = "Hello from server";

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
        char recvbuf[DEFAULT_BUFLEN] = {0};
        int iResult = 0, iSendResult = 0;
        int recvbuflen = DEFAULT_BUFLEN;
        while(true) {

            iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
            if (iResult == 7) {
                LOG_INFO("Bytes received : " << iResult);
                LOG_INFO(recvbuf);
                // Echo the buffer back to the sender
                iSendResult = send(ClientSocket, "ack", 3, 0);
                if (SOCKET_ERROR(iSendResult)) {
                    LOG_INFO("Could not ack");
                    break;
                }
                std::cout << "Bytes sent: " << iSendResult << std::endl;
            }
            else if (iResult > 7 || iResult < 7) {

            }
            else if (iResult == 0) {
                std::cout << "Connection closing...\n";
                break;
            }
            else {
                std::cout << "recv failed: " /*<< WSAGetLastError()*/;
                break;
            }

        };
        sockClose(ClientSocket);
    }
	sockQuit();
	return 0;
}
