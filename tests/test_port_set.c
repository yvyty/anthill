#include "port_set.h"
#include "test_util.h"

static void test_single_ports(void)
{
    port_set_t set;

    port_set_init(&set);
    TEST_CHECK(port_set_count(&set) == 0);
    TEST_CHECK(!port_set_contains(&set, 80));

    port_set_add(&set, 80);
    TEST_CHECK(port_set_count(&set) == 1);
    TEST_CHECK(port_set_contains(&set, 80));
    TEST_CHECK(!port_set_contains(&set, 81));

    port_set_add(&set, 80);
    port_set_add(&set, 80);
    TEST_CHECK(port_set_count(&set) == 1);

    port_set_add(&set, 1);
    port_set_add(&set, 65535);
    TEST_CHECK(port_set_contains(&set, 1));
    TEST_CHECK(port_set_contains(&set, 65535));
    TEST_CHECK(port_set_count(&set) == 3);
}

static void test_ranges(void)
{
    port_set_t set;

    port_set_init(&set);
    port_set_add_range(&set, 1, 3);
    port_set_add(&set, 2);
    TEST_CHECK(port_set_count(&set) == 3);

    port_set_init(&set);
    port_set_add_range(&set, 1, 10);
    port_set_add_range(&set, 5, 15);
    TEST_CHECK(port_set_count(&set) == 15);
    TEST_CHECK(port_set_contains(&set, 15));
    TEST_CHECK(!port_set_contains(&set, 16));
}

static void test_out_of_range_values(void)
{
    port_set_t set;

    port_set_init(&set);

    port_set_add(&set, 0);
    port_set_add(&set, -1);
    port_set_add(&set, 65536);

    TEST_CHECK(port_set_count(&set) == 0);
    TEST_CHECK(!port_set_contains(&set, 0));
    TEST_CHECK(!port_set_contains(&set, -1));
    TEST_CHECK(!port_set_contains(&set, 65536));

    port_set_add_range(&set, 65000, 70000);
    TEST_CHECK(port_set_count(&set) == 536);
    TEST_CHECK(port_set_contains(&set, 65000));
    TEST_CHECK(port_set_contains(&set, 65535));
    TEST_CHECK(!port_set_contains(&set, 65536));
}

int main(void)
{
    test_single_ports();
    test_ranges();
    test_out_of_range_values();

    return test_summary("test_port_set");
}