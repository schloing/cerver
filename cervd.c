#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORTNUM 2300

int main() {
    FILE* response = fopen("response", "r");

    if (response == NULL) {
        perror("response");
        goto exit_response;
    }

    fseek(response, 0, SEEK_END); long fsize = ftell(response);
    fseek(response, 0, SEEK_SET);

    char* msg = (char*)malloc(fsize + 1);

    fread(msg, 1, fsize, response);

    // null terminate the buffer
    msg[fsize] = '\0';

    struct sockaddr_in dest;
    struct sockaddr_in serv;

    socklen_t socksize = sizeof(struct sockaddr_in);
    int       mysocket;
    int       consocket;

    memset(&serv, 0, sizeof(serv));

    serv.sin_family      = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port        = htons(PORTNUM);

    printf("server @ %s:%d\n", inet_ntoa(serv.sin_addr), PORTNUM);

    if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        goto exit_socket;
    }

    bind(mysocket, (struct sockaddr*)&serv, sizeof(struct sockaddr));

    if (listen(mysocket, 1) == -1) {
        perror("listen");
        goto exit_socket;
    }
    
    if ((consocket = accept(mysocket, (struct sockaddr*)&dest, &socksize)) == -1) {
        perror("accept");
        goto exit_socket;
    }

    while (consocket) {
        printf("new client @ %s\n", inet_ntoa(dest.sin_addr));

        send(consocket, msg, strlen(msg), 0);
        close(consocket);
        
        if ((consocket = accept(mysocket, (struct sockaddr*)&dest, &socksize)) == -1) {
            perror("accept (while)");
            goto exit_socket;
        }
    }

exit_socket:
    close(mysocket);
    free(msg);

exit_response:
    fclose(response);

    return EXIT_SUCCESS;
}
