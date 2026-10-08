#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[16384];
    socklen_t addr_len = sizeof(serv_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("Socket creation error\n");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    char base_ip[30];
    int num_subnets;

    printf("Enter base IP address block (e.g., 192.168.1.0): ");
    scanf("%s", base_ip);
    printf("Enter number of subnets (using 2^x rule partition): ");
    scanf("%d", &num_subnets);

    sprintf(buffer, "%s %d", base_ip, num_subnets);

    // Initial transmission to main server port 8080
    sendto(sock, buffer, strlen(buffer) + 1, 0, (struct sockaddr *)&serv_addr, addr_len);

    // Receive initial subnet table (serv_addr is updated to thread's new ephemeral port)
    memset(buffer, 0, sizeof(buffer));
    recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&serv_addr, &addr_len);
    printf("\n--- Calculated Subnet Table (Concurrent Session) ---\n%s\n", buffer);

    // Interactive allocation loop with thread session
    while (1) {
        int target_subnet;
        printf("Enter subnet number to request an IP slot allocation (or enter 0 to exit): ");
        scanf("%d", &target_subnet);

        if (target_subnet == 0) {
            int exit_signal = -1;
            sendto(sock, &exit_signal, sizeof(exit_signal), 0, (struct sockaddr *)&serv_addr, addr_len);
            break;
        }

        // Sends datagram to the dedicated session port updated in serv_addr
        sendto(sock, &target_subnet, sizeof(target_subnet), 0, (struct sockaddr *)&serv_addr, addr_len);

        memset(buffer, 0, sizeof(buffer));
        recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&serv_addr, &addr_len);
        printf("Server Response -> %s\n\n", buffer);
    }

    close(sock);
    printf("Disconnected.\n");
    return 0;
}
