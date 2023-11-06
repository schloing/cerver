// virtually identical to vanilla

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8000
#define SIZE 1024

static volatile int cont = 1;
static char*        message;

int create_socket() {
    struct sockaddr_in addr;

    int server_socket    = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_res   = bind(server_socket, (struct sockaddr*)&addr, sizeof(addr));
    int listen_res = listen(server_socket, 5);

    printf("created socket!\n");

    // reimplement error handling

    return server_socket;
}

int wait_client(int server_socket) {
    struct sockaddr_in cliaddr;

    printf("entered wait_client\n");

    int addrlen       = sizeof(cliaddr);
    int client_socket = accept(server_socket, (struct sockaddr*)&cliaddr, &addrlen);

    printf("accept success %s\n", inet_ntoa(cliaddr.sin_addr));

    return client_socket;
}

void socket_handler(int epoll_fd, struct epoll_event* events,
                    int* socket_desc) {
    int  client_socket = *socket_desc;
    char buf[SIZE];

    int epoll_events = epoll_wait(epoll_fd, events, 1, -1);
    if (epoll_events == 0) return;

    if (events[0].events & EPOLLIN) {
        int bufsiz = read(client_socket, buf, SIZE - 1);

        if (bufsiz <= 0) { goto end; }

        buf[bufsiz] = '\0';

        printf("\nreceived: %s\n", buf);

        send(client_socket, message, strlen(message), 0);

        if (strncmp(buf, "end", 3) == 0);

end:
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
        close(client_socket);
    }
}

void abort_all(int s) {
    printf("ctrl+c, aborting all\n");
    cont = 0;
}

int main() {
    int   server_socket = create_socket();
    int   epoll_fd      = epoll_create1(0);
    FILE* response      = fopen("response", "r");

    fcntl(server_socket, F_SETFL, O_NONBLOCK);
   
    struct epoll_event event;

    event.events  = EPOLLIN;
    event.data.fd = server_socket;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event);

    /* static char* message */

    fseek(response, 0, SEEK_END);
    
    size_t response_length = ftell(response);
    
    message = (char*)malloc(response_length + 1);

    fseek(response, 0, SEEK_SET);

    fread(message, 1, response_length, response);
    message[response_length] = '\0';

    printf("message: %s\n", message);

    fclose(response);

    struct epoll_event events[10];

    while (cont) {
        int num_events = epoll_wait(epoll_fd, events, 10, 1);

        for (int i = 0; i < num_events; i++) {
            int fd = events[i].data.fd;

            if (fd == server_socket) {
                int client_socket = wait_client(server_socket);

                fcntl(client_socket, F_SETFL, O_NONBLOCK);
                
                event.events  = EPOLLIN | EPOLLET;
                event.data.fd = client_socket;
               
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
            }
            else {
                socket_handler(epoll_fd, events, &fd);
            }
        }

        if (!cont) { goto cleanup; }
    }

cleanup:
    close(epoll_fd);
    close(server_socket);
    free(message);

    return 0;
}
