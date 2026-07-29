#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #pragma comment(lib, "ws2_32.lib")

    typedef SOCKET socket_fd_t;
    #define CLOSE_SOCKET(s) closesocket(s)
    #define VALID_SOCKET(s) ((s) != INVALID_SOCKET)

    static inline void init_network_workers(void) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            fprintf(stderr, "WSAStartup failed: %d\n", result);
            exit(EXIT_FAILURE);
        }
        printf("[Anthill] Winsock initialized. Ants are ready.\n");
    }

    static inline void cleanup_network_workers(void) {
        WSACleanup();
    }

#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>

    typedef int socket_fd_t;
    #define CLOSE_SOCKET(s) close(s)
    #define VALID_SOCKET(s) ((s) >= 0)

    static inline void init_network_workers(void) {
        printf("[Anthill] POSIX network ready. Ants are ready.\n");
    }

    static inline void cleanup_network_workers(void) {
        //
    }

#endif

#endif
