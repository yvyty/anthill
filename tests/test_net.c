#include <stdbool.h>
#include <string.h>

#include "net_utils.h"
#include "test_util.h"

/*
 * These tests exercise the connect-based and UDP probes against loopback. They
 * need working sockets but no internet access, so CTest labels them "network"
 * and they can be skipped offline with `ctest -LE network`.
 */

#define PROBE_TIMEOUT_MS 500

static bool reserve_tcp_port(int *port_out)
{
    socket_fd_t sock = socket(AF_INET, SOCK_STREAM, 0);

    if (!VALID_SOCKET(sock)) {
        return false;
    }

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (
        bind(
            sock,
            (const struct sockaddr *)&addr,
            sizeof(addr)
        ) != 0
    ) {
        CLOSE_SOCKET(sock);
        return false;
    }

    anthill_socklen_t length = (anthill_socklen_t)sizeof(addr);

    if (
        getsockname(
            sock,
            (struct sockaddr *)&addr,
            &length
        ) != 0
    ) {
        CLOSE_SOCKET(sock);
        return false;
    }

    *port_out = ntohs(addr.sin_port);

    CLOSE_SOCKET(sock);

    return true;
}

static bool open_tcp_listener(socket_fd_t *sock_out, int *port_out)
{
    socket_fd_t sock = socket(AF_INET, SOCK_STREAM, 0);

    if (!VALID_SOCKET(sock)) {
        return false;
    }

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (
        bind(
            sock,
            (const struct sockaddr *)&addr,
            sizeof(addr)
        ) != 0
    ) {
        CLOSE_SOCKET(sock);
        return false;
    }

    if (listen(sock, 1) != 0) {
        CLOSE_SOCKET(sock);
        return false;
    }

    anthill_socklen_t length = (anthill_socklen_t)sizeof(addr);

    if (
        getsockname(
            sock,
            (struct sockaddr *)&addr,
            &length
        ) != 0
    ) {
        CLOSE_SOCKET(sock);
        return false;
    }

    *sock_out = sock;
    *port_out = ntohs(addr.sin_port);

    return true;
}

static void test_connect_finds_open_port(void)
{
    socket_fd_t listener;
    int port = 0;

    if (!open_tcp_listener(&listener, &port)) {
        TEST_CHECK(false);
        return;
    }

    TEST_CHECK(check_port_open("127.0.0.1", port, PROBE_TIMEOUT_MS));

    CLOSE_SOCKET(listener);
}

static void test_connect_rejects_closed_port(void)
{
    int port = 0;

    if (!reserve_tcp_port(&port)) {
        TEST_CHECK(false);
        return;
    }

    TEST_CHECK(!check_port_open("127.0.0.1", port, PROBE_TIMEOUT_MS));
}

static void test_connect_rejects_unresolvable_host(void)
{
    TEST_CHECK(!check_port_open("", 80, PROBE_TIMEOUT_MS));
    TEST_CHECK(!check_port_open(NULL, 80, PROBE_TIMEOUT_MS));
}

static void test_connect_resolves_hostname(void)
{
    socket_fd_t listener;
    int port = 0;

    if (!open_tcp_listener(&listener, &port)) {
        TEST_CHECK(false);
        return;
    }

    TEST_CHECK(check_port_open("localhost", port, PROBE_TIMEOUT_MS));

    CLOSE_SOCKET(listener);
}

static void test_udp_reports_bound_port_unavailable(void)
{
    socket_fd_t sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (!VALID_SOCKET(sock)) {
        TEST_CHECK(false);
        return;
    }

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (
        bind(
            sock,
            (const struct sockaddr *)&addr,
            sizeof(addr)
        ) != 0
    ) {
        CLOSE_SOCKET(sock);
        TEST_CHECK(false);
        return;
    }

    anthill_socklen_t length = (anthill_socklen_t)sizeof(addr);

    TEST_CHECK(
        getsockname(sock, (struct sockaddr *)&addr, &length) == 0
    );

    int port = ntohs(addr.sin_port);

    TEST_CHECK(!check_udp_availability(port));

    CLOSE_SOCKET(sock);
}

static void test_udp_reports_free_port_available(void)
{
    int port = 0;

    if (!reserve_tcp_port(&port)) {
        TEST_CHECK(false);
        return;
    }

    TEST_CHECK(check_udp_availability(port));
}

static void test_udp_rejects_invalid_ports(void)
{
    TEST_CHECK(!check_udp_availability(0));
    TEST_CHECK(!check_udp_availability(65536));
    TEST_CHECK(!check_udp_availability(-1));
}

int main(void)
{
    init_network_workers(false);

    test_connect_finds_open_port();
    test_connect_rejects_closed_port();
    test_connect_rejects_unresolvable_host();
    test_connect_resolves_hostname();
    test_udp_reports_bound_port_unavailable();
    test_udp_reports_free_port_available();
    test_udp_rejects_invalid_ports();

    cleanup_network_workers();

    return test_summary("test_net");
}
