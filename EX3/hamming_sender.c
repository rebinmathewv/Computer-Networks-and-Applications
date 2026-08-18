#include <stdio.h>

#define DATA_BITS 7
#define PARITY_BITS 4
#define TOTAL_BITS 12

int readInput(char message[], int binaryMatrix[100][7])
{
    FILE *fp;
    char ch;
    int count = 0, i, val;

    fp = fopen("input.txt", "r");

    if (fp == NULL)
        return 0;

    while ((ch = fgetc(fp)) != EOF)
    {
        if (ch == '\n' || ch == '\r')
            continue;

        message[count] = ch;
        val = (int)ch;

        for (i = 6; i >= 0; i--)
        {
            binaryMatrix[count][i] = val % 2;
            val = val / 2;
        }

        count++;

        if (count >= 100)
            break;
    }

    fclose(fp);
    return count;
}

void displayBinary(char message[], int binaryMatrix[100][7], int count)
{
    int i, j;

    printf("\n-------------------------------------------------------------\n");
    printf("| LOADED INPUT SUMMARY |\n");
    printf("-------------------------------------------------------------\n");

    printf("| Loaded Message : \"");

    for (i = 0; i < count; i++)
        printf("%c", message[i]);

    printf("\"\n");

    printf("| Total Characters: %-40d|\n", count);

    printf("-------------------------------------------------------------\n");
    printf("| Char | Binary Representative (7-bit ASCII) |\n");
    printf("-------------------------------------------------------------\n");

    for (i = 0; i < count; i++)
    {
        printf("| '%c' | ", message[i]);

        for (j = 0; j < 7; j++)
            printf("%d ", binaryMatrix[i][j]);

        printf("|\n");
    }

    printf("-------------------------------------------------------------\n");
}

/* Calculate required parity bits */

int calculateParityBits(int d)
{
    int p = 0;

    printf("\n=============================================================\n");
    printf(" HAMMING PARITY BIT CALCULATION\n");
    printf("=============================================================\n");

    printf("Number of data bits (d) = %d\n", d);

    printf("\nCondition:\n");
    printf("d + p + 1 <= 2^p\n\n");

    while (1)
    {
        p++;

        printf("For p = %d:\n", p);

        printf("d + p + 1 = %d + %d + 1 = %d\n",
               d, p, d + p + 1);

        printf("2^p       = 2^%d = %d\n",
               p, 1 << p);

        if ((d + p + 1) <= (1 << p))
        {
            printf("%d <= %d  -> TRUE\n",
                   d + p + 1, 1 << p);

            printf("\nTherefore, number of parity bits (p) = %d\n",
                   p);

            break;
        }
        else
        {
            printf("%d <= %d  -> FALSE\n\n",
                   d + p + 1, 1 << p);
        }
    }

    printf("\nTotal Hamming bits = d + p\n");
    printf("                   = %d + %d\n", d, p);
    printf("                   = %d\n", d + p);

    printf("=============================================================\n");

    return p;
}

/* Display Hamming layout */

void displayBitPositions()
{
    printf("\n=============================================================\n");
    printf(" HAMMING CODE BIT POSITION LAYOUT\n");
    printf("=============================================================\n");

    printf("\nHamming positions (right to left):\n\n");

    printf("Position : 11  10   9   8   7   6   5   4   3   2   1\n");
    printf("         -------------------------------------------------\n");
    printf("Content  : D7  D6  D5  P8  D4  D3  D2  P4  D1  P2  P1\n");

    printf("\nProgram layout (index order):\n");
    printf("P0 P1 P2 D1 P4 D2 D3 D4 P8 D5 D6 D7\n");

    printf("=============================================================\n");
}

/* Generate Hamming code */

void genHamm(int data[7], int codeword[12])
{
    int i;

    for (i = 0; i < 12; i++)
        codeword[i] = 0;

    codeword[3] = data[0];
    codeword[5] = data[1];
    codeword[6] = data[2];
    codeword[7] = data[3];
    codeword[9] = data[4];
    codeword[10] = data[5];
    codeword[11] = data[6];

    codeword[1] =
        codeword[3] ^
        codeword[5] ^
        codeword[7] ^
        codeword[9] ^
        codeword[11];

    codeword[2] =
        codeword[3] ^
        codeword[6] ^
        codeword[7] ^
        codeword[10] ^
        codeword[11];

    codeword[4] =
        codeword[5] ^
        codeword[6] ^
        codeword[7];

    codeword[8] =
        codeword[9] ^
        codeword[10] ^
        codeword[11];
}

/* Overall parity */

int calcParity(int codeword[12])
{
    int count = 0, i;

    for (i = 1; i <= 11; i++)
    {
        if (codeword[i] == 1)
            count++;
    }

    return count % 2;
}

void writeFile(int binaryMatrix[100][7], int count)
{
    FILE *fp;
    int codeword[12];
    int i, j;

    fp = fopen("transmitted.txt", "w");

    if (fp == NULL)
    {
        printf("\n[ERROR] Failed to open 'transmitted.txt' for writing.\n");
        return;
    }

    printf("\n-------------------------------------------------------------\n");
    printf("| GENERATING SENDER CODEWORDS |\n");
    printf("-------------------------------------------------------------\n");

    printf("| Layout: P0 P1 P2 D1 P4 D2 D3 D4 P8 D5 D6 D7 |\n");
    printf("-------------------------------------------------------------\n");

    fprintf(fp, "%d\n", count);

    for (i = 0; i < count; i++)
    {
        genHamm(binaryMatrix[i], codeword);

        codeword[0] = calcParity(codeword);

        printf("| Char #%-2d | ", i + 1);

        for (j = 0; j < 12; j++)
            printf("%d ", codeword[j]);

        printf("|\n");

        for (j = 0; j < 12; j++)
        {
            fprintf(fp, "%d", codeword[j]);

            if (j < 11)
                fprintf(fp, " ");
        }

        fprintf(fp, "\n");
    }

    printf("-------------------------------------------------------------\n");

    fclose(fp);
}

int main()
{
    char message[100];
    int binaryMatrix[100][7];
    int count;

    printf("===================================================\n");
    printf(" SENDER TRANSMISSION SIMULATOR \n");
    printf("===================================================\n");

    count = readInput(message, binaryMatrix);

    if (count == 0)
    {
        printf("\n[FATAL ERROR] 'input.txt' is missing or completely empty.\n");
        return 1;
    }

    displayBinary(message, binaryMatrix, count);

    calculateParityBits(DATA_BITS);

    displayBitPositions();

    writeFile(binaryMatrix, count);

    printf("\n--- [SUCCESS] Stream mapped and encoded into 'transmitted.txt'.\n\n");

    return 0;
}
