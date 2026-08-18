#include <stdio.h>
#include <stdlib.h>

#define ROWS 6
#define COLS 7

int receiver[ROWS + 1][COLS + 1];

void readFromChannel();
void introduceError();
void displayReceiver();
void checkParity();

void readFromChannel()
{
    FILE *fp;
    int i, j;
    fp = fopen("transmission.dat", "r");
    if (fp == NULL)
    {
        printf("\n[ERROR] transmission.dat not found.\n");
        printf("Make sure you run sender first.\n");
        exit(1);
    }
    for (i = 0; i <= ROWS; i++)
    {
        for (j = 0; j <= COLS; j++)
        {
            if (fscanf(fp, "%d", &receiver[i][j]) != 1)
            {
                printf("\n[ERROR] Error reading transmission.dat\n");
                fclose(fp);
                exit(1);
            }
        }
    }
    fclose(fp);
    printf("\n========================================\n");
    printf(" RECEIVER : Matrix received from channel\n");
    printf("========================================\n");
    displayReceiver();
}

void introduceError()
{
    int choice;
    int errors;
    int row, col;
    int i, j;
    int errorRow[42], errorCol[42]; // Maintained as array sizes matching page 2/4 of PDF
    int duplicate;
    printf("\nDo you want to introduce error(s)?\n");
    printf("1. Yes\n");
    printf("2. No\n");
    printf("Enter Choice : ");
    scanf("%d", &choice);
    if (choice == 1)
    {
        printf("Enter number of errors: ");
        scanf("%d", &errors);
        printf("\nNote: Enter different positions for each error.\n");
        for (i = 0; i < errors; i++)
        {
            while (1)
            {
                printf("\nError %d\n", i + 1);
                printf("Enter Row (1-%d): ", ROWS);
                scanf("%d", &row);
                printf("Enter Column (1-%d): ", COLS);
                scanf("%d", &col);
                row--;
                col--;
                if (row < 0 || row >= ROWS ||
                    col < 0 || col >= COLS)
                {
                    printf("Invalid Position. Try again.\n");
                    continue;
                }
                duplicate = 0;
                for (j = 0; j < i; j++)
                {
                    if (errorRow[j] == row &&
                        errorCol[j] == col)
                    {
                        duplicate = 1;
                        break;
                    }
                }
                if (duplicate)
                {
                    printf("Position already selected. "
                           "Enter a different position.\n");
                    continue;
                }
                errorRow[i] = row;
                errorCol[i] = col;
                receiver[row][col] ^= 1;
                break;
            }
        }
    }
}

void displayReceiver()
{
    int i, j;
    printf("\nReceiver Matrix:\n\n");
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            printf("%d ", receiver[i][j]);
        }
        printf("| %d <- row parity\n",
               receiver[i][COLS]);
    }
    printf("------------------\n");
    for (j = 0; j < COLS; j++)
    {
        printf("%d ", receiver[ROWS][j]);
    }
    printf("| %d <- corner\n",
           receiver[ROWS][COLS]);
}

void checkParity()
{
    int i, j;
    int count;

    // Tracking structures to catch multiple parallel parity discrepancies
    int rowErrorsList[ROWS] = {0};
    int colErrorsList[COLS] = {0};

    int rowCount = 0;
    int colCount = 0;

    /* Check Row Parity */
    for (i = 0; i < ROWS; i++)
    {
        count = 0;
        for (j = 0; j < COLS; j++)
        {
            if (receiver[i][j] == 1)
                count++;
        }
        if ((count % 2) != receiver[i][COLS])
        {
            rowErrorsList[i] = 1;
            rowCount++;
        }
    }

    /* Check Column Parity */
    for (j = 0; j < COLS; j++)
    {
        count = 0;
        for (i = 0; i < ROWS; i++)
        {
            if (receiver[i][j] == 1)
                count++;
        }
        if ((count % 2) != receiver[ROWS][j])
        {
            colErrorsList[j] = 1;
            colCount++;
        }
    }

    /* Process Parity Results */
    if (rowCount == 0 && colCount == 0)
    {
        printf("\nData received without error.\n");
    }
    else if (rowCount == 1 && colCount == 1)
    {
        int rErr = -1, cErr = -1;
        for (i = 0; i < ROWS; i++) { if (rowErrorsList[i]) rErr = i; }
        for (j = 0; j < COLS; j++) { if (colErrorsList[j]) cErr = j; }

        printf("\nSingle Bit Error Detected.\n");
        printf("Error Position : Row %d Column %d\n", rErr + 1, cErr + 1);
        receiver[rErr][cErr] ^= 1;
        printf("\nError Corrected Successfully.\n");
        printf("\nCorrected Matrix:\n\n");
        displayReceiver();
    }
    else
    {
        printf("\nMultiple Bit Errors Detected.\n");
        printf("Error cannot be corrected using 2D Parity.\n");

        // Print out all offending rows
        printf("Parity errors found at Rows: ");
        if (rowCount > 0) {
            for (i = 0; i < ROWS; i++) {
                if (rowErrorsList[i]) {
                    printf("%d ", i + 1);
                }
            }
        } else {
            printf("None");
        }
        printf("\n");

        // Print out all offending columns
        printf("Parity errors found at Columns: ");
        if (colCount > 0) {
            for (j = 0; j < COLS; j++) {
                if (colErrorsList[j]) {
                    printf("%d ", j + 1);
                }
            }
        } else {
            printf("None");
        }
        printf("\n");
    }
}

int main()
{
    readFromChannel();
    introduceError();
    printf("\n");
    displayReceiver();
    checkParity();
    return 0;
}
