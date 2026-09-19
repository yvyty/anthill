#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

#include "port_set.h"

/*
 * Default squadron count when the user does not pass -t, and the hard ceiling
 * that larger requested values are clamped to.
 */
#define MAX_THREADS 16
#define MAX_THREADS_CAP 256

/*
 * Pure, side-effect-free argument helpers.
 *
 * They report failure with a `false` return instead of calling exit(), which
 * keeps them unit-testable and lets the CLI layer own user-facing messages.
 */

/*
 * Parses "START-END" with both bounds inside 1..65535 and START <= END.
 * Returns false for a missing hyphen, non-numeric bounds, reversed bounds, or
 * values outside the port space.
 */
bool parse_port_range(
    const char *text,
    int *start_port,
    int *end_port
);

/*
 * Parses a comma-separated port list where each element is either a single
 * port ("80") or an inclusive span ("8000-9000"). Ports are added to `set`.
 *
 * Returns false on the first invalid or empty element; callers decide whether
 * partial additions matter.
 */
bool parse_port_list(const char *text, port_set_t *set);

/*
 * Parses a thread count. Returns false for empty, non-numeric, or zero/negative
 * input. Values above MAX_THREADS_CAP are accepted but clamped down to it.
 */
bool parse_thread_count(const char *text, int *thread_count);

#endif