#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_LEN 1000
#define N_PHASE 100
#define N_OFFSET 7
#define N_APPEND 10000

/**
 * n x n upper triangular matrix
 */
int main()
{
    int i, j, k, n, cnt, sum, phase, sigsize;
    // actual output list starts at 0, 1, 0, -1
    // due to the matrix being upper triangular and shifted we can shift at the start
    int output[4] = {1, 0, -1, 0};
    char offlist[N_OFFSET + 1];
    char *line, *origsig;
    line = malloc(sizeof(*line) * MAX_LEN);
    if (!line) {return -1;}

    if (!fgets(line, sizeof(*line) * MAX_LEN, stdin)) {return -2;}
    n = strlen(line);
    origsig = malloc(sizeof(*origsig) * (n + 1));
    if (!origsig) {return -1;}
    line[n-- - 1] = '\0';
    strcpy(origsig, line); // save the original input for part 2
    strncpy(offlist, line, N_OFFSET);
    offlist[N_OFFSET] = '\0';

    // upper triangular matrix so we can ignore at least half
    phase = 0;
    while (phase++ < N_PHASE)
    {
        for (i = 0; i < n; i++)
        {
            sum = 0;
            cnt = 0;
            k = 0;
            for (j = i; j < n; j++)
            {
                sum += (line[j] - '0') * output[k];
                cnt++;
                if (cnt >= i + 1)
                {
                    k++;
                    cnt = 0;
                    if (k == 4) {k = 0;}
                }
            }
            line[i] = (abs(sum) % 10) + '0';
        }
    }
    printf("part 1 - first 8 digits are %.8s\n", line);
    free(line);

    // part 2 - input list is the initial input appended to itself 10000 times
    // we can instead start calculating from the 7-digit message offset, which is the first 7 digits of the input
    int offset = (n * N_APPEND) - atoi(offlist);
    sigsize = offset - (offset % n) + n; // rounded up to nearest whole n (size of input string)
    char *realsig = malloc(sizeof(*realsig) * (sigsize + 1)); // +1 for null byte
    if (!realsig) {printf("Not enough memory for part 2\n"); return -3;}
    for (i = 0; i < sigsize; i += n) {strcat(realsig, origsig);}
    free(origsig);

    // due to the fact the output list expands by the depth of the y index you're at in the matrix
    // thus for FFT calculations at index > matrix length / 2 the sum is merely the sum off all the indices
    // and since it's this simplified, we can just go over the diagonal of the matrix backwards
    phase = 0;
    while (phase++ < N_PHASE)
    {
        sum = 0;
        for (i = sigsize - 1; i > sigsize - offset - 1; i--)
        {
            sum += realsig[i] - '0';
            realsig[i] = (abs(sum) % 10) + '0';
        }
    }
    printf("part 2 - first 8 digits after offset (%d) are %.8s\n", offset, &realsig[sigsize - offset]);

    free(realsig);

    return 0;
}
