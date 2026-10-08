
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUF_SIZE];
    char reply[BUF_SIZE];
    FILE *fp;

    /* 1. Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Allow immediate reuse of port after restart */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. Prepare server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   /* listen on all interfaces */
    server_addr.sin_port = htons(PORT);

    /* 3. Bind socket to address and port */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* 4. Listen for incoming connections (waiting queue size = 5) */
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        /* 5. Accept a client connection */
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        /* 6. Receive the string from the client */
        memset(buffer, 0, BUF_SIZE);
        int bytes_received = recv(client_fd, buffer, BUF_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected without sending data.\n");
            close(client_fd);
            continue;
        }
        buffer[bytes_received] = '\0';

        printf("Received string: %s\n", buffer);

        /* 7. Store the received string in a file (append mode) */
        fp = fopen("data.txt", "a");
        if (fp == NULL) {
            perror("File open failed");
            strcpy(reply, "ERROR: Server could not open file.");
        } else {
            fprintf(fp, "%s\n", buffer);
            fclose(fp);
            snprintf(reply, BUF_SIZE, "Message stored successfully in file: \"%s\"", buffer);
        }

        /* 8. Send confirmation back to the client */
        send(client_fd, reply, strlen(reply), 0);

        /* 9. Close connection with this client */
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

