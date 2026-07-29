#include <stdio.h>
#include "../include/net_utils.h"

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

int main(void) {
    printf("Spawing Anthil...\n");
    init_network_workers();

    printf("Scout ants deploying to port 1 through 65335...\n");
    int available_count = 0;

    for (int port = 1; port <=65535; port++){
        if (check_port_availability(port)) {
            printf("Port: %d: [AVAILABLE]\n", port);
            available_count++;
        }
    }

    printf("Anthill dormant. All ants returned.\n");
    printf("Total available ports on localhost: %d\n", available_count);
    cleanup_network_workers();

    return 0;
}
