#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUF_SIZE 1024
#define SERVER_IP "127.0.0.1"   /* change if server runs on another machine */

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char message[BUF_SIZE];
    char reply[BUF_SIZE];

    /* 1. Create socket */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* 2. Prepare server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    /* 3. Connect to the server */
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    /* 4. Get string input from the user */
    printf("Enter a message to send to the server: ");
    fgets(message, BUF_SIZE, stdin);

    /* remove trailing newline, if any */
    message[strcspn(message, "\n")] = '\0';

    /* 5. Send the string to the server */
    send(sock_fd, message, strlen(message), 0);

    /* 6. Receive confirmation from the server */
    memset(reply, 0, BUF_SIZE);
    int bytes_received = recv(sock_fd, reply, BUF_SIZE - 1, 0);
    if (bytes_received > 0) {
        reply[bytes_received] = '\0';
        printf("Server response: %s\n", reply);
    } else {
        printf("No response received from server.\n");
    }

    /* 7. Close the socket */
    close(sock_fd);
    return 0;
}
