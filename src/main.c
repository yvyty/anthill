#include <stdio.h>
#include "../include/net_utils.h"

#define NUM_THREADS 16
#define MAX_PORT 65535

typedef struct {
    int squadron_id;
    int start_port;
    int end_port;
    int *total_available;
    anthill_mutex_t *mutex;
} worker_args_t;

bool check_port_availability(int port){
    socket_fd_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if(!VALID_SOCKET(socket)){
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    CLOSE_SOCKET(sock);

    return (result == 0);
}

ANTHILL_THREAD_FUNC ant_worker(void *arg) {
    worker_args_t *args = (worker_args_t *)arg;
    int local_count = 0;

    for (int p = args->start_port; p <= args->end_port; p++){
        if(check_port_availability(p)) {
            local_count++;
        }
    }

    anthill_mutex_lock(args->mutex);
    *(args->total_available) += local_count;
    anthill_mutex_unlock(args->mutex);

    printf("    -> Squadron %02d completed (Ports %d to %d). \n", args->squadron_id, args->start_port, args->end_port);

    return (ANTHILL_THREAD_RETURN)0;
}

int main(void) {
    printf("Spawing Anthil...\n");
    init_network_workers();

    int total_available = 0;
    anthill_mutex_t counter_mutex;
    anthill_mutex_init(&counter_mutex);

    anthill_thread_t threads[NUM_THREADS];
    worker_args_t thread_args[NUM_THREADS];

    int ports_per_thread = MAX_PORT / NUM_THREADS;

    printf("\nDeploying %d ant squadrons...\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].squadron_id = i + 1;
        thread_args[i].start_port = (i * ports_per_thread) + 1;

        thread_args[i].end_port = (i == NUM_THREADS - 1) ? MAX_PORT : ((i + 1) * ports_per_thread);
        thread_args[i].total_available = &total_available;
        thread_args[i].mutex = &counter_mutex;

        anthill_thread_create(&threads[i], ant_worker, &thread_args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        anthill_thread_join(threads[i]);
    }

    anthill_mutex_destroy(&counter_mutex);
    printf("Anthill dormant. All ants returned.\n");
    printf("Total available ports on localhost: %d\n", total_available);

    cleanup_network_workers();
    return 0;
}
