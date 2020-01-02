#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define N_MIN 200
#define N_REC 250
#define DIM 5
#define OFF 1
#define EMPTY '.'
#define BUG   '#'

int adj[] = {
    [EMPTY] = 0,
    [BUG] = 0
};

/**
 * only need to check horizontal and vertical adjacents
 * a bug becomes empty unless it is adjacent to exactly one bug
 * empty becomes a bug if exactly 1 to 2 bugs are adjacent to it
 */
char checkadj(char state[DIM + 2][DIM + 3], int x, int y)
{
    char ret = state[y][x];

    adj[EMPTY] = adj[BUG] = 0;
    adj[(int) state[y][x + 1]]++;
    adj[(int) state[y][x - 1]]++;
    adj[(int) state[y + 1][x]]++;
    adj[(int) state[y - 1][x]]++;

    if ((adj[BUG] == 1 || adj[BUG] == 2) && ret == EMPTY) {ret = BUG;}
    else if (adj[BUG] != 1 && ret == BUG) {ret = EMPTY;}

    return ret;
}

int main()
{
    char grid[DIM + 2][DIM + 3], next[DIM][DIM], line[DIM + 2];
    int i, j, hsize, fnd, temp;
    int *hist = calloc(N_REC, sizeof(*hist));

    // so the bounds outside the grid are more easily treated as empty spaces
    memset(grid, EMPTY, sizeof(grid));

    i = 0;
    while (fgets(line, sizeof(line), stdin))
    {
        strncpy(&grid[i + OFF][OFF], line, strlen(line) - 1);
        grid[i + OFF][DIM + OFF] = '.';
        i++;
    }

    for (i = 0; i < DIM + 2; i++) {grid[i][DIM + 2] = '\0';}

    // part 1
    hsize = fnd = 0;
    while (!fnd)
    {
        for (i = 0; i < DIM; i++)
        {
            for (j = 0; j < DIM; j++)
            {
                next[i][j] = checkadj(grid, j + OFF, i + OFF);
                if (next[i][j] == BUG) {hist[hsize] += pow(2, i * DIM + j);}
            }
        }

        temp = hist[hsize++];
        for (i = 0; i < hsize - 1; i++)
        {
            if (temp == hist[i])
            {
                printf("part 1 - %d repeated biodiversity rating\n", hist[i]);
                fnd = 1;
            }
        }

        for (i = 0; i < DIM; i++) {memcpy(&grid[i + OFF][OFF], &next[i][0], DIM);}
    }

    free(hist);

    return 0;
}
