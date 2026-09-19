#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

#include "port_set.h"

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

#endif