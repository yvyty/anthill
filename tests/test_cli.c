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
#define STDERR_SIZE 8192

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
    }

    /* Drain whatever did not fit so the child never blocks on a full pipe. */
    char discard[256];

    while (fread(discard, 1, sizeof(discard), pipe) > 0) {
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

static void make_stderr_path(char *buffer, size_t size)
{
    char base_path[512];

    make_temp_path(base_path, sizeof(base_path));

    snprintf(buffer, size, "%s.err", base_path);
}

static bool read_file_contents(const char *path, char *buffer, size_t size)
{
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return false;
    }

    size_t read = fread(buffer, 1, size - 1, file);

    buffer[read] = '\0';

    fclose(file);

    return true;
}

static bool file_is_empty(const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return false;
    }

    int character = fgetc(file);

    fclose(file);

    return character == EOF;
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
    TEST_CHECK(strstr(output, "--threads N") != NULL);
    TEST_CHECK(strstr(output, "--progress") != NULL);
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

static void test_thread_count_single_thread_matches_many(void)
{
    char output[STDOUT_SIZE];
    char multi[STDOUT_SIZE];

    TEST_CHECK(
        run_anthill("-c -r 1-500 -t 1", output, sizeof(output)) == 0
    );
    TEST_CHECK(is_single_integer(output));

    TEST_CHECK(
        run_anthill("-c -r 1-500 -t 8", multi, sizeof(multi)) == 0
    );
    TEST_CHECK(is_single_integer(multi));
    TEST_CHECK(strcmp(output, multi) == 0);
}

static void test_thread_count_matches_banner(void)
{
    char output[STDOUT_SIZE * 4];

    TEST_CHECK(
        run_anthill("-r 1-64 -t 64 2>&1", output, sizeof(output)) == 0
    );
    TEST_CHECK(strstr(output, "Deploying 64 ant squadrons") != NULL);
}

static void test_thread_count_accepts_cap(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(
        run_anthill("-c -r 1-512 -t 256", output, sizeof(output)) == 0
    );
    TEST_CHECK(is_single_integer(output));
}

static void test_thread_count_clamps_large(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(
        run_anthill("-c -r 1-64 -t 99999", output, sizeof(output)) == 0
    );
    TEST_CHECK(is_single_integer(output));
}

static void test_thread_count_rejects_invalid(void)
{
    char output[STDOUT_SIZE];

    TEST_CHECK(run_anthill("-r 1-1 -t 0", output, sizeof(output)) != 0);
    TEST_CHECK(run_anthill("-r 1-1 -t abc", output, sizeof(output)) != 0);
    TEST_CHECK(run_anthill("-r 1-1 -t", output, sizeof(output)) != 0);
    TEST_CHECK(
        run_anthill("-r 1-1 --threads 0", output, sizeof(output)) != 0
    );
}

static void test_progress_suppressed_without_flag(void)
{
    char err_path[512];
    char output[STDOUT_SIZE];
    char command[COMMAND_SIZE];

    make_stderr_path(err_path, sizeof(err_path));
    remove(err_path);

    snprintf(command, sizeof(command), "-j -r 1-1 2> \"%s\"", err_path);

    TEST_CHECK(run_anthill(command, output, sizeof(output)) == 0);
    TEST_CHECK(output[0] == '{');
    TEST_CHECK(file_is_empty(err_path));

    remove(err_path);
}

static void test_progress_emits_when_requested(void)
{
    char err_path[512];
    char output[STDOUT_SIZE];
    char progress[STDERR_SIZE];
    char command[COMMAND_SIZE];

    make_stderr_path(err_path, sizeof(err_path));
    remove(err_path);

    snprintf(
        command,
        sizeof(command),
        "-c -r 1-2000 --progress 2> \"%s\"",
        err_path
    );

    TEST_CHECK(run_anthill(command, output, sizeof(output)) == 0);
    TEST_CHECK(is_single_integer(output));

    TEST_CHECK(read_file_contents(err_path, progress, sizeof(progress)));
    TEST_CHECK(strstr(progress, "Progress:") != NULL);
    TEST_CHECK(strstr(progress, "%") != NULL);

    remove(err_path);
}

static void test_progress_json_only_on_stderr(void)
{
    char err_path[512];
    char output[STDOUT_SIZE];
    char progress[STDERR_SIZE];
    char command[COMMAND_SIZE];

    make_stderr_path(err_path, sizeof(err_path));
    remove(err_path);

    snprintf(
        command,
        sizeof(command),
        "-j -r 1-1000 --progress 2> \"%s\"",
        err_path
    );

    TEST_CHECK(run_anthill(command, output, sizeof(output)) == 0);
    TEST_CHECK(output[0] == '{');
    TEST_CHECK(strstr(output, "Progress") == NULL);

    TEST_CHECK(read_file_contents(err_path, progress, sizeof(progress)));
    TEST_CHECK(strstr(progress, "Progress:") != NULL);
    TEST_CHECK(strstr(progress, "available_ports") == NULL);

    remove(err_path);
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
    test_thread_count_single_thread_matches_many();
    test_thread_count_matches_banner();
    test_thread_count_accepts_cap();
    test_thread_count_clamps_large();
    test_thread_count_rejects_invalid();
    test_progress_suppressed_without_flag();
    test_progress_emits_when_requested();
    test_progress_json_only_on_stderr();

    return test_summary("test_cli");
}