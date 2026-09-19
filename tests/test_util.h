#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdio.h>
#include <stdlib.h>

/*
 * Minimal assertion helpers for the CTest binaries. Each test file is its own
 * executable, so the counters can live in file scope without linking a shared
 * test library.
 */

static int test_checks = 0;
static int test_failures = 0;

#define TEST_CHECK(condition) \
    do { \
        test_checks++; \
        if (!(condition)) { \
            test_failures++; \
            fprintf( \
                stderr, \
                "FAIL %s:%d: %s\n", \
                __FILE__, \
                __LINE__, \
                #condition \
            ); \
        } \
    } while (0)

static inline int test_summary(const char *name)
{
    if (test_failures == 0) {
        printf(
            "%s: %d checks passed\n",
            name,
            test_checks
        );

        return EXIT_SUCCESS;
    }

    fprintf(
        stderr,
        "%s: %d of %d checks failed\n",
        name,
        test_failures,
        test_checks
    );

    return EXIT_FAILURE;
}

#endif