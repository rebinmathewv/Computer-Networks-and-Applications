#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define PORT 5000
#define BUFFER_SIZE 1024
#define FRAME_SIZE 5
#define TIMEOUT_MS 3000

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

int isEmpty(LinearQueue *q)
{
    return q->front > q->rear;
}

void enqueue(LinearQueue *q, Frame f)
{
    if (q->rear < 99)
        q->items[++q->rear] = f;
}

Frame dequeue(LinearQueue *q)
{
    return q->items[q->front++];
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

/* ---------- MAIN ---------- */

int main()
{
    int clientSocket;

    struct sockaddr_in server;

    LinearQueue queue;

    char message[BUFFER_SIZE];

    printf("============================================\n");
    printf("       STOP-AND-WAIT ARQ - SENDER\n");
    printf("============================================\n");

    /* Create socket */

    clientSocket = socket(AF_INET,
                          SOCK_STREAM,
                          0);

    if (clientSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /* Receiver address */

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Connecting to receiver...\n");

    if (connect(clientSocket,
                (struct sockaddr *)&server,
                sizeof(server)) < 0)
    {
        perror("Connection failed");
        close(clientSocket);
        return 1;
    }

    printf("Receiver connected.\n\n");

    initQueue(&queue);

    /* Enter message */

    printf("Enter message: ");

    fgets(message,
          BUFFER_SIZE,
          stdin);

    message[strcspn(message, "\n")] = '\0';

    /* Create frames */

    int totalFrames = 0;
    int pos = 0;

    while (pos < strlen(message))
    {
        Frame f;

        f.seq = totalFrames % 2;
        f.length = 0;

        while (message[pos] != '\0' &&
               f.length < FRAME_SIZE)
        {
            f.data[f.length] = message[pos];
            f.length++;
            pos++;
        }

        f.data[f.length] = '\0';

        enqueue(&queue, f);

        totalFrames++;
    }

    printf("\nTotal Frames = %d\n",
           totalFrames);

    /* ---------- STOP AND WAIT ---------- */

    while (!isEmpty(&queue))
    {
        Frame frame = dequeue(&queue);

        int acknowledged = 0;

        /*
         * Keep sending the SAME frame
         * until correct ACK is received.
         */

        while (!acknowledged)
        {
            printf("\n--------------------------------------------\n");

            printf("Sending Frame %d\n",
                   frame.seq);

            printf("Data : %s\n",
                   frame.data);

            /* Send frame */

            if (!sendAll(clientSocket,
                         (char *)&frame,
                         sizeof(Frame)))
            {
                printf("Transmission failed.\n");
                close(clientSocket);
                return 1;
            }

            printf("Frame %d sent.\n",
                   frame.seq);

            /*
             * Wait for ACK for 3 seconds.
             */

            fd_set readfds;

            FD_ZERO(&readfds);
            FD_SET(clientSocket, &readfds);

            struct timeval timeout;

            timeout.tv_sec = TIMEOUT_MS / 1000;

            timeout.tv_usec =
                (TIMEOUT_MS % 1000) * 1000;

            int result = select(
                clientSocket + 1,
                &readfds,
                NULL,
                NULL,
                &timeout);

            /* Timeout */

            if (result == 0)
            {
                printf("TIMEOUT!\n");
                printf("No ACK received.\n");

                printf("Retransmitting Frame %d...\n",
                       frame.seq);

                continue;
            }

            /* Select error */

            if (result < 0)
            {
                perror("Select error");
                close(clientSocket);
                return 1;
            }

            /* Receive ACK */

            int ack;

            if (!recvAll(clientSocket,
                         (char *)&ack,
                         sizeof(int)))
            {
                printf("Receiver disconnected.\n");
                close(clientSocket);
                return 1;
            }

            printf("ACK %d received.\n",
                   ack);

            /*
             * ACK contains the sequence number
             * of the NEXT frame expected.
             */

            int expectedACK =
                (frame.seq + 1) % 2;

            if (ack == expectedACK)
            {
                printf("Correct ACK.\n");

                printf("Frame %d successfully delivered.\n",
                       frame.seq);

                acknowledged = 1;
            }
            else
            {
                printf("Wrong/duplicate ACK.\n");

                printf("Waiting for correct ACK...\n");
            }
        }
    }

    /* ---------- END FRAME ---------- */

    Frame endFrame;

    endFrame.seq = -1;
    endFrame.length = 0;

    strcpy(endFrame.data, "END");

    sendAll(clientSocket,
            (char *)&endFrame,
            sizeof(Frame));

    printf("\n============================================\n");
    printf("All frames transmitted successfully.\n");
    printf("============================================\n");

    close(clientSocket);

    return 0;
}

