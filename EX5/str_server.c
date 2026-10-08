#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    /* The string to send to the client */
    char str[100] = "HELLO WORLD";

    /* 1. Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Allow immediate reuse of port after restart */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    /* 2. Prepare server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* 3. Bind socket to address and port */
    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* 4. Listen for incoming connections */
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {

        /* 5. Accept a client connection */
        client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &client_len);

        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s\n",
               inet_ntoa(client_addr.sin_addr));

        /* 6. Send the string to the client */
        send(client_fd, str, strlen(str) + 1, 0);

        printf("String sent to client: %s\n", str);

        /* 7. Receive the reversed string from client */
        char reversed[100];

        int bytes_received = recv(client_fd,
                                  reversed,
                                  sizeof(reversed) - 1,
                                  0);

        if (bytes_received > 0) {
            reversed[bytes_received] = '\0';

            printf("Reversed string received from client: %s\n",
                   reversed);
        }

        /* 8. Close connection with this client */
        close(client_fd);
    }

    close(server_fd);

    return 0;
}
