#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")

    typedef SOCKET socket_fd_t;
    #define CLOSE_SOCKET(s) closesocket(s)
    #define VALID_SOCKET(s) ((s) != INVALID_SOCKET)

    typedef HANDLE anthill_thread_t;
    typedef CRITICAL_SECTION anthill_mutex_t;
    #define ANTHILL_THREAD_FUNC DWORD WINAPI
    #define ANTHILL_THREAD_RETURN DWORD

    static inline void anthill_mutex_init(anthill_mutex_t *m) { InitilizeCriticalSection(m); }
    static inline void anthill_mutex_lock(anthill_mutex_t *m) { EnterCriticalSection(m); }
    static inline void anthill_mutex_unlock(anthill_mutex_t *m) { LeaveCriticalSection(m); }
    static inline void anthill_mutex_destroy(anthill_mutex_t *m) { DeleteCriticalSection(m); }

    static inline int anthill_thread_create(anthill_thread_t *t, ANTHILL_THREAD_FUNC (*func)(void*), void *arg){
        *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
        return (*t == NULL) ? 1 : 0;
    }

    static inline void anthill_thread_join(anthill_thread_t t) {
        WaitForSingleObject(t, INFINITE);
        CloseHandle(t);
    }

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
    #include <pthread.h>

    typedef int socket_fd_t;
    #define CLOSE_SOCKET(s) close(s)
    #define VALID_SOCKET(s) ((s) >= 0)

    typedef pthread_t anthill_thread_t;
    typedef pthread_mutex_t anthill_mutex_t;
    #define ANTHILL_THREAD_FUNC void*
    #define ANTHILL_THREAD_RETURN void*

    static inline void anthill_mutex_init(anthill_mutex_t *m) { pthread_mutex_init(m, NULL); }
    static inline void anthill_mutex_lock(anthill_mutex_t *m) { pthread_mutex_lock(m); }
    static inline void anthill_mutex_unlock(anthill_mutex_t *m) { pthread_mutex_unlock(m); }
    static inline void anthill_mutex_destroy(anthill_mutex_t *m) { pthread_mutex_destroy(m); }

    static inline int anthill_thread_create(anthill_thread_t *t, ANTHILL_THREAD_FUNC (*func)(void*), void *arg) {
        return pthread_create(t, NULL, func, arg);
    }

    static inline void anthill_thread_join(anthill_thread_t t) { pthread_join(t, NULL); }
    static inline void init_network_workers(void) {
        printf("[Anthill] POSIX network ready. Ants are ready.\n");
    }

    static inline void cleanup_network_workers(void) {}

#endif

#endif
