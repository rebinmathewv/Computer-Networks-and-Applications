#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TOKENS 100
#define TOKEN_LEN  10

/* Global variables for loop indexing */
int i, j;

/* Tokenizes an input string based on whitespace */
int tokenize(char line[], char tokens[][TOKEN_LEN])
{
    int count = 0;
    char *tok = strtok(line, " \t\n");
    while (tok != NULL && count < MAX_TOKENS)
    {
        strcpy(tokens[count], tok);
        count++;
        tok = strtok(NULL, " \t\n");
    }
    return count;
}

/* Performs character stuffing by adding a DLE before any DLE, STX, or ETX in the payload */
int stuffing(char in[][TOKEN_LEN], int inCount, char out[][TOKEN_LEN])
{
    j = 0;
    for (i = 0; i < inCount; i++)
    {
        if (strcmp(in[i], "DLE") == 0 ||
            strcmp(in[i], "STX") == 0 ||
            strcmp(in[i], "ETX") == 0)
        {
            strcpy(out[j], "DLE");
            j++;
        }
        strcpy(out[j], in[i]);
        j++;
    }
    return j;
}

/* Reverses character stuffing by removing the extra DLE prefix */
int destuffing(char in[][TOKEN_LEN], int inCount, char out[][TOKEN_LEN])
{
    j = 0;
    for (i = 0; i < inCount; i++)
    {
        if (strcmp(in[i], "DLE") == 0 && i + 1 < inCount &&
            (strcmp(in[i + 1], "DLE") == 0 ||
             strcmp(in[i + 1], "STX") == 0 ||
             strcmp(in[i + 1], "ETX") == 0))
        {
            strcpy(out[j], i + 1 < inCount ? in[i + 1] : "");
            j++;
            i++;
        }
        else
        {
            strcpy(out[j], in[i]);
            j++;
        }
    }
    return j;
}

/* Rebuilds a space-separated string from an array of tokens */
void joinTokens(char tokens[][TOKEN_LEN], int count, char dest[])
{
    dest[0] = '\0';
    for (i = 0; i < count; i++)
    {
        strcat(dest, tokens[i]);
        if (i != count - 1) strcat(dest, " ");
    }
}

/* Converts a text string into its raw binary string representation */
void stringToBinary(char str[], char binaryStr[])
{
    int idx = 0;
    for (i = 0; str[i] != '\0'; i++)
    {
        for (j = 7; j >= 0; j--)
            binaryStr[idx++] = ((str[i] >> j) & 1) ? '1' : '0';
    }
    binaryStr[idx] = '\0';
}

/* Converts a binary string back into text characters */
void binaryToString(char binaryStr[], char str[])
{
    int idx = 0;
    for (i = 0; binaryStr[i] != '\0'; i += 8)
    {
        char c = 0;
        for (j = 0; j < 8; j++)
            c = (c << 1) | (binaryStr[i + j] - '0');
        str[idx++] = c;
    }
    str[idx] = '\0';
}

int main()
{
    char header[50];
    char line[500];

    char bodyTokens[MAX_TOKENS][TOKEN_LEN];
    char stuffedTokens[MAX_TOKENS][TOKEN_LEN];
    char destuffedTokens[MAX_TOKENS][TOKEN_LEN];
    char recvTokens[MAX_TOKENS][TOKEN_LEN];

    char bodyStr[500], stuffedStr[500], destuffedStr[500];
    char binaryBody[4000], receivedBinaryBody[4000], receivedStuffedStr[500];

    char fullFrameStr[1000];
    char binaryFrame[8000];

    printf("=====================================================\n");
    printf("                BISYNC CHARACTER STUFFING          \n");
    printf("=====================================================\n\n");

    printf("Enter Header  : ");
    scanf("%s", header);
    getchar(); /* consume leftover newline */

    printf("Enter Payload : ");
    fgets(line, sizeof(line), stdin);

    int bodyCount = tokenize(line, bodyTokens);
    joinTokens(bodyTokens, bodyCount, bodyStr);

    int stuffedCount = stuffing(bodyTokens, bodyCount, stuffedTokens);
    joinTokens(stuffedTokens, stuffedCount, stuffedStr);

    printf("\n------------------- SENDER SIDE -------------------\n");

    printf("Original Frame Text  :\n%s\n\n", bodyStr);
    printf("Stuffed Payload Text :\n%s\n\n", stuffedStr);

    stringToBinary(stuffedStr, binaryBody);
    printf("Binary Stuffed Body  :\n%s\n", binaryBody);

    FILE *fp = fopen("message.txt", "w");
    if (fp != NULL)
    {
        fprintf(fp, "%s", binaryBody);
        fclose(fp);
        printf("\n[Status: Stuffed payload stream saved to message.txt]\n");
    }

    printf("\n------------------ RECEIVER SIDE ------------------\n");

    fp = fopen("message.txt", "r");
    if (fp != NULL)
    {
        fscanf(fp, "%s", receivedBinaryBody);
        fclose(fp);
        printf("[Status: Encrypted stream extracted from message.txt]\n");
    }
    else
    {
        strcpy(receivedBinaryBody, binaryBody);
    }

    binaryToString(receivedBinaryBody, receivedStuffedStr);

    printf("\nReceived Frame Text  :\n");
    sprintf(fullFrameStr, "SYN SYN SOH %s STX %s ETX CRC", header, receivedStuffedStr);
    printf("%s\n\n", fullFrameStr);

    stringToBinary(fullFrameStr, binaryFrame);
    printf("Binary Received Frame:\n%s\n", binaryFrame);

    int recvCount = tokenize(receivedStuffedStr, recvTokens);

    int destuffedCount = destuffing(recvTokens, recvCount, destuffedTokens);
    joinTokens(destuffedTokens, destuffedCount, destuffedStr);

    printf("\nDestuffed Frame Text :\n");
    sprintf(fullFrameStr, "SYN SYN SOH %s STX %s ETX CRC", header, destuffedStr);
    printf("%s\n\n", fullFrameStr);

    stringToBinary(fullFrameStr, binaryFrame);
    printf("Binary Destuffed Frame:\n%s\n", binaryFrame);
    printf("=====================================================\n");

    return 0;
}
