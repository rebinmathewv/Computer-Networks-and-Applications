#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char str[1000];

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

    if (inet_pton(AF_INET, SERVER_IP,
                  &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    /* 3. Connect to the server */
    if (connect(sock_fd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    /* 4. Receive string from server */
    int n = recv(sock_fd,
                 str,
                 sizeof(str) - 1,
                 0);

    if (n <= 0) {
        perror("Failed to receive string");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    str[n] = '\0';

    printf("String received from server: %s\n", str);

    /* 5. Reverse the string */
    int i = 0;
    int j = strlen(str) - 1;

    while (i < j) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;

        i++;
        j--;
    }

    /* 6. Print reversed string */
    printf("Reversed string = %s\n", str);

    /* 7. Send reversed string back to server */
    send(sock_fd, str, strlen(str) + 1, 0);

    /* 8. Cleanup */
    close(sock_fd);

    return 0;
}
