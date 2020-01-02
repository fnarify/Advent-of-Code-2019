#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIM    5
#define MID    (DIM / 2)
#define N_STEP 200
#define EMPTY  '.'
#define BUG    '#'
#define RECUR  '?'

int adj[] = {
    [EMPTY] = 0,
    [BUG] = 0,
    [RECUR] = 0
};

/**
 * if the point is in the middle '?' just return that early
 */
char checkadj(char (*state)[DIM][DIM], int x, int y, int z)
{
    int i;
    char ret = state[z][y][x];
    adj[EMPTY] = adj[BUG] = adj[RECUR] = 0;

    if (x == MID && y == MID) {return ret;}

    // left side
    if (!x)           {adj[(int) state[z - 1][MID][MID - 1]]++;}
    else              {adj[(int) state[z][y][x - 1]]++;}
    // right side
    if (x == DIM - 1) {adj[(int) state[z - 1][MID][MID + 1]]++;}
    else              {adj[(int) state[z][y][x + 1]]++;}
    // upper side
    if (!y)           {adj[(int) state[z - 1][MID - 1][MID]]++;}
    else              {adj[(int) state[z][y - 1][x]]++;}
    // lower side
    if (y == DIM - 1) {adj[(int) state[z - 1][MID + 1][MID]]++;}
    else              {adj[(int) state[z][y + 1][x]]++;}

    // we check interior adjacents now
    // you can make this a single for loop, but it doesn't make the logic any terser or simpler
    if (adj[RECUR])
    {
        if      (x == MID - 1 && y == MID) // left
        {
            for (i = 0; i < DIM; i++) {adj[(int) state[z + 1][i][0]]++;}
        }
        else if (x == MID + 1 && y == MID) // right
        {
            for (i = 0; i < DIM; i++) {adj[(int) state[z + 1][i][DIM - 1]]++;}
        }
        else if (x == MID && y == MID - 1) // up
        {
            for (i = 0; i < DIM; i++) {adj[(int) state[z + 1][0][i]]++;}
        }
        else if (x == MID && y == MID + 1) // down
        {
            for (i = 0; i < DIM; i++) {adj[(int) state[z + 1][DIM - 1][i]]++;}
        }
    }

    if      (ret == EMPTY && (adj[BUG] == 1 || adj[BUG] == 2)) {ret = BUG;}
    else if (ret == BUG && adj[BUG] != 1)                      {ret = EMPTY;}

    return ret;
}

/**
 * yes you could probably safe a lot of space and some speed by making each level an int
 * as there are only 25 places per level, thus it can easily fit in a bitmask in a 32-bit integer
 */
int main()
{
    char (*grid)[DIM][DIM], (*next)[DIM][DIM], line[DIM + 2];
    int i, j, n, level, nlevel, lmin, lmax, alt;

    nlevel = N_STEP * 3;
    grid = malloc(sizeof(*grid) * nlevel);
    next = malloc(sizeof(*next) * nlevel);
    if (!grid || !next) {return -1;}

    for (i = 0; i < nlevel; i++)
    {
        memset(grid[i], EMPTY, sizeof(grid[i]));
        grid[i][MID][MID] = RECUR;
    }

    i = 0;
    lmin = lmax = nlevel / 2;
    while (fgets(line, sizeof(line), stdin)) {strncpy(&grid[lmin][i++][0], line, strlen(line) - 1);}
    grid[lmin][MID][MID] = RECUR;

    n = alt = 0;
    while (n++ < N_STEP)
    {
        for (level = lmin; level < lmax + 1; level++)
        {
            for (i = 0; i < DIM; i++)
            {
                for (j = 0; j < DIM; j++)
                {
                    next[level][i][j] = checkadj(grid, j, i, level);
                }
            }
        }
        for (level = lmin; level < lmax + 1; level++) {memcpy(grid[level], next[level], DIM * DIM);}

        // layers increase in size in alternating directions each turn, at a glance?
        alt ? lmin-- : lmax++;
        alt ^= 1;
    }

    n = 0;
    while (lmin < lmax)
    {
        for (i = 0; i < DIM; i++)
        {
            for (j = 0; j < DIM; j++)
            {
                if (grid[lmin][i][j] == BUG) {n++;}
            }
        }
        lmin++;
    }
    printf("%d bugs left after %d mins\n", n, N_STEP); // answer should be 1912, test input passes

    free(grid);
    free(next);

    return 0;
}
