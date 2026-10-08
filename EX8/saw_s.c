#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 5000
#define BUFFER_SIZE 1024

typedef struct
{
    int seq;
    int length;
    char data[BUFFER_SIZE];
} Frame;

typedef struct
{
    int front;
    int rear;
    Frame items[100];
} LinearQueue;

/* ---------- QUEUE ---------- */

void initQueue(LinearQueue *q)
{
    q->front = 0;
    q->rear = -1;
}

void enqueue(LinearQueue *q, Frame f)
{
    if (q->rear < 99)
        q->items[++q->rear] = f;
}

int isEmpty(LinearQueue *q)
{
    return q->front > q->rear;
}

/* ---------- RECEIVE ALL ---------- */

int recvAll(int sock, char *buffer, int size)
{
    int total = 0;

    while (total < size)
    {
        int n = recv(sock,
                     buffer + total,
                     size - total,
                     0);

        if (n <= 0)
            return 0;

        total += n;
    }

    return 1;
}

/* ---------- SEND ALL ---------- */

int sendAll(int sock, char *buffer, int size)
{
    int total = 0;

    while (total < size)
    {
        int n = send(sock,
                     buffer + total,
                     size - total,
                     0);

        if (n <= 0)
            return 0;

        total += n;
    }

    return 1;
}

/* ---------- MAIN ---------- */

int main()
{
    int serverSocket;
    int clientSocket;

    struct sockaddr_in server;
    struct sockaddr_in client;

    socklen_t clientSize =
        sizeof(client);

    LinearQueue receivedQueue;

    printf("============================================\n");
    printf("      STOP-AND-WAIT ARQ - RECEIVER\n");
    printf("============================================\n");

    /* Create socket */

    serverSocket = socket(AF_INET,
                          SOCK_STREAM,
                          0);

    if (serverSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /* Allow port reuse */

    int opt = 1;

    setsockopt(serverSocket,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    /* Server address */

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    /* Bind */

    if (bind(serverSocket,
             (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    /* Listen */

    if (listen(serverSocket, 1) < 0)
    {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    printf("Waiting for sender...\n");

    /* Accept sender */

    clientSocket = accept(serverSocket,
                          (struct sockaddr *)&client,
                          &clientSize);

    if (clientSocket < 0)
    {
        perror("Accept failed");
        close(serverSocket);
        return 1;
    }

    printf("Sender connected.\n\n");

    initQueue(&receivedQueue);

    /*
     * Receiver initially expects Frame 0.
     */

    int expectedSeq = 0;

    /* ---------- RECEIVE FRAMES ---------- */

    while (1)
    {
        Frame frame;

        /*
         * Receive one complete frame.
         */

        if (!recvAll(clientSocket,
                     (char *)&frame,
                     sizeof(Frame)))
        {
            printf("Sender disconnected.\n");
            break;
        }

        /* END frame */

        if (frame.seq == -1)
        {
            printf("\nTransmission completed.\n");
            break;
        }

        printf("\n--------------------------------------------\n");

        printf("Frame %d received.\n",
               frame.seq);

        printf("Data : %s\n",
               frame.data);

        /*
         * Check whether this is
         * the expected frame.
         */

        if (frame.seq == expectedSeq)
        {
            printf("Frame %d is expected.\n",
                   frame.seq);

            printf("Delivering data to Network Layer...\n");

            enqueue(&receivedQueue,
                    frame);

            /*
             * Send ACK for NEXT frame.
             */

            int ack =
                (expectedSeq + 1) % 2;

            if (!sendAll(clientSocket,
                         (char *)&ack,
                         sizeof(int)))
            {
                printf("Failed to send ACK.\n");
                break;
            }

            printf("ACK %d sent.\n",
                   ack);

            /*
             * Change expected sequence number.
             */

            expectedSeq =
                (expectedSeq + 1) % 2;
        }

        /*
         * Duplicate frame.
         */

        else
        {
            printf("DUPLICATE FRAME!\n");

            printf("Frame %d discarded.\n",
                   frame.seq);

            /*
             * Send previous ACK again.
             */

            int ack = expectedSeq;

            if (!sendAll(clientSocket,
                         (char *)&ack,
                         sizeof(int)))
            {
                printf("Failed to retransmit ACK.\n");
                break;
            }

            printf("ACK %d retransmitted.\n",
                   ack);
        }
    }

    /* ---------- DISPLAY DATA ---------- */

    printf("\n============================================\n");

    printf("Received Data:\n");

    printf("--------------------------------------------\n");

    while (!isEmpty(&receivedQueue))
    {
        Frame f =
            receivedQueue.items[
                receivedQueue.front++
            ];

        printf("%s", f.data);
    }

    printf("\n============================================\n");

    /* Close sockets */

    close(clientSocket);
    close(serverSocket);

    return 0;
}
