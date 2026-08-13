#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <process.h>

    /*
     * Networking
     */

    typedef SOCKET socket_fd_t;

    #define CLOSE_SOCKET(s) closesocket(s)
    #define VALID_SOCKET(s) ((s) != INVALID_SOCKET)

    /*
     * Threading
     */

    typedef HANDLE anthill_thread_t;

    typedef unsigned (__stdcall *anthill_thread_func_t)(void *);

    #define ANTHILL_THREAD_FUNC unsigned __stdcall
    #define ANTHILL_THREAD_RETURN unsigned

    /*
     * Mutex
     */

    typedef CRITICAL_SECTION anthill_mutex_t;

    static inline void anthill_mutex_init(anthill_mutex_t *m)
    {
        InitializeCriticalSection(m);
    }

    static inline void anthill_mutex_lock(anthill_mutex_t *m)
    {
        EnterCriticalSection(m);
    }

    static inline void anthill_mutex_unlock(anthill_mutex_t *m)
    {
        LeaveCriticalSection(m);
    }

    static inline void anthill_mutex_destroy(anthill_mutex_t *m)
    {
        DeleteCriticalSection(m);
    }

    /*
     * Thread creation
     */

    static inline int anthill_thread_create(
        anthill_thread_t *t,
        anthill_thread_func_t func,
        void *arg
    )
    {
        uintptr_t handle = _beginthreadex(
            NULL,
            0,
            func,
            arg,
            0,
            NULL
        );

        if (handle == 0) {
            *t = NULL;
            return 1;
        }

        *t = (HANDLE)handle;

        return 0;
    }

    static inline void anthill_thread_join(anthill_thread_t t)
    {
        WaitForSingleObject(t, INFINITE);
        CloseHandle(t);
    }

    /*
     * Winsock lifecycle
     */

    static inline void init_network_workers(void)
    {
        WSADATA wsa_data;

        int result = WSAStartup(
            MAKEWORD(2, 2),
            &wsa_data
        );

        if (result != 0) {
            fprintf(
                stderr,
                "WSAStartup failed: %d\n",
                result
            );

            exit(EXIT_FAILURE);
        }

        printf(
            "[Anthill] Winsock initialized. Ants are ready.\n"
        );
    }

    static inline void cleanup_network_workers(void)
    {
        WSACleanup();
    }

#else

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <pthread.h>

    /*
     * Networking
     */

    typedef int socket_fd_t;

    #define CLOSE_SOCKET(s) close(s)
    #define VALID_SOCKET(s) ((s) >= 0)

    /*
     * Threading
     */

    typedef pthread_t anthill_thread_t;

    typedef void *(*anthill_thread_func_t)(void *);

    #define ANTHILL_THREAD_FUNC void *
    #define ANTHILL_THREAD_RETURN void *

    /*
     * Mutex
     */

    typedef pthread_mutex_t anthill_mutex_t;

    static inline void anthill_mutex_init(anthill_mutex_t *m)
    {
        pthread_mutex_init(m, NULL);
    }

    static inline void anthill_mutex_lock(anthill_mutex_t *m)
    {
        pthread_mutex_lock(m);
    }

    static inline void anthill_mutex_unlock(anthill_mutex_t *m)
    {
        pthread_mutex_unlock(m);
    }

    static inline void anthill_mutex_destroy(anthill_mutex_t *m)
    {
        pthread_mutex_destroy(m);
    }

    /*
     * Thread creation
     */

    static inline int anthill_thread_create(
        anthill_thread_t *t,
        anthill_thread_func_t func,
        void *arg
    )
    {
        return pthread_create(
            t,
            NULL,
            func,
            arg
        );
    }

    static inline void anthill_thread_join(anthill_thread_t t)
    {
        pthread_join(t, NULL);
    }

    /*
     * POSIX networking requires no explicit initialization.
     */

    static inline void init_network_workers(void)
    {
        printf(
            "[Anthill] POSIX network ready. Ants are ready.\n"
        );
    }

    static inline void cleanup_network_workers(void)
    {
    }

#endif

#endif
