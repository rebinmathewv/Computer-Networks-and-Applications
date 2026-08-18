#include <stdio.h>

#define DATA_BITS 7
#define HAMMING_BITS 11
#define TOTAL_BITS 12

int readFile(int codewords[100][12])
{
    FILE *fp;
    int count = 0, i, j;

    fp = fopen("transmitted.txt", "r");

    if (fp == NULL)
        return 0;

    if (fscanf(fp, "%d", &count) != 1)
    {
        fclose(fp);
        return 0;
    }

    for (i = 0; i < count; i++)
    {
        for (j = 0; j < 12; j++)
            fscanf(fp, "%d", &codewords[i][j]);
    }

    fclose(fp);

    return count;
}

void writeFile(int codewords[100][12], int count)
{
    FILE *fp;
    int i, j;

    fp = fopen("transmitted.txt", "w");

    if (fp == NULL)
    {
        printf("\n[ERROR] Failed to update 'transmitted.txt'\n");
        return;
    }

    fprintf(fp, "%d\n", count);

    for (i = 0; i < count; i++)
    {
        for (j = 0; j < 12; j++)
        {
            fprintf(fp, "%d", codewords[i][j]);

            if (j < 11)
                fprintf(fp, " ");
        }

        fprintf(fp, "\n");
    }

    fclose(fp);
}

void displayTransmission(int codewords[100][12], int count)
{
    int i, j;

    printf("\n-------------------------------------------------------------\n");
    printf("| RECEIVER TRANSMISSION MATRIX |\n");
    printf("-------------------------------------------------------------\n");

    printf("| Layout: P0 P1 P2 D1 P4 D2 D3 D4 P8 D5 D6 D7 |\n");
    printf("| Indices: 0 1 2 3 4 5 6 7 8 9 10 11 |\n");

    printf("-------------------------------------------------------------\n");

    for (i = 0; i < count; i++)
    {
        printf("| Char #%-2d | ", i + 1);

        for (j = 0; j < 12; j++)
            printf("%d ", codewords[i][j]);

        printf("|\n");
    }

    printf("-------------------------------------------------------------\n");
}

/* Introduce error */

void flipBit(int codewords[100][12], int charIdx, int bitIdx)
{
    int oldVal = codewords[charIdx][bitIdx];

    if (codewords[charIdx][bitIdx] == 0)
        codewords[charIdx][bitIdx] = 1;
    else
        codewords[charIdx][bitIdx] = 0;

    printf("\n--- [CHANNEL NOISE INJECTED] ---\n");

    printf("Character %d\n", charIdx + 1);

    printf("Bit Index %d changed (%d -> %d)\n",
           bitIdx,
           oldVal,
           codewords[charIdx][bitIdx]);

    if (bitIdx == 0)
        printf("This is the overall parity bit P0.\n");
    else
        printf("This corresponds to Hamming position %d.\n",
               bitIdx);
}

/* Calculate syndrome */

int calcSyndrome(int codeword[12])
{
    int s1, s2, s4, s8;

    s1 =
        codeword[1] ^
        codeword[3] ^
        codeword[5] ^
        codeword[7] ^
        codeword[9] ^
        codeword[11];

    s2 =
        codeword[2] ^
        codeword[3] ^
        codeword[6] ^
        codeword[7] ^
        codeword[10] ^
        codeword[11];

    s4 =
        codeword[4] ^
        codeword[5] ^
        codeword[6] ^
        codeword[7];

    s8 =
        codeword[8] ^
        codeword[9] ^
        codeword[10] ^
        codeword[11];

    return (s8 * 8) +
           (s4 * 4) +
           (s2 * 2) +
           s1;
}

/* Overall parity */

int checkOverallParity(int codeword[12])
{
    int count = 0, i;

    for (i = 0; i < 12; i++)
    {
        if (codeword[i] == 1)
            count++;
    }

    return count % 2;
}

/* ---------------------------------------------------------
   Detailed error position calculation
   This is shown ONLY when an error is injected.
   --------------------------------------------------------- */

void showErrorCalculation(int codeword[12])
{
    int s1, s2, s4, s8;
    int errorPosition;

    s1 =
        codeword[1] ^
        codeword[3] ^
        codeword[5] ^
        codeword[7] ^
        codeword[9] ^
        codeword[11];

    s2 =
        codeword[2] ^
        codeword[3] ^
        codeword[6] ^
        codeword[7] ^
        codeword[10] ^
        codeword[11];

    s4 =
        codeword[4] ^
        codeword[5] ^
        codeword[6] ^
        codeword[7];

    s8 =
        codeword[8] ^
        codeword[9] ^
        codeword[10] ^
        codeword[11];

    errorPosition =
        (s8 * 8) +
        (s4 * 4) +
        (s2 * 2) +
        s1;

    printf("\n-------------------------------------------------------------\n");
    printf(" ERROR POSITION CALCULATION\n");
    printf("-------------------------------------------------------------\n");

    printf("\nP1 check:\n");
    printf("1 3 5 7 9 11\n");

    printf("%d %d %d %d %d %d\n",
           codeword[1],
           codeword[3],
           codeword[5],
           codeword[7],
           codeword[9],
           codeword[11]);

    printf("S1 = %d\n", s1);

    printf("\nP2 check:\n");
    printf("2 3 6 7 10 11\n");

    printf("%d %d %d %d %d %d\n",
           codeword[2],
           codeword[3],
           codeword[6],
           codeword[7],
           codeword[10],
           codeword[11]);

    printf("S2 = %d\n", s2);

    printf("\nP4 check:\n");
    printf("4 5 6 7\n");

    printf("%d %d %d %d\n",
           codeword[4],
           codeword[5],
           codeword[6],
           codeword[7]);

    printf("S4 = %d\n", s4);

    printf("\nP8 check:\n");
    printf("8 9 10 11\n");

    printf("%d %d %d %d\n",
           codeword[8],
           codeword[9],
           codeword[10],
           codeword[11]);

    printf("S8 = %d\n", s8);

    printf("\nSyndrome bits:\n");
    printf("S8 S4 S2 S1\n");

    printf("%d  %d  %d  %d\n",
           s8, s4, s2, s1);

    printf("\nError Position Calculation:\n");

    printf("= (S8 x 8) + (S4 x 4) + (S2 x 2) + (S1 x 1)\n");

    printf("= (%d x 8) + (%d x 4) + (%d x 2) + (%d x 1)\n",
           s8, s4, s2, s1);

    printf("= %d + %d + %d + %d\n",
           s8 * 8,
           s4 * 4,
           s2 * 2,
           s1);

    printf("= %d\n", errorPosition);

    printf("\n>>> ERROR POSITION = %d <<<\n",
           errorPosition);

    printf("-------------------------------------------------------------\n");
}

/* ---------------------------------------------------------
   Check and correct
   showDetails = 1 only for the character where error
   was injected.
   --------------------------------------------------------- */

void checkAndFix(int codeword[12], int charNum, int showDetails)
{
    int errorPosition;
    int overallParityError;
    int oldValue;

    errorPosition = calcSyndrome(codeword);

    overallParityError = checkOverallParity(codeword);

    if (showDetails)
    {
        showErrorCalculation(codeword);
    }

    printf("\n-------------------------------------------------------------\n");
    printf("| Character %d Diagnostic |\n", charNum);
    printf("-------------------------------------------------------------\n");

    printf("Syndrome = %d\n", errorPosition);
    printf("Overall parity = %d\n", overallParityError);

    if (errorPosition == 0 &&
        overallParityError == 0)
    {
        printf("\nDiagnostic Status : ERROR-FREE\n");
        printf("Action Taken      : None (Data intact)\n");
    }

    else if (errorPosition != 0 &&
             overallParityError == 1)
    {
        printf("\nDiagnostic Status : SINGLE-BIT ERROR\n");

        printf("Error Position    : Bit %d\n",
               errorPosition);

        oldValue = codeword[errorPosition];

        printf("\nBefore correction : %d\n",
               oldValue);

        codeword[errorPosition] ^= 1;

        printf("After correction  : %d\n",
               codeword[errorPosition]);

        printf("\n>>> Bit %d auto-corrected successfully. <<<\n",
               errorPosition);
    }

    else if (errorPosition != 0 &&
             overallParityError == 0)
    {
        printf("\nDiagnostic Status : DOUBLE-BIT ERROR\n");

        printf("Syndrome          : %d\n",
               errorPosition);

        printf("\n>>> TWO-BIT ERROR DETECTED. <<<\n");
        printf(">>> Cannot safely correct this error. <<<\n");
    }

    else if (errorPosition == 0 &&
             overallParityError == 1)
    {
        printf("\nDiagnostic Status : PARITY BIT (P0) ERROR\n");

        printf("Error Position    : P0 (index 0)\n");

        oldValue = codeword[0];

        printf("P0 before correction : %d\n",
               oldValue);

        codeword[0] ^= 1;

        printf("P0 after correction  : %d\n",
               codeword[0]);

        printf("\n>>> P0 bit auto-corrected successfully. <<<\n");
    }
}

char bitsToChar(int codeword[12])
{
    int dataBits[7];
    int asciiVal = 0;
    int i;

    dataBits[0] = codeword[3];
    dataBits[1] = codeword[5];
    dataBits[2] = codeword[6];
    dataBits[3] = codeword[7];
    dataBits[4] = codeword[9];
    dataBits[5] = codeword[10];
    dataBits[6] = codeword[11];

    for (i = 0; i < 7; i++)
        asciiVal = (asciiVal * 2) + dataBits[i];

    return (char)asciiVal;
}

/* Normal verification - NO detailed calculation */

void initialVerification(int codewords[100][12], int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        checkAndFix(codewords[i], i + 1, 0);
    }

    printf("\n-------------------------------------------------------------\n");

    printf("| DECODED OUTPUT: ");

    for (i = 0; i < count; i++)
        printf("%c", bitsToChar(codewords[i]));

    printf(" |\n");

    printf("-------------------------------------------------------------\n");
}

/* After error injection */

void verificationAfterError(int codewords[100][12],
                            int count,
                            int errorChar)
{
    int i;

    for (i = 0; i < count; i++)
    {
        if (i == errorChar)
        {
            /* Detailed calculation ONLY HERE */
            checkAndFix(codewords[i],
                        i + 1,
                        1);
        }
        else
        {
            /* Other characters remain short */
            checkAndFix(codewords[i],
                        i + 1,
                        0);
        }
    }

    printf("\n-------------------------------------------------------------\n");

    printf("| DECODED OUTPUT: ");

    for (i = 0; i < count; i++)
        printf("%c", bitsToChar(codewords[i]));

    printf(" |\n");

    printf("-------------------------------------------------------------\n");
}

int main()
{
    int codewords[100][12];

    int count;
    int choice;
    int charNum;
    int bitIdx;

    count = readFile(codewords);

    if (count == 0)
    {
        printf("\n[FATAL ERROR] 'transmitted.txt' is missing or unreadable.\n");
        return 1;
    }

    displayTransmission(codewords, count);

    printf("\n--- Performing Initial Verification Diagnostic...\n");

    initialVerification(codewords, count);

    while (1)
    {
        printf("\n===================================================\n");
        printf(" RECEIVER TRANSMISSION SIMULATOR \n");
        printf("===================================================\n");

        printf("1. Add Error\n");
        printf("2. Exit\n");

        printf("---------------------------------------------------\n");
        printf("Select an action (1-2): ");

        if (scanf("%d", &choice) != 1)
            break;

        if (choice == 1)
        {
            printf("Target Character Number (1 to %d): ",
                   count);

            scanf("%d", &charNum);

            printf("Target Bit Index (0 to 11): ");
            printf("\n0 = P0, 1-11 = Hamming position\n");

            scanf("%d", &bitIdx);

            if (charNum < 1 ||
                charNum > count ||
                bitIdx < 0 ||
                bitIdx > 11)
            {
                printf("\n[INVALID] Selection out of boundaries! Please retry.\n");
                continue;
            }

            /* Remember which character had the error */
            int errorChar = charNum - 1;

            flipBit(codewords,
                    errorChar,
                    bitIdx);

            writeFile(codewords, count);

            printf("\n>>> Re-evaluating Updated Transmission Stream:\n");

            displayTransmission(codewords, count);

            printf("\n>>> Error Analysis:\n");

            /*
             * Detailed calculation appears ONLY ONCE
             * for the character where the error was injected.
             */
            verificationAfterError(codewords,
                                    count,
                                    errorChar);
        }

        else if (choice == 2)
        {
            printf("\nSimulation successfully terminated.\n");
            break;
        }

        else
        {
            printf("\n[INVALID] Unknown choice sequence selected.\n");
        }
    }

    return 0;
}
