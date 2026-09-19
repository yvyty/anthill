#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "net_utils.h"
#include "port_set.h"
#include "scan.h"

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
    bool list_ports;
    const char *output_path;
    int thread_count;
    bool progress;

    port_set_t include_ports;
    port_set_t exclude_ports;
    bool has_include;
    bool has_exclude;
} app_config_t;

typedef struct {
    int *scanned;
    int total;
    int last_percent;
    anthill_mutex_t *mutex;
} progress_state_t;

typedef struct {
    int squadron_id;
    int start_port;
    int end_port;

    int *total_available;
    anthill_mutex_t *mutex;

    const port_set_t *include_ports;
    const port_set_t *exclude_ports;

    output_format_t format;

    scan_progress_fn on_progress;
    void *progress_context;

    port_list_t results;
    bool failed;
} worker_args_t;


static app_config_t parse_arguments(int argc, char *argv[]);

static void print_help(void);

static bool check_port_availability(int port);

static void print_results(
    FILE *out,
    const app_config_t *config,
    const port_list_t *ports,
    int total_available
);

static ANTHILL_THREAD_FUNC ant_worker(void *arg);

static void report_progress(int ports_scanned, void *context);


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

    /*
     * The squadron builds its list in a private buffer; the shared count is
     * the only thing merged under the mutex, so there is no per-port locking.
     */
    int available = scan_collect_available(
        args->start_port,
        args->end_port,
        args->include_ports,
        args->exclude_ports,
        check_port_availability,
        args->on_progress,
        args->progress_context,
        &args->results
    );

    if (available < 0) {
        args->failed = true;
    } else {
        anthill_mutex_lock(args->mutex);

        *(args->total_available) += available;

        anthill_mutex_unlock(args->mutex);
    }

    if (args->format == FORMAT_HUMAN) {
        /* Serialize progress lines so squadrons never interleave mid-line. */
        anthill_mutex_lock(args->mutex);

        fprintf(
            stderr,
            "    -> Squadron %02d completed (Ports %d to %d).\n",
            args->squadron_id,
            args->start_port,
            args->end_port
        );

        anthill_mutex_unlock(args->mutex);
    }

    return (ANTHILL_THREAD_RETURN)0;
}

static void report_progress(int ports_scanned, void *context)
{
    progress_state_t *state = (progress_state_t *)context;

    if (state == NULL) {
        return;
    }

    anthill_mutex_lock(state->mutex);

    *(state->scanned) += ports_scanned;

    int scanned = *(state->scanned);
    int percent = state->total > 0
        ? (int)(((long)scanned * 100L) / state->total)
        : 100;

    if (percent > state->last_percent) {
        state->last_percent = percent;

        fprintf(
            stderr,
            "Progress: %d%% (%d/%d ports scanned)\n",
            percent,
            scanned,
            state->total
        );

        fflush(stderr);
    }

    anthill_mutex_unlock(state->mutex);
}

static void print_help(void)
{
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
        "    --list          List every available port "
        "(default for human output)\n"
    );

    printf(
        "    --no-list       Suppress the per-port "
        "listing\n"
    );

    printf(
        "    -o PATH         Write results to PATH "
        "instead of stdout\n"
    );

    printf(
        "    -x PORTS        Exclude ports, e.g. "
        "-x 22,80,8000-9000\n"
    );

    printf(
        "    -i PORTS        Only check these ports, "
        "e.g. -i 80,443\n"
    );

    printf(
        "    -t, --threads N Squadron count (default: 16, "
        "max: %d; larger values are clamped)\n",
        MAX_THREADS_CAP
    );

    printf(
        "    --progress      Print periodic progress to "
        "stderr\n"
    );

    printf(
        "    -h, --help      Show this help message\n"
    );
}

static void print_results(
    FILE *out,
    const app_config_t *config,
    const port_list_t *ports,
    int total_available
)
{
    if (config->format == FORMAT_COUNT) {
        fprintf(
            out,
            "%d\n",
            total_available
        );

        return;
    }

    if (config->format == FORMAT_JSON) {
        fprintf(out, "{\n");

        fprintf(
            out,
            "    \"start_port\": %d,\n",
            config->start_port
        );

        fprintf(
            out,
            "    \"end_port\": %d,\n",
            config->end_port
        );

        fprintf(
            out,
            "    \"available_ports\": %d\n",
            total_available
        );

        fprintf(out, "}\n");

        return;
    }

    if (config->list_ports) {
        print_port_list(out, ports);
    }

    fprintf(
        out,
        "Total available ports: %d\n",
        total_available
    );
}

static app_config_t parse_arguments(int argc, char *argv[])
{
    app_config_t config = {
        .start_port = DEFAULT_START_PORT,
        .end_port = DEFAULT_END_PORT,
        .format = FORMAT_HUMAN,
        .list_ports = true,
        .output_path = NULL,
        .thread_count = MAX_THREADS,
        .progress = false,
        .has_include = false,
        .has_exclude = false
    };

    port_set_init(&config.include_ports);
    port_set_init(&config.exclude_ports);

    /* Tri-state: -1 means the user did not choose; resolve from the format. */
    int list_flag = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 >= argc) {
                fprintf(
                    stderr,
                    "Error: Invalid range format.\n"
                    "Use: -r START-END\n"
                );

                exit(EXIT_FAILURE);
            }

            if (
                !parse_port_range(
                    argv[i + 1],
                    &config.start_port,
                    &config.end_port
                )
            ) {
                fprintf(
                    stderr,
                    "Error: Invalid range format: %s\n"
                    "Use: -r START-END\n",
                    argv[i + 1]
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

        else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(
                    stderr,
                    "Error: Option -o requires a file path.\n"
                    "Run 'anthill --help' for usage information.\n"
                );

                exit(EXIT_FAILURE);
            }

            config.output_path = argv[i + 1];
            i++;
        }

        else if (strcmp(argv[i], "-x") == 0) {
            if (i + 1 >= argc) {
                fprintf(
                    stderr,
                    "Error: Option -x requires a port list.\n"
                    "Run 'anthill --help' for usage information.\n"
                );

                exit(EXIT_FAILURE);
            }

            if (!parse_port_list(argv[i + 1], &config.exclude_ports)) {
                fprintf(
                    stderr,
                    "Error: Invalid port list for -x: %s\n"
                    "Use comma-separated ports and ranges, "
                    "e.g. -x 22,80,8000-9000\n",
                    argv[i + 1]
                );

                exit(EXIT_FAILURE);
            }

            config.has_exclude = true;
            i++;
        }

        else if (strcmp(argv[i], "-i") == 0) {
            if (i + 1 >= argc) {
                fprintf(
                    stderr,
                    "Error: Option -i requires a port list.\n"
                    "Run 'anthill --help' for usage information.\n"
                );

                exit(EXIT_FAILURE);
            }

            if (!parse_port_list(argv[i + 1], &config.include_ports)) {
                fprintf(
                    stderr,
                    "Error: Invalid port list for -i: %s\n"
                    "Use comma-separated ports and ranges, "
                    "e.g. -i 80,443\n",
                    argv[i + 1]
                );

                exit(EXIT_FAILURE);
            }

            config.has_include = true;
            i++;
        }

        else if (
            strcmp(argv[i], "-t") == 0 ||
            strcmp(argv[i], "--threads") == 0
        ) {
            if (i + 1 >= argc) {
                fprintf(
                    stderr,
                    "Error: Option -t requires a thread count.\n"
                    "Run 'anthill --help' for usage information.\n"
                );

                exit(EXIT_FAILURE);
            }

            if (!parse_thread_count(argv[i + 1], &config.thread_count)) {
                fprintf(
                    stderr,
                    "Error: Invalid thread count: %s\n"
                    "Use: -t N with N between 1 and %d\n",
                    argv[i + 1],
                    MAX_THREADS_CAP
                );

                exit(EXIT_FAILURE);
            }

            i++;
        }

        else if (strcmp(argv[i], "--progress") == 0) {
            config.progress = true;
        }

        else if (strcmp(argv[i], "--list") == 0) {
            list_flag = 1;
        }

        else if (strcmp(argv[i], "--no-list") == 0) {
            list_flag = 0;
        }

        else if (
            strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0
        ) {
            print_help();

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

    config.list_ports = list_flag < 0
        ? config.format == FORMAT_HUMAN
        : list_flag != 0;

    return config;
}


int main(int argc, char *argv[])
{
    app_config_t config = parse_arguments(argc, argv);

    FILE *out = stdout;

    if (config.output_path != NULL) {
        out = fopen(config.output_path, "w");

        if (out == NULL) {
            fprintf(
                stderr,
                "Error: Cannot write to output file: %s\n",
                config.output_path
            );

            return EXIT_FAILURE;
        }
    }

    if (config.format == FORMAT_HUMAN) {
        fprintf(stderr, "Spawning Anthill...\n");
    }

    init_network_workers(config.format == FORMAT_HUMAN);
    int total_available = 0;
    anthill_mutex_t counter_mutex;
    anthill_mutex_init(&counter_mutex);
    int total_ports_to_scan =
        config.end_port -
        config.start_port +
        1;
    int requested_threads = config.thread_count > 0
        ? config.thread_count
        : MAX_THREADS;
    int active_threads =
        total_ports_to_scan < requested_threads
            ? total_ports_to_scan
            : requested_threads;

    /* Sized from the live count so a raised ceiling cannot overflow. */
    anthill_thread_t *threads = (anthill_thread_t *)malloc(
        (size_t)active_threads * sizeof(anthill_thread_t)
    );
    worker_args_t *thread_args = (worker_args_t *)malloc(
        (size_t)active_threads * sizeof(worker_args_t)
    );

    if (threads == NULL || thread_args == NULL) {
        fprintf(
            stderr,
            "Error: Could not allocate %d squadrons.\n",
            active_threads
        );

        free(threads);
        free(thread_args);

        anthill_mutex_destroy(&counter_mutex);

        cleanup_network_workers();

        if (out != stdout) {
            fclose(out);
        }

        return EXIT_FAILURE;
    }

    int scanned_total = 0;
    progress_state_t progress = {
        .scanned = &scanned_total,
        .total = total_ports_to_scan,
        .last_percent = -1,
        .mutex = &counter_mutex
    };

    int ports_per_thread =
        total_ports_to_scan / active_threads;

    const port_set_t *include_ports =
        config.has_include
            ? &config.include_ports
            : NULL;

    const port_set_t *exclude_ports =
        config.has_exclude
            ? &config.exclude_ports
            : NULL;


    if (config.format == FORMAT_HUMAN) {
        fprintf(
            stderr,
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
        args->include_ports = include_ports;
        args->exclude_ports = exclude_ports;
        args->format = config.format;
        args->on_progress = config.progress ? report_progress : NULL;
        args->progress_context = config.progress ? &progress : NULL;
        args->failed = false;

        port_list_init(&args->results);


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

            for (int j = 0; j < created_threads; j++) {
                port_list_free(&thread_args[j].results);
            }

            free(threads);
            free(thread_args);

            anthill_mutex_destroy(&counter_mutex);

            cleanup_network_workers();

            if (out != stdout) {
                fclose(out);
            }

            return EXIT_FAILURE;
        }

        created_threads++;
    }

    for (int i = 0; i < created_threads; i++) {
        anthill_thread_join(threads[i]);
    }

    anthill_mutex_destroy(&counter_mutex);

    /*
     * Merge squadron listings. Each worker owns its own buffer and its slot in
     * thread_args, so no lock is needed here. Squadrons already cover disjoint
     * ascending sub-ranges, but a final sort guarantees strictly ascending
     * output even if ranges or filters change later.
     */
    port_list_t merged_ports;
    port_list_init(&merged_ports);

    bool merge_failed = false;

    for (int i = 0; i < created_threads; i++) {
        worker_args_t *args = &thread_args[i];

        if (!merge_failed) {
            for (int j = 0; j < args->results.count; j++) {
                if (!port_list_push(&merged_ports, args->results.ports[j])) {
                    merge_failed = true;
                    break;
                }
            }
        }

        if (args->failed) {
            merge_failed = true;
        }

        port_list_free(&args->results);
    }

    if (merge_failed) {
        fprintf(
            stderr,
            "Error: Ran out of memory while collecting results.\n"
        );

        port_list_free(&merged_ports);

        free(threads);
        free(thread_args);

        cleanup_network_workers();

        if (out != stdout) {
            fclose(out);
        }

        return EXIT_FAILURE;
    }

    port_list_sort(&merged_ports);

    if (config.format == FORMAT_HUMAN) {
        fprintf(
            stderr,
            "\nAnthill dormant. All ants returned.\n"
        );
    }

    print_results(
        out,
        &config,
        &merged_ports,
        total_available
    );

    port_list_free(&merged_ports);

    free(threads);
    free(thread_args);

    if (out != stdout) {
        fclose(out);
    }

    cleanup_network_workers();

    return EXIT_SUCCESS;
}