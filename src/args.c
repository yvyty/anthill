#include "args.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define PORT_TOKEN_MAX 32

static bool parse_port_number(const char *text, int *port)
{
    if (
        text == NULL ||
        *text == '\0' ||
        text[0] < '0' ||
        text[0] > '9'
    ) {
        return false;
    }

    errno = 0;

    char *end = NULL;
    long value = strtol(text, &end, 10);

    if (end == text || *end != '\0' || errno == ERANGE) {
        return false;
    }

    if (
        value < PORT_SET_MIN_PORT ||
        value > PORT_SET_MAX_PORT
    ) {
        return false;
    }

    *port = (int)value;

    return true;
}

bool parse_port_range(
    const char *text,
    int *start_port,
    int *end_port
)
{
    if (text == NULL || *text == '\0') {
        return false;
    }

    const char *hyphen = strchr(text, '-');

    if (
        hyphen == NULL ||
        hyphen == text ||
        hyphen[1] == '\0'
    ) {
        return false;
    }

    size_t start_length = (size_t)(hyphen - text);

    if (start_length >= PORT_TOKEN_MAX) {
        return false;
    }

    char start_text[PORT_TOKEN_MAX];

    memcpy(start_text, text, start_length);
    start_text[start_length] = '\0';

    int start = 0;
    int end = 0;

    if (!parse_port_number(start_text, &start)) {
        return false;
    }

    if (!parse_port_number(hyphen + 1, &end)) {
        return false;
    }

    if (start > end) {
        return false;
    }

    *start_port = start;
    *end_port = end;

    return true;
}

bool parse_port_list(const char *text, port_set_t *set)
{
    if (text == NULL || *text == '\0') {
        return false;
    }

    const char *cursor = text;

    while (*cursor != '\0') {
        const char *comma = strchr(cursor, ',');
        size_t length = comma != NULL
            ? (size_t)(comma - cursor)
            : strlen(cursor);

        if (length == 0 || length >= PORT_TOKEN_MAX) {
            return false;
        }

        char token[PORT_TOKEN_MAX];

        memcpy(token, cursor, length);
        token[length] = '\0';

        int start = 0;
        int end = 0;

        if (strchr(token, '-') != NULL) {
            if (!parse_port_range(token, &start, &end)) {
                return false;
            }

            port_set_add_range(set, start, end);
        } else {
            if (!parse_port_number(token, &start)) {
                return false;
            }

            port_set_add(set, start);
        }

        if (comma == NULL) {
            break;
        }

        cursor = comma + 1;
    }

    return true;
}

bool parse_thread_count(const char *text, int *thread_count)
{
    if (
        text == NULL ||
        thread_count == NULL ||
        *text == '\0' ||
        text[0] < '0' ||
        text[0] > '9'
    ) {
        return false;
    }

    errno = 0;

    char *end = NULL;
    long value = strtol(text, &end, 10);

    if (end == text || *end != '\0' || errno == ERANGE) {
        return false;
    }

    if (value < 1) {
        return false;
    }

    if (value > MAX_THREADS_CAP) {
        value = MAX_THREADS_CAP;
    }

    *thread_count = (int)value;

    return true;
}