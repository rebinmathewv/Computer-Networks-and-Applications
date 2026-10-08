#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define PORT 8081
#define MAX_FRAMES 100
#define FRAME_SIZE 5
#define WINDOW_SIZE 4
#define TIMEOUT 3

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

    Frame frames[MAX_FRAMES];

    char message[500];

    printf("============================================\n");
    printf("          GO-BACK-N ARQ - SENDER\n");
    printf("============================================\n");

    /* ---------- CREATE SOCKET ---------- */

    clientSocket = socket(AF_INET,
                          SOCK_STREAM,
                          0);

    if (clientSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /* ---------- RECEIVER ADDRESS ---------- */

    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    server.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    /* ---------- CONNECT ---------- */

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

    /* ---------- ENTER MESSAGE ---------- */

    printf("Enter message: ");

    fgets(message,
          sizeof(message),
          stdin);

    message[strcspn(message, "\n")] = '\0';

    /* ---------- CREATE FRAMES ---------- */

    int totalFrames = 0;
    int pos = 0;

    while (pos < strlen(message) &&
           totalFrames < MAX_FRAMES)
    {
        Frame f;

        f.seq = totalFrames;
        f.length = 0;

        while (message[pos] != '\0' &&
               f.length < FRAME_SIZE)
        {
            f.data[f.length++] =
                message[pos++];

        }

        f.data[f.length] = '\0';

        frames[totalFrames] = f;

        enqueue(&queue, f);

        totalFrames++;
    }

    printf("\nTotal Frames = %d\n",
           totalFrames);

    printf("Window Size = %d\n",
           WINDOW_SIZE);

    printf("Frame Size = %d characters\n",
           FRAME_SIZE);

    /* ---------- LOSS SIMULATION ---------- */

    int lostFrame;

    printf("\nEnter frame number to simulate loss");
    printf(" (-1 for no loss): ");

    scanf("%d", &lostFrame);

    printf("\n");

    if (lostFrame >= totalFrames)
    {
        printf("Invalid frame number.\n");
        lostFrame = -1;
    }

    /* ---------- GBN VARIABLES ---------- */

    /*
     * base = first unacknowledged frame
     * next = next frame to send
     */

    int base = 0;
    int next = 0;

    int lossDone = 0;

    /* ---------- GO-BACK-N ---------- */

    while (base < totalFrames)
    {
        /*
         * Send frames inside window.
         *
         * Maximum WINDOW_SIZE frames
         * can be outstanding.
         */

        while (next < totalFrames &&
               next < base + WINDOW_SIZE)
        {
            /*
             * Simulate loss only once.
             */

            if (frames[next].seq == lostFrame &&
                lossDone == 0)
            {
                printf("\n*** SIMULATING LOSS OF FRAME %d ***\n",
                       frames[next].seq);

                printf("Frame %d is NOT sent.\n",
                       frames[next].seq);

                lossDone = 1;

                next++;

                continue;
            }

            printf("\nSending Frame %d : %s\n",
                   frames[next].seq,
                   frames[next].data);

            if (!sendAll(clientSocket,
                         (char *)&frames[next],
                         sizeof(Frame)))
            {
                printf("Transmission failed.\n");
                close(clientSocket);
                return 1;
            }

            printf("Frame %d sent.\n",
                   frames[next].seq);

            next++;
        }

        /* ---------- WAIT FOR ACK ---------- */

        fd_set readfds;

        FD_ZERO(&readfds);

        FD_SET(clientSocket,
               &readfds);

        struct timeval timeout;

        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        /*
         * Linux select:
         *
         * First argument =
         * socket + 1
         */

        int result = select(
            clientSocket + 1,
            &readfds,
            NULL,
            NULL,
            &timeout);

        /* ---------- TIMEOUT ---------- */

        if (result == 0)
        {
            printf("\n============================================\n");
            printf("TIMEOUT!\n");
            printf("No ACK received within %d seconds.\n",
                   TIMEOUT);

            printf("Go-Back-N: Going back to Frame %d\n",
                   base);

            printf("Retransmitting outstanding frames...\n");
            printf("============================================\n");

            /*
             * Retransmit all outstanding frames.
             *
             * base ... next-1
             */

            for (int i = base; i < next; i++)
            {
                printf("Retransmitting Frame %d : %s\n",
                       frames[i].seq,
                       frames[i].data);

                if (!sendAll(clientSocket,
                             (char *)&frames[i],
                             sizeof(Frame)))
                {
                    printf("Retransmission failed.\n");
                    close(clientSocket);
                    return 1;
                }
            }

            /*
             * After retransmission,
             * wait for ACK again.
             */

            continue;
        }

        /* ---------- SELECT ERROR ---------- */

        if (result < 0)
        {
            perror("Select error");
            close(clientSocket);
            return 1;
        }

        /* ---------- RECEIVE ACK ---------- */

        int ack;

        if (!recvAll(clientSocket,
                     (char *)&ack,
                     sizeof(int)))
        {
            printf("Receiver disconnected.\n");
            close(clientSocket);
            return 1;
        }

        printf("\nACK %d received.\n",
               ack);

        /*
         * ACK means NEXT expected frame.
         *
         * ACK 3 means:
         *
         * Frame 0 received
         * Frame 1 received
         * Frame 2 received
         *
         * Receiver expects Frame 3.
         */

        if (ack > base &&
            ack <= next)
        {
            printf("Cumulative ACK received.\n");

            printf("Window slides from %d to %d.\n",
                   base,
                   ack);

            base = ack;
        }
        else
        {
            printf("Duplicate/old ACK ignored.\n");
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
