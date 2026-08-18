#include <stdio.h>

#define ROWS 6
#define COLS 7

int matrix[ROWS + 1][COLS + 1];

void readInput()
{
    int i, j;

    printf("\n========================================\n");
    printf(" SENDER : Binary Matrix + 2D Parity\n");
    printf("========================================\n");

    printf("\nEnter %d x %d Binary Data:\n\n", ROWS, COLS);

    for (i = 0; i < ROWS; i++)
    {
        printf("Row %d: ", i + 1);

        for (j = 0; j < COLS; j++)
        {
            scanf("%d", &matrix[i][j]);
        }
    }
}

void calculateParity()
{
    int i, j;
    int count;

    /* Row parity */
    for (i = 0; i < ROWS; i++)
    {
        count = 0;

        for (j = 0; j < COLS; j++)
        {
            if (matrix[i][j] == 1)
                count++;
        }

        matrix[i][COLS] = count % 2;
    }

    /* Column parity */
    for (j = 0; j < COLS; j++)
    {
        count = 0;

        for (i = 0; i < ROWS; i++)
        {
            if (matrix[i][j] == 1)
                count++;
        }

        matrix[ROWS][j] = count % 2;
    }

    /* Corner parity */
    count = 0;

    for (j = 0; j < COLS; j++)
    {
        if (matrix[ROWS][j] == 1)
            count++;
    }

    matrix[ROWS][COLS] = count % 2;
}

void displayMatrix()
{
    int i, j;

    printf("\n========================================\n");
    printf(" SENDER : 2D PARITY MATRIX\n");
    printf("========================================\n");

    printf("Matrix size: 6 rows x 7 columns (+ parity row/col)\n\n");

    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            printf("%d ", matrix[i][j]);
        }

        printf("| %d   <- row parity\n", matrix[i][COLS]);
    }

    printf("------------------\n");

    for (j = 0; j < COLS; j++)
    {
        printf("%d ", matrix[ROWS][j]);
    }

    printf("| %d   <- corner (parity of parities)\n",
           matrix[ROWS][COLS]);

    printf("^\n");
    printf("|\n");
    printf("column parities\n");
}

void writeFile()
{
    FILE *fp;
    int i, j;

    fp = fopen("transmission.dat", "w");

    if (fp == NULL)
    {
        printf("\n[ERROR] Cannot create transmission.dat\n");
        return;
    }

    for (i = 0; i <= ROWS; i++)
    {
        for (j = 0; j <= COLS; j++)
        {
            fprintf(fp, "%d", matrix[i][j]);

            if (j < COLS)
                fprintf(fp, " ");
        }

        fprintf(fp, "\n");
    }

    fclose(fp);

    printf("\nSender: matrix with 2D parity written to transmission.dat\n");
    printf("Run receiver next to simulate bit errors and check parity.\n");
}

int main()
{
    readInput();
    calculateParity();
    displayMatrix();
    writeFile();

    return 0;
}
