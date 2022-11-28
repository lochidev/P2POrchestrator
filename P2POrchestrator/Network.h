#pragma once
#include <iostream>
#ifdef _WIN32
  /* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0501  /* Windows XP. */
    #endif
    #include <winsock2.h>
    #include <Ws2tcpip.h>
    #define SOCKET_ERROR(skt) (skt) == INVALID_SOCKET
    #pragma comment(lib, "Ws2_32.lib")
#else
    /* Assume that any non-Windows platform uses POSIX-style sockets instead. */
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
    #include <unistd.h> /* Needed for close() */
    typedef int SOCKET;
    #define SOCKET_ERROR(skt) skt < 0
#endif



#define PORT 5432
#define DEFAULT_BUFLEN 4 // 1st byte: (M)obile or (P)C, 2 and 3: Region ex: 'US' 4 : padding for ip size
#ifndef NDEBUG
#define LOG_INFO(i) std::cout << "[t:" << std::this_thread::get_id() << "] " << i << std::endl;
#else
#define LOG_INFO(i);
#endif

int sockInit(void)
{
#ifdef _WIN32
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
    return 0;
#endif
}

int sockQuit(void)
{
#ifdef _WIN32
    return WSACleanup();
#else
    return 0;
#endif
}
int sockClose(SOCKET sock)
{

    int status = 0;

#ifdef _WIN32
    status = shutdown(sock, SD_BOTH);
    if (status == 0) { status = closesocket(sock); }
#else
    status = shutdown(sock, SHUT_RDWR);
    if (status == 0) { status = close(sock); }
#endif

    return status;

}
