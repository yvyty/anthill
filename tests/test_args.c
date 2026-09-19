#include <stdbool.h>

#include "args.h"
#include "test_util.h"

static void test_parse_port_range(void)
{
    int start = 0;
    int end = 0;

    TEST_CHECK(parse_port_range("80-443", &start, &end));
    TEST_CHECK(start == 80);
    TEST_CHECK(end == 443);

    TEST_CHECK(parse_port_range("1-65535", &start, &end));
    TEST_CHECK(start == 1);
    TEST_CHECK(end == 65535);

    TEST_CHECK(!parse_port_range("443-80", &start, &end));
    TEST_CHECK(!parse_port_range("0-10", &start, &end));
    TEST_CHECK(!parse_port_range("1-70000", &start, &end));
    TEST_CHECK(!parse_port_range("70000", &start, &end));
    TEST_CHECK(!parse_port_range("8080", &start, &end));
    TEST_CHECK(!parse_port_range("abc-def", &start, &end));
    TEST_CHECK(!parse_port_range("80-", &start, &end));
    TEST_CHECK(!parse_port_range("-80", &start, &end));
    TEST_CHECK(!parse_port_range("", &start, &end));
    TEST_CHECK(!parse_port_range(NULL, &start, &end));
}

static void test_parse_port_list(void)
{
    port_set_t set;

    port_set_init(&set);
    TEST_CHECK(parse_port_list("22,80,443", &set));
    TEST_CHECK(port_set_count(&set) == 3);
    TEST_CHECK(port_set_contains(&set, 22));
    TEST_CHECK(port_set_contains(&set, 80));
    TEST_CHECK(port_set_contains(&set, 443));
    TEST_CHECK(!port_set_contains(&set, 23));

    port_set_init(&set);
    TEST_CHECK(parse_port_list("8000-9000", &set));
    TEST_CHECK(port_set_count(&set) == 1001);
    TEST_CHECK(port_set_contains(&set, 8000));
    TEST_CHECK(port_set_contains(&set, 8500));
    TEST_CHECK(port_set_contains(&set, 9000));
    TEST_CHECK(!port_set_contains(&set, 7999));
    TEST_CHECK(!port_set_contains(&set, 9001));

    port_set_init(&set);
    TEST_CHECK(parse_port_list("22,8000-8002", &set));
    TEST_CHECK(port_set_count(&set) == 4);

    port_set_init(&set);
    TEST_CHECK(parse_port_list("80,80,80", &set));
    TEST_CHECK(port_set_count(&set) == 1);

    port_set_init(&set);
    TEST_CHECK(parse_port_list("1-10,5-15", &set));
    TEST_CHECK(port_set_count(&set) == 15);

    port_set_init(&set);
    TEST_CHECK(!parse_port_list("0,70000", &set));
    TEST_CHECK(!parse_port_list("abc", &set));
    TEST_CHECK(!parse_port_list("80,,443", &set));
    TEST_CHECK(!parse_port_list("", &set));
    TEST_CHECK(!parse_port_list("80-", &set));
    TEST_CHECK(!parse_port_list(NULL, &set));
}

int main(void)
{
    test_parse_port_range();
    test_parse_port_list();

    return test_summary("test_args");
}