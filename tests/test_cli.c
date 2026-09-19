#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_util.h"

#ifdef _WIN32
    #include <process.h>

    #define POPEN _popen
    #define PCLOSE _pclose
    #define GETPID _getpid
#else
    #include <sys/wait.h>
    #include <unistd.h>

    #define POPEN popen
    #define PCLOSE pclose
    #define GETPID getpid
#endif

#ifndef ANTHILL_BINARY
    #error "ANTHILL_BINARY must be defined by the build system"
#endif

#define COMMAND_SIZE 1024
#define STDOUT_SIZE 512

static int run_anthill(const char *arguments, char *buffer, size_t size)
{
    char command[COMMAND_SIZE];

    snprintf(
        command,
        sizeof(command),
        "\"%s\" %s",
        ANTHILL_BINARY,
        arguments
    );

    FILE *pipe = POPEN(command, "r");

    if (pipe == NULL) {
        return -1;
    }

    if (buffer != NULL && size > 0) {
        size_t read = fread(buffer, 1, size - 1, pipe);

        buffer[read] = '\0';
    } else {
        char discard[256];

        while (fread(discard, 1, sizeof(discard), pipe) > 0) {
        }
    }

    int status = PCLOSE(pipe);

#ifdef _WIN32
    return status;
#else
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
#endif
}

static bool is_single_integer(const char *text)
{
    char *end = NULL;
    long value = strtol(text, &end, 10);

    (void)value;

    if (end == text) {
        return false;
    }

    while (
        *end == ' ' ||
        *end == '\t' ||
        *end == '\r' ||
        *end == '\n'
    ) {
        end++;
    }

    return *end == '\0';
}

static void make_temp_path(char *buffer, size_t size)
{
#ifdef _WIN32
    const char *base = getenv("TEMP");

    if (base == NULL || *base == '\0') {
        base = ".";
    }
#else
    const char *base = "/tmp";
#endif

    snprintf(
        buffer,
        size,
        "%s/anthill_ctest_output_%ld.txt",
        base,
        (long)GETPID
    );
}

static void test_count_output(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(run_anthill("-c -r 1-1", output, sizeof(output)) == 0);
    TEST_CHECK(is_single_integer(output));
}

static void test_json_output(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(run_anthill("-j -r 1-1", output, sizeof(output)) == 0);
    TEST_CHECK(output[0] == '{');
    TEST_CHECK(strstr(output, "\"available_ports\":") != NULL);
}

static void test_human_output_respects_no_list(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(run_anthill("-r 1-1", output, sizeof(output)) == 0);
    TEST_CHECK(strstr(output, "Total available ports:") != NULL);

    TEST_CHECK(
        run_anthill("-r 1-1 --no-list", output, sizeof(output)) == 0
    );
    TEST_CHECK(strstr(output, "Total available ports:") != NULL);
    TEST_CHECK(strstr(output, "Available ports") == NULL);
}

static void test_help_documents_flags(void)
{
    char output[STDOUT_SIZE * 4];

    TEST_CHECK(run_anthill("--help", output, sizeof(output)) == 0);
    TEST_CHECK(strstr(output, "--list") != NULL);
    TEST_CHECK(strstr(output, "--no-list") != NULL);
    TEST_CHECK(strstr(output, "-o PATH") != NULL);
    TEST_CHECK(strstr(output, "-x PORTS") != NULL);
    TEST_CHECK(strstr(output, "-i PORTS") != NULL);
}

static void test_file_output(void)
{
    char path[512];
    char command[COMMAND_SIZE];
    char output[STDOUT_SIZE];

    make_temp_path(path, sizeof(path));
    remove(path);

    snprintf(
        command,
        sizeof(command),
        "-c -o \"%s\" -r 1-1",
        path
    );

    TEST_CHECK(run_anthill(command, output, sizeof(output)) == 0);
    TEST_CHECK(output[0] == '\0');

    FILE *file = fopen(path, "r");

    TEST_CHECK(file != NULL);

    if (file != NULL) {
        char contents[STDOUT_SIZE];

        size_t read = fread(contents, 1, sizeof(contents) - 1, file);

        contents[read] = '\0';

        fclose(file);

        TEST_CHECK(is_single_integer(contents));
    }

    remove(path);
}

static void test_json_file_output(void)
{
    char path[512];
    char command[COMMAND_SIZE];

    make_temp_path(path, sizeof(path));
    remove(path);

    snprintf(
        command,
        sizeof(command),
        "-j -o \"%s\" -r 1-1",
        path
    );

    TEST_CHECK(run_anthill(command, NULL, 0) == 0);

    FILE *file = fopen(path, "r");

    TEST_CHECK(file != NULL);

    if (file != NULL) {
        char contents[STDOUT_SIZE];

        size_t read = fread(contents, 1, sizeof(contents) - 1, file);

        contents[read] = '\0';

        fclose(file);

        TEST_CHECK(contents[0] == '{');
        TEST_CHECK(strstr(contents, "\"available_ports\":") != NULL);
        TEST_CHECK(strstr(contents, "}") != NULL);
    }

    remove(path);
}

static void test_unwritable_output_fails(void)
{
#ifdef _WIN32
    const char *bad_path = "Z:/nonexistent-anthill-tests/output.txt";
#else
    const char *bad_path = "/nonexistent-anthill-tests/output.txt";
#endif

    char command[COMMAND_SIZE];

    snprintf(
        command,
        sizeof(command),
        "-c -o \"%s\" -r 1-1",
        bad_path
    );

    TEST_CHECK(run_anthill(command, NULL, 0) != 0);
}

static void test_exclude_removes_only_port(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(
        run_anthill(
            "-r 1-1 -x 1 --list",
            output,
            sizeof(output)
        ) == 0
    );
    TEST_CHECK(strstr(output, "Available ports") == NULL);
    TEST_CHECK(strstr(output, "Total available ports: 0") != NULL);
}

static void test_include_intersects_range(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(
        run_anthill(
            "-r 1-1 -i 65534 --list",
            output,
            sizeof(output)
        ) == 0
    );
    TEST_CHECK(strstr(output, "Available ports") == NULL);
    TEST_CHECK(strstr(output, "Total available ports: 0") != NULL);
}

static int total_available_from(const char *output)
{
    const char *marker = strstr(output, "Total available ports:");

    if (marker == NULL) {
        return -1;
    }

    return atoi(marker + strlen("Total available ports:"));
}

static void test_include_limits_checked_ports(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(
        run_anthill(
            "-r 1-65535 -i 1,2,3 --list",
            output,
            sizeof(output)
        ) == 0
    );

    int total = total_available_from(output);

    TEST_CHECK(total >= 0);
    TEST_CHECK(total <= 3);
    TEST_CHECK(strstr(output, "available_ports") == NULL);
}

static void test_exclude_range_blocks_band(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(
        run_anthill(
            "-r 8000-9000 -x 8000-9000 --list",
            output,
            sizeof(output)
        ) == 0
    );

    TEST_CHECK(strstr(output, "Available ports") == NULL);
    TEST_CHECK(total_available_from(output) == 0);
}

static void test_invalid_flags_fail(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(
        run_anthill("-r 1-1 -x not-a-port", output, sizeof(output)) != 0
    );
    TEST_CHECK(
        run_anthill("-r 1-1 -i 70000", output, sizeof(output)) != 0
    );
    TEST_CHECK(
        run_anthill("-r 1-1 -o", output, sizeof(output)) != 0
    );
    TEST_CHECK(
        run_anthill("-r 443-80", output, sizeof(output)) != 0
    );
}

int main(void)
{
    test_count_output();
    test_json_output();
    test_human_output_respects_no_list();
    test_help_documents_flags();
    test_file_output();
    test_json_file_output();
    test_unwritable_output_fails();
    test_exclude_removes_only_port();
    test_include_intersects_range();
    test_include_limits_checked_ports();
    test_exclude_range_blocks_band();
    test_invalid_flags_fail();

    return test_summary("test_cli");
}