#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <process.h>

    /*
     * Networking
     */

    typedef SOCKET socket_fd_t;

    typedef int anthill_socklen_t;

    #define CLOSE_SOCKET(s) closesocket(s)
    #define VALID_SOCKET(s) ((s) != INVALID_SOCKET)

    static inline bool anthill_set_nonblocking(socket_fd_t sock)
    {
        u_long mode = 1;

        return ioctlsocket(sock, FIONBIO, &mode) == 0;
    }

    static inline int anthill_socket_error(void)
    {
        return WSAGetLastError();
    }

    static inline bool anthill_socket_in_progress(int error)
    {
        return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
    }

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

    static inline void init_network_workers(bool verbose)
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

        if (verbose) {
            fprintf(
                stderr,
                "[Anthill] Winsock initialized. Ants are ready.\n"
            );
        }
    }

    static inline void cleanup_network_workers(void)
    {
        WSACleanup();
    }

#else

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <pthread.h>

    /*
     * Networking
     */

    typedef int socket_fd_t;

    typedef socklen_t anthill_socklen_t;

    #define CLOSE_SOCKET(s) close(s)
    #define VALID_SOCKET(s) ((s) >= 0)

    static inline bool anthill_set_nonblocking(socket_fd_t sock)
    {
        int flags = fcntl(sock, F_GETFL, 0);

        if (flags < 0) {
            return false;
        }

        return fcntl(sock, F_SETFL, flags | O_NONBLOCK) == 0;
    }

    static inline int anthill_socket_error(void)
    {
        return errno;
    }

    static inline bool anthill_socket_in_progress(int error)
    {
        return error == EINPROGRESS;
    }

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

    static inline void init_network_workers(bool verbose)
    {
        if (!verbose) {
            return;
        }

        fprintf(
            stderr,
            "[Anthill] POSIX network ready. Ants are ready.\n"
        );
    }

    static inline void cleanup_network_workers(void)
    {
    }

#endif

/*
 * Target probing
 *
 * These helpers are shared by both platforms and rely on the small
 * anthill_set_nonblocking / anthill_socket_error wrappers defined above.
 */

/*
 * Resolves `host` (numeric IPv4 or DNS name) into an IPv4 `sockaddr_in`.
 * Returns false when the name is empty or cannot be resolved.
 */
static inline bool anthill_resolve_host(
    const char *host,
    struct sockaddr_in *out
)
{
    if (host == NULL || *host == '\0' || out == NULL) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;

    if (inet_pton(AF_INET, host, &out->sin_addr) == 1) {
        return true;
    }

    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *result = NULL;

    if (getaddrinfo(host, NULL, &hints, &result) != 0 || result == NULL) {
        return false;
    }

    const struct sockaddr_in *resolved =
        (const struct sockaddr_in *)result->ai_addr;

    out->sin_addr = resolved->sin_addr;

    freeaddrinfo(result);

    return true;
}

/*
 * True only for the exact localhost address 127.0.0.1, which stays on the
 * local bind-based probe. Anything else (including other 127/8 addresses)
 * switches to the remote connect-based probe so the requested host is honored.
 */
static inline bool anthill_is_localhost_addr(const struct sockaddr_in *addr)
{
    if (addr == NULL) {
        return false;
    }

    return addr->sin_addr.s_addr == htonl(INADDR_LOOPBACK);
}

/*
 * Non-blocking connect() with a bounded wait. Returns true only when the TCP
 * handshake actually completes, so a filtered or closed port reports within
 * `timeout_ms` instead of waiting on the OS default.
 */
static inline bool check_port_open_addr(
    const struct sockaddr_in *target,
    int port,
    int timeout_ms
)
{
    if (target == NULL) {
        return false;
    }

    socket_fd_t sock = socket(AF_INET, SOCK_STREAM, 0);

    if (!VALID_SOCKET(sock)) {
        return false;
    }

    if (!anthill_set_nonblocking(sock)) {
        CLOSE_SOCKET(sock);
        return false;
    }

    struct sockaddr_in addr = *target;

    addr.sin_port = htons((unsigned short)port);

    int result = connect(
        sock,
        (const struct sockaddr *)&addr,
        sizeof(addr)
    );

    bool connected = false;

    if (result == 0) {
        connected = true;
    } else if (anthill_socket_in_progress(anthill_socket_error())) {
        fd_set writable;

        FD_ZERO(&writable);
        FD_SET(sock, &writable);

        struct timeval timeout;

        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        if (
            select(
                (int)(sock + 1),
                NULL,
                &writable,
                NULL,
                &timeout
            ) > 0
        ) {
            int socket_error = 0;
            anthill_socklen_t error_length =
                (anthill_socklen_t)sizeof(socket_error);

            if (
                getsockopt(
                    sock,
                    SOL_SOCKET,
                    SO_ERROR,
                    (char *)&socket_error,
                    &error_length
                ) == 0
            ) {
                connected = socket_error == 0;
            }
        }
    }

    CLOSE_SOCKET(sock);

    return connected;
}

/*
 * Convenience form that resolves `host` on every call. Callers probing many
 * ports should resolve once with anthill_resolve_host and use
 * check_port_open_addr instead.
 */
static inline bool check_port_open(
    const char *host,
    int port,
    int timeout_ms
)
{
    struct sockaddr_in target;

    if (!anthill_resolve_host(host, &target)) {
        return false;
    }

    return check_port_open_addr(&target, port, timeout_ms);
}

/*
 * UDP has no handshake, so "available" means the local box can bind the port
 * (SOCK_DGRAM). This mirrors the TCP bind-based probe and is intentionally
 * local-only: remote UDP reachability cannot be established reliably.
 */
static inline bool check_udp_availability(int port)
{
    if (port <= 0 || port > 65535) {
        return false;
    }

    socket_fd_t sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (!VALID_SOCKET(sock)) {
        return false;
    }

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int result = bind(
        sock,
        (const struct sockaddr *)&addr,
        sizeof(addr)
    );

    CLOSE_SOCKET(sock);

    return result == 0;
}

#endif
