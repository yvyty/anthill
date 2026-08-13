#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "net_utils.h"

#define MAX_THREADS 16
#define DEFAULT_START_PORT 1
#define DEFAULT_END_PORT 65535

typedef enum {
    FORMAT_HUMAN,
    FORMAT_COUNT,
    FORMAT_JSON
} output_format_t;

typedef struct {
    int start_port;
    int end_port;
    output_format_t format;
} app_config_t;

typedef struct {
    int squadron_id;
    int start_port;
    int end_port;

    int *total_available;
    anthill_mutex_t *mutex;

    output_format_t format;
} worker_args_t;


static app_config_t parse_arguments(int argc, char *argv[]);

static bool check_port_availability(int port);

static ANTHILL_THREAD_FUNC ant_worker(void *arg);


static bool check_port_availability(int port)
{
    socket_fd_t sock = socket(AF_INET, SOCK_STREAM, 0);

    if (!VALID_SOCKET(sock)) {
        return false;
    }

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    if (
        inet_pton(
            AF_INET,
            "127.0.0.1",
            &addr.sin_addr
        ) != 1
    ) {
        CLOSE_SOCKET(sock);
        return false;
    }

    int result = bind(
        sock,
        (struct sockaddr *)&addr,
        sizeof(addr)
    );

    CLOSE_SOCKET(sock);

    return result == 0;
}

static ANTHILL_THREAD_FUNC ant_worker(void *arg)
{
    worker_args_t *args = (worker_args_t *)arg;

    int local_count = 0;

    for (
        int port = args->start_port;
        port <= args->end_port;
        port++
    ) {
        if (check_port_availability(port)) {
            local_count++;
        }
    }

    anthill_mutex_lock(args->mutex);

    *(args->total_available) += local_count;

    anthill_mutex_unlock(args->mutex);

    if (args->format == FORMAT_HUMAN) {
        printf(
            "    -> Squadron %02d completed (Ports %d to %d).\n",
            args->squadron_id,
            args->start_port,
            args->end_port
        );
    }

    return (ANTHILL_THREAD_RETURN)0;
}

static app_config_t parse_arguments(int argc, char *argv[])
{
    app_config_t config = {
        DEFAULT_START_PORT,
        DEFAULT_END_PORT,
        FORMAT_HUMAN
    };

    for (int i = 1; i < argc; i++) {
        if (
            strcmp(argv[i], "-r") == 0 &&
            i + 1 < argc
        ) {
            if (
                sscanf(
                    argv[i + 1],
                    "%d-%d",
                    &config.start_port,
                    &config.end_port
                ) != 2
            ) {
                fprintf(
                    stderr,
                    "Error: Invalid range format.\n"
                    "Use: -r START-END\n"
                );

                exit(EXIT_FAILURE);
            }

            if (
                config.start_port < 1 ||
                config.end_port > 65535 ||
                config.start_port > config.end_port
            ) {
                fprintf(
                    stderr,
                    "Error: Ports must be between 1 and 65535 "
                    "and START must be <= END.\n"
                );

                exit(EXIT_FAILURE);
            }

            i++;
        }

        else if (strcmp(argv[i], "-c") == 0) {
            config.format = FORMAT_COUNT;
        }

        else if (strcmp(argv[i], "-j") == 0) {
            config.format = FORMAT_JSON;
        }

        else if (
            strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0
        ) {
            printf("USAGE:\n");
            printf("    anthill [OPTIONS]\n\n");

            printf("OPTIONS:\n");
            printf(
                "    -r START-END    Specify port range "
                "(default: 1-65535)\n"
            );

            printf(
                "    -c              Output only the total "
                "count of available ports\n"
            );

            printf(
                "    -j              Output result as JSON\n"
            );

            printf(
                "    -h, --help      Show this help message\n"
            );

            exit(EXIT_SUCCESS);
        }

        else {
            fprintf(
                stderr,
                "Error: Unknown argument: %s\n",
                argv[i]
            );

            fprintf(
                stderr,
                "Run 'anthill --help' for usage information.\n"
            );

            exit(EXIT_FAILURE);
        }
    }

    return config;
}


int main(int argc, char *argv[])
{
    app_config_t config = parse_arguments(argc, argv);

    if (config.format == FORMAT_HUMAN) {
        printf("Spawning Anthill...\n");
    }

    init_network_workers();
    int total_available = 0;
    anthill_mutex_t counter_mutex;
    anthill_mutex_init(&counter_mutex);
    int total_ports_to_scan =
        config.end_port -
        config.start_port +
        1;
    int active_threads =
        total_ports_to_scan < MAX_THREADS
            ? total_ports_to_scan
            : MAX_THREADS;

    anthill_thread_t threads[MAX_THREADS];
    worker_args_t thread_args[MAX_THREADS];

    int ports_per_thread =
        total_ports_to_scan / active_threads;


    if (config.format == FORMAT_HUMAN) {
        printf(
            "Deploying %d ant squadrons to check ports %d-%d...\n",
            active_threads,
            config.start_port,
            config.end_port
        );
    }

    int created_threads = 0;

    for (int i = 0; i < active_threads; i++) {

        worker_args_t *args = &thread_args[i];

        args->squadron_id = i + 1;

        args->start_port =
            config.start_port +
            (i * ports_per_thread);

        if (i == active_threads - 1) {
            args->end_port = config.end_port;
        } else {
            args->end_port =
                args->start_port +
                ports_per_thread -
                1;
        }

        args->total_available = &total_available;
        args->mutex = &counter_mutex;
        args->format = config.format;


        int thread_result = anthill_thread_create(
            &threads[i],
            ant_worker,
            args
        );

        if (thread_result != 0) {
            fprintf(
                stderr,
                "Error: Failed to create squadron %d.\n",
                i + 1
            );

            for (int j = 0; j < created_threads; j++) {
                anthill_thread_join(threads[j]);
            }

            anthill_mutex_destroy(&counter_mutex);

            cleanup_network_workers();

            return EXIT_FAILURE;
        }

        created_threads++;
    }

    for (int i = 0; i < created_threads; i++) {
        anthill_thread_join(threads[i]);
    }

    anthill_mutex_destroy(&counter_mutex);

    if (config.format == FORMAT_HUMAN) {

        printf(
            "\nAnthill dormant. All ants returned.\n"
        );

        printf(
            "Total available ports: %d\n",
            total_available
        );

    } else if (config.format == FORMAT_COUNT) {

        printf(
            "%d\n",
            total_available
        );

    } else if (config.format == FORMAT_JSON) {

        printf("{\n");

        printf(
            "    \"start_port\": %d,\n",
            config.start_port
        );

        printf(
            "    \"end_port\": %d,\n",
            config.end_port
        );

        printf(
            "    \"available_ports\": %d\n",
            total_available
        );

        printf("}\n");
    }

    cleanup_network_workers();

    return EXIT_SUCCESS;
}
