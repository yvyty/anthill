#ifndef PORT_SET_H
#define PORT_SET_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*
 * Fixed-size bitmap over the valid TCP port space (1..65535).
 *
 * The bitmap is allocated inline (8 KiB) so membership checks never touch the
 * heap: `port_set_contains` is called once per port inside the worker hot
 * loop, where a malloc would be unacceptable.
 */

#define PORT_SET_MIN_PORT 1
#define PORT_SET_MAX_PORT 65535
#define PORT_SET_SLOTS (PORT_SET_MAX_PORT + 1)
#define PORT_SET_BYTES ((PORT_SET_SLOTS + 7) / 8)

typedef struct {
    uint8_t bits[PORT_SET_BYTES];
} port_set_t;

static inline void port_set_init(port_set_t *set)
{
    memset(set, 0, sizeof(*set));
}

static inline bool port_set_is_valid_port(int port)
{
    return port >= PORT_SET_MIN_PORT && port <= PORT_SET_MAX_PORT;
}

static inline void port_set_add(port_set_t *set, int port)
{
    if (!port_set_is_valid_port(port)) {
        return;
    }

    set->bits[port >> 3] |= (uint8_t)(1u << (port & 7));
}

static inline void port_set_add_range(port_set_t *set, int start_port, int end_port)
{
    if (start_port < PORT_SET_MIN_PORT) {
        start_port = PORT_SET_MIN_PORT;
    }

    if (end_port > PORT_SET_MAX_PORT) {
        end_port = PORT_SET_MAX_PORT;
    }

    for (int port = start_port; port <= end_port; port++) {
        port_set_add(set, port);
    }
}

static inline bool port_set_contains(const port_set_t *set, int port)
{
    if (!port_set_is_valid_port(port)) {
        return false;
    }

    return ((set->bits[port >> 3] >> (port & 7)) & 1u) != 0;
}

static inline int port_set_count(const port_set_t *set)
{
    int count = 0;

    for (size_t i = 0; i < PORT_SET_BYTES; i++) {
        uint8_t byte = set->bits[i];

        while (byte != 0) {
            count += (byte & 1u);
            byte = (uint8_t)(byte >> 1);
        }
    }

    return count;
}

#endif