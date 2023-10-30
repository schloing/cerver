#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8000
#define SIZE 1024

static volatile int cont = 1;

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

void* socket_handler(void* socket_desc) {

    int  client_socket = *(int*)socket_desc;

    char buf[SIZE];
    char msg[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n"
                 "<!DOCTYPE html><html> <head> <title>Hello World</title> </head> <body> <h1>Hello, World!</h1> </body> </html>"; // from response buffer
    
    while (cont) {
        int bufsiz = read(client_socket, buf, SIZE - 1);
    
        if (bufsiz <= 0) break;

        buf[bufsiz] = '\0';
       
        printf("\n\n%s\n\n", buf);
        fflush(stdout);

        send(client_socket, msg, strlen(msg), 0);

        if (strncmp(buf, "end", 3) == 0) break;
    }

    close(client_socket);
}

void abort_all(int s) {
    printf("ctrl+c, aborting all\n");
    cont = 0;
}

int main() {
    int server_socket = create_socket();
    
//  signal(SIGINT, abort_all);

    while (cont) {
        int       client_socket = wait_client(server_socket);
        int       flags         = fcntl(client_socket, F_GETFL, 0);
       
        fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

        pthread_t id;
        
        pthread_create(&id, NULL, (void*)socket_handler, (void*)&client_socket);
        pthread_detach(id);
    }

    return 0;
}
