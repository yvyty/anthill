#ifndef SCAN_H
#define SCAN_H

#include <stdbool.h>
#include <stdio.h>

#include "port_set.h"

/*
 * Testable scanning core.
 *
 * `scan_collect_available` receives the availability probe as a function
 * pointer so tests can supply a stub instead of binding real sockets. The
 * threaded worker in src/main.c passes `check_port_availability`.
 */

typedef bool (*port_available_fn)(int port);

typedef struct {
    int *ports;
    int count;
    int capacity;
} port_list_t;

void port_list_init(port_list_t *list);

void port_list_free(port_list_t *list);

bool port_list_push(port_list_t *list, int port);

void port_list_sort(port_list_t *list);

/*
 * Probes every port in [start_port, end_port] that survives the include and
 * exclude filters, appending available ones to `out` in ascending order.
 *
 * `include_ports`/`exclude_ports` may be NULL to disable that filter. An
 * excluded port is never probed even when it also appears in the include set.
 *
 * Returns the number of ports appended, or -1 on failure (bad input or
 * allocation failure).
 */
int scan_collect_available(
    int start_port,
    int end_port,
    const port_set_t *include_ports,
    const port_set_t *exclude_ports,
    port_available_fn is_available,
    port_list_t *out
);

/*
 * Writes the human-readable listing for `list`. Prints nothing when the list
 * is empty so a failed scan never claims to have found ports.
 */
void print_port_list(FILE *out, const port_list_t *list);

#endif