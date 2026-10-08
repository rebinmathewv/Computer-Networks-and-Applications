
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUF_SIZE 1024
#define SERVER_IP "127.0.0.1"
#define GENERATOR "10011"          /* must be identical to server's generator */

/* XOR of two bit strings of equal length */
void xor_bits(const char *a, const char *b, char *result, int len) {
    for (int i = 0; i < len; i++)
        result[i] = (a[i] == b[i]) ? '0' : '1';
    result[len] = '\0';
}

/* Modulo-2 division: divides 'dividend' by 'generator', stores remainder */
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
            char zeros[BUF_SIZE];
            memset(zeros, '0', genLen);
            zeros[genLen] = '\0';
            xor_bits(temp, zeros, xored, genLen);
        }

        memmove(temp, xored + 1, genLen - 1);
        if (i < divLen)
            temp[genLen - 1] = dividend[i];
        else
            temp[genLen - 1] = '\0';
        temp[genLen] = '\0';
    }

    strncpy(remainder, temp, genLen - 1);
    remainder[genLen - 1] = '\0';
}

/* Returns 1 if the bit string is all zeros, else 0 */
int is_all_zero(const char *s) {
    for (int i = 0; s[i] != '\0'; i++)
        if (s[i] != '0')
            return 0;
    return 1;
}

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char codeword[BUF_SIZE];
    char remainder[BUF_SIZE];
    char verdict[BUF_SIZE];

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

    /* 4. Receive the codeword (data + CRC) from the server */
    memset(codeword, 0, BUF_SIZE);
    int n = recv(sock_fd, codeword, BUF_SIZE - 1, 0);
    if (n <= 0) {
        perror("Failed to receive codeword");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    codeword[n] = '\0';

    printf("Codeword received: %s\n", codeword);

    /* 5. Perform Modulo-2 division on the received codeword */
    mod2_divide(codeword, GENERATOR, remainder);

    printf("Remainder after division: %s\n", remainder);

    /* 6. Decide whether an error occurred */
    int genLen = strlen(GENERATOR);
    int dataLen = strlen(codeword) - (genLen - 1);

    if (is_all_zero(remainder)) {
        char data[BUF_SIZE];
        strncpy(data, codeword, dataLen);
        data[dataLen] = '\0';
        printf("Result: NO ERROR detected.\n");
        printf("Original data recovered: %s\n", data);
        snprintf(verdict, BUF_SIZE, "OK - No error. Data = %s", data);
    } else {
        printf("Result: ERROR detected in transmission! Data is corrupted.\n");
        snprintf(verdict, BUF_SIZE, "ERROR DETECTED - remainder = %s", remainder);
    }

    /* 7. Send the verdict back to the server */
    send(sock_fd, verdict, strlen(verdict), 0);

    close(sock_fd);
    return 0;
}
