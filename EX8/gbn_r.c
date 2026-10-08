#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8081
#define MAX_FRAMES 100

typedef struct
{
    int seq;
    int length;
    char data[100];
} Frame;

typedef struct
{
    int front;
    int rear;
    Frame items[MAX_FRAMES];
} LinearQueue;

/* ---------- QUEUE ---------- */

void initQueue(LinearQueue *q)
{
    q->front = 0;
    q->rear = -1;
}

void enqueue(LinearQueue *q, Frame f)
{
    if (q->rear < MAX_FRAMES - 1)
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
    printf("         GO-BACK-N ARQ - RECEIVER\n");
    printf("============================================\n");

    /* ---------- CREATE SOCKET ---------- */

    serverSocket = socket(AF_INET,
                          SOCK_STREAM,
                          0);

    if (serverSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /* ---------- REUSE PORT ---------- */

    int opt = 1;

    if (setsockopt(serverSocket,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(serverSocket);
        return 1;
    }

    /* ---------- SERVER ADDRESS ---------- */

    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;

    server.sin_addr.s_addr =
        INADDR_ANY;

    server.sin_port =
        htons(PORT);

    /* ---------- BIND ---------- */

    if (bind(serverSocket,
             (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    /* ---------- LISTEN ---------- */

    if (listen(serverSocket, 1) < 0)
    {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    printf("Waiting for sender...\n");

    /* ---------- ACCEPT ---------- */

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
     * Receiver starts expecting Frame 0.
     *
     * expectedSeq = Rn
     */

    int expectedSeq = 0;

    /* ---------- RECEIVE FRAMES ---------- */

    while (1)
    {
        Frame frame;

        /*
         * Receive complete frame.
         */

        if (!recvAll(clientSocket,
                     (char *)&frame,
                     sizeof(Frame)))
        {
            printf("Sender disconnected.\n");
            break;
        }

        /* ---------- END FRAME ---------- */

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

        /* ---------- EXPECTED FRAME ---------- */

        if (frame.seq == expectedSeq)
        {
            printf("Frame %d is expected.\n",
                   frame.seq);

            printf("Delivering Frame %d.\n",
                   frame.seq);

            /*
             * Store frame.
             */

            enqueue(&receivedQueue,
                    frame);

            /*
             * Move receiver to
             * next expected frame.
             */

            expectedSeq++;

            /*
             * Cumulative ACK.
             *
             * ACK = next expected frame.
             */

            int ack = expectedSeq;

            if (!sendAll(clientSocket,
                         (char *)&ack,
                         sizeof(int)))
            {
                printf("Failed to send ACK.\n");
                break;
            }

            printf("Cumulative ACK %d sent.\n",
                   ack);
        }

        /* ---------- OUT OF ORDER ---------- */

        else
        {
            printf("\n*** OUT-OF-ORDER FRAME! ***\n");

            printf("Expected Frame %d.\n",
                   expectedSeq);

            printf("Received Frame %d.\n",
                   frame.seq);

            printf("Frame %d discarded.\n",
                   frame.seq);

            /*
             * Receiver does NOT buffer
             * out-of-order frames.
             *
             * It sends the ACK for the
             * last correctly received
             * sequence.
             */

            int ack = expectedSeq;

            if (!sendAll(clientSocket,
                         (char *)&ack,
                         sizeof(int)))
            {
                printf("Failed to send duplicate ACK.\n");
                break;
            }

            printf("Duplicate cumulative ACK %d sent.\n",
                   ack);
        }
    }

    /* ---------- DISPLAY DATA ---------- */

    printf("\n============================================\n");
    printf("FINAL RECEIVED DATA\n");
    printf("============================================\n");

    while (!isEmpty(&receivedQueue))
    {
        Frame f =
            receivedQueue.items[
                receivedQueue.front++
            ];

        printf("%s", f.data);
    }

    printf("\n============================================\n");

    /* ---------- CLOSE ---------- */

    close(clientSocket);
    close(serverSocket);

    return 0;
}
