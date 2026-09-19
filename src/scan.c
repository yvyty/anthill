#include "scan.h"

#include <stdlib.h>

#define PORT_LIST_INITIAL_CAPACITY 16

void port_list_init(port_list_t *list)
{
    list->ports = NULL;
    list->count = 0;
    list->capacity = 0;
}

void port_list_free(port_list_t *list)
{
    free(list->ports);

    list->ports = NULL;
    list->count = 0;
    list->capacity = 0;
}

bool port_list_push(port_list_t *list, int port)
{
    if (list->count == list->capacity) {
        int new_capacity = list->capacity == 0
            ? PORT_LIST_INITIAL_CAPACITY
            : list->capacity * 2;

        int *grown = (int *)realloc(
            list->ports,
            (size_t)new_capacity * sizeof(int)
        );

        if (grown == NULL) {
            return false;
        }

        list->ports = grown;
        list->capacity = new_capacity;
    }

    list->ports[list->count] = port;
    list->count++;

    return true;
}

static int compare_ports(const void *left, const void *right)
{
    int a = *(const int *)left;
    int b = *(const int *)right;

    if (a < b) {
        return -1;
    }

    if (a > b) {
        return 1;
    }

    return 0;
}

void port_list_sort(port_list_t *list)
{
    if (list->count < 2) {
        return;
    }

    qsort(list->ports, (size_t)list->count, sizeof(int), compare_ports);
}

int scan_collect_available(
    int start_port,
    int end_port,
    const port_set_t *include_ports,
    const port_set_t *exclude_ports,
    port_available_fn is_available,
    scan_progress_fn on_progress,
    void *progress_context,
    port_list_t *out
)
{
    if (out == NULL || is_available == NULL) {
        return -1;
    }

    int collected = 0;

    for (int port = start_port; port <= end_port; port++) {
        if (!port_set_is_valid_port(port)) {
            continue;
        }

        if (
            include_ports != NULL &&
            !port_set_contains(include_ports, port)
        ) {
            continue;
        }

        if (
            exclude_ports != NULL &&
            port_set_contains(exclude_ports, port)
        ) {
            continue;
        }

        if (on_progress != NULL) {
            on_progress(1, progress_context);
        }

        if (!is_available(port)) {
            continue;
        }

        if (!port_list_push(out, port)) {
            return -1;
        }

        collected++;
    }

    return collected;
}

void print_port_list(FILE *out, const port_list_t *list)
{
    if (out == NULL || list == NULL || list->count == 0) {
        return;
    }

    fprintf(
        out,
        "Available ports (%d):\n",
        list->count
    );

    for (int i = 0; i < list->count; i++) {
        fprintf(
            out,
            "    %d\n",
            list->ports[i]
        );
    }
}