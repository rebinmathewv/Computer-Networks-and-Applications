#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUF_SIZE 1024
#define GENERATOR "10011"          /* fixed generator polynomial (bit string) */

/* XOR of two bit strings of equal length (returns result in 'result') */
void xor_bits(const char *a, const char *b, char *result, int len) {
    for (int i = 0; i < len; i++)
        result[i] = (a[i] == b[i]) ? '0' : '1';
    result[len] = '\0';
}

/*
 * Performs Modulo-2 (binary) division of 'dividend' by 'generator'
 * and stores the CRC remainder (length = genLen - 1) in 'remainder'.
 */
void mod2_divide(const char *dividend, const char *generator, char *remainder) {
    int genLen = strlen(generator);
    int divLen = strlen(dividend);

    char temp[BUF_SIZE];
    strncpy(temp, dividend, genLen);
    temp[genLen] = '\0';

    for (int i = genLen; i <= divLen; i++) {
        char xored[BUF_SIZE];

        if (temp[0] == '1') {
            xor_bits(temp, generator, xored, genLen);
        } else {
            /* XOR with all zeros -> just shift, drop the leading bit */
            char zeros[BUF_SIZE];
            memset(zeros, '0', genLen);
            zeros[genLen] = '\0';
            xor_bits(temp, zeros, xored, genLen);
        }

        /* drop the leading (already-processed) bit, bring down next bit */
        memmove(temp, xored + 1, genLen - 1);
        if (i < divLen)
            temp[genLen - 1] = dividend[i];
        else
            temp[genLen - 1] = '\0';
        temp[genLen] = '\0';
    }

    /* what's left in temp (minus the extra bring-down slot) is the remainder */
    strncpy(remainder, temp, genLen - 1);
    remainder[genLen - 1] = '\0';
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char data[BUF_SIZE];
    char remainder[BUF_SIZE];
    char codeword[BUF_SIZE];

    /* 1. Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. Prepare server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* 3. Bind */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* 4. Listen */
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);
    printf("Generator polynomial: %s\n", GENERATOR);

    while (1) {
        /* 5. Accept a client connection */
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        /* 6. Get binary data string from the user */
        printf("Enter binary data to send (e.g. 1101011011): ");
        fgets(data, BUF_SIZE, stdin);
        data[strcspn(data, "\n")] = '\0';

        /* 7. Compute CRC remainder: dividend = data + (genLen-1) zeros */
        int genLen = strlen(GENERATOR);
        char dividend[BUF_SIZE];
        snprintf(dividend, BUF_SIZE, "%s", data);
        int dataLen = strlen(data);
        for (int i = 0; i < genLen - 1; i++)
            dividend[dataLen + i] = '0';
        dividend[dataLen + genLen - 1] = '\0';

        mod2_divide(dividend, GENERATOR, remainder);

        /* 8. Form codeword = data + CRC remainder */
        snprintf(codeword, BUF_SIZE, "%s%s", data, remainder);

        printf("Data           : %s\n", data);
        printf("CRC remainder  : %s\n", remainder);
        printf("Codeword sent  : %s\n", codeword);

        /* 9. Send the codeword to the client over TCP */
        send(client_fd, codeword, strlen(codeword), 0);

        /* 10. Receive verification result from client */
        char verdict[BUF_SIZE];
        memset(verdict, 0, BUF_SIZE);
        int n = recv(client_fd, verdict, BUF_SIZE - 1, 0);
        if (n > 0) {
            verdict[n] = '\0';
            printf("Client's verdict: %s\n", verdict);
        }

        close(client_fd);
        printf("--------------------------------------------------\n");
    }

    close(server_fd);
    return 0;
}
