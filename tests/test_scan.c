#include <stdbool.h>
#include <string.h>

#include "scan.h"
#include "test_util.h"

#define CAPTURE_SIZE 512

static bool stub_even_ports_available(int port)
{
    return port == 8080 || port == 8082 || port == 8084;
}

static bool stub_no_ports_available(int port)
{
    (void)port;

    return false;
}

static bool stub_always_available(int port)
{
    (void)port;

    return true;
}

static bool stub_targeted_ports(int port)
{
    return port == 80 || port == 443;
}

static size_t capture_list(const port_list_t *list, char *buffer, size_t size)
{
    FILE *sink = tmpfile();

    if (sink == NULL) {
        return (size_t)-1;
    }

    print_port_list(sink, list);

    if (fseek(sink, 0, SEEK_SET) != 0) {
        fclose(sink);
        return (size_t)-1;
    }

    size_t read = fread(buffer, 1, size - 1, sink);

    buffer[read] = '\0';

    fclose(sink);

    return read;
}

static void test_collects_available_ports(void)
{
    port_list_t list;

    port_list_init(&list);

    int collected = scan_collect_available(
        8080,
        8085,
        NULL,
        NULL,
        stub_even_ports_available,
        &list
    );

    TEST_CHECK(collected == 3);
    TEST_CHECK(list.count == 3);
    TEST_CHECK(list.ports[0] == 8080);
    TEST_CHECK(list.ports[1] == 8082);
    TEST_CHECK(list.ports[2] == 8084);

    port_list_free(&list);
}

static void test_prints_stubbed_list(void)
{
    port_list_t list;
    char buffer[CAPTURE_SIZE];

    port_list_init(&list);

    scan_collect_available(
        8080,
        8085,
        NULL,
        NULL,
        stub_even_ports_available,
        &list
    );

    TEST_CHECK(
        capture_list(&list, buffer, sizeof(buffer)) ==
        strlen("Available ports (3):\n    8080\n    8082\n    8084\n")
    );
    TEST_CHECK(
        strcmp(
            buffer,
            "Available ports (3):\n    8080\n    8082\n    8084\n"
        ) == 0
    );

    port_list_free(&list);
}

static void test_empty_result_prints_nothing(void)
{
    port_list_t list;
    char buffer[CAPTURE_SIZE];

    port_list_init(&list);

    int collected = scan_collect_available(
        8080,
        8085,
        NULL,
        NULL,
        stub_no_ports_available,
        &list
    );

    TEST_CHECK(collected == 0);
    TEST_CHECK(list.count == 0);

    buffer[0] = 'x';
    TEST_CHECK(capture_list(&list, buffer, sizeof(buffer)) == 0);
    TEST_CHECK(buffer[0] == '\0');

    port_list_free(&list);
}

static void test_include_filter(void)
{
    port_set_t include_ports;
    port_list_t list;

    port_set_init(&include_ports);
    port_set_add(&include_ports, 8082);

    port_list_init(&list);

    int collected = scan_collect_available(
        8080,
        8085,
        &include_ports,
        NULL,
        stub_even_ports_available,
        &list
    );

    TEST_CHECK(collected == 1);
    TEST_CHECK(list.count == 1);
    TEST_CHECK(list.ports[0] == 8082);

    port_list_free(&list);
}

static void test_include_list_skips_ports_outside_list(void)
{
    port_set_t include_ports;
    port_list_t list;

    port_set_init(&include_ports);
    port_set_add(&include_ports, 80);
    port_set_add(&include_ports, 443);

    port_list_init(&list);

    int collected = scan_collect_available(
        1,
        65535,
        &include_ports,
        NULL,
        stub_targeted_ports,
        &list
    );

    TEST_CHECK(collected == 2);
    TEST_CHECK(list.count == 2);
    TEST_CHECK(list.ports[0] == 80);
    TEST_CHECK(list.ports[1] == 443);

    port_list_free(&list);
}

static void test_exclude_filter(void)
{
    port_set_t exclude_ports;
    port_list_t list;

    port_set_init(&exclude_ports);
    port_set_add(&exclude_ports, 8082);

    port_list_init(&list);

    int collected = scan_collect_available(
        8080,
        8085,
        NULL,
        &exclude_ports,
        stub_even_ports_available,
        &list
    );

    TEST_CHECK(collected == 2);
    TEST_CHECK(list.ports[0] == 8080);
    TEST_CHECK(list.ports[1] == 8084);

    port_list_free(&list);
}

static void test_exclude_wins_over_include(void)
{
    port_set_t include_ports;
    port_set_t exclude_ports;
    port_list_t list;

    port_set_init(&include_ports);
    port_set_add_range(&include_ports, 8080, 8084);

    port_set_init(&exclude_ports);
    port_set_add(&exclude_ports, 8082);

    port_list_init(&list);

    int collected = scan_collect_available(
        8080,
        8085,
        &include_ports,
        &exclude_ports,
        stub_always_available,
        &list
    );

    TEST_CHECK(collected == 4);
    TEST_CHECK(list.count == 4);
    TEST_CHECK(list.ports[0] == 8080);
    TEST_CHECK(list.ports[1] == 8081);
    TEST_CHECK(list.ports[2] == 8083);
    TEST_CHECK(list.ports[3] == 8084);

    port_list_free(&list);
}

static void test_bad_arguments(void)
{
    port_list_t list;
    char buffer[CAPTURE_SIZE];

    port_list_init(&list);

    TEST_CHECK(
        scan_collect_available(
            80,
            90,
            NULL,
            NULL,
            NULL,
            &list
        ) == -1
    );

    TEST_CHECK(
        scan_collect_available(
            80,
            90,
            NULL,
            NULL,
            stub_always_available,
            NULL
        ) == -1
    );

    buffer[0] = '\0';
    TEST_CHECK(capture_list(&list, buffer, sizeof(buffer)) == 0);

    port_list_free(&list);
}

int main(void)
{
    test_collects_available_ports();
    test_prints_stubbed_list();
    test_empty_result_prints_nothing();
    test_include_filter();
    test_include_list_skips_ports_outside_list();
    test_exclude_filter();
    test_exclude_wins_over_include();
    test_bad_arguments();

    return test_summary("test_scan");
}