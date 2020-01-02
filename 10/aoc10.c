#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WT 26
#define HT 26
#define ASTER '#'
#define SPACE '.'
#define ASTREM 200

struct Station {
    char map[HT][WT + 2];
    int h, w, x, y, nlos;
    double los[HT * WT];
    int los_pos[HT * WT][2];
};
typedef struct Station Station;

void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * return -1 for not found, index otherwise
 */
int contain(double val, double *arr, int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        if (val == arr[i]) {return i;}
    }
    return -1;
}

/**
 * returns ditance from x, y to ox, oy
 * manhattan distance is fine, euclidean distance not needed
 */
int dist(int ox, int oy, int x, int y)
{
    return abs(ox - x) + abs(oy - y);
}

/**
 * line of sight is determined by angle from origin point (x, y)
 * thus can be calculated for all 4 quadrants via arc tangent
 *
 * if the angle is already present, add the value with the smallest distance from the origin
 */
void getlos(Station *stat, int x, int y)
{
    int i, j, cnt, fnd;
    double angle;

    stat->x = x;
    stat->y = y;

    cnt = stat->nlos = 0;
    for (i = 0; i < stat->h; i++)
    {
        for (j = 0; j < stat->w; j++)
        {
            if (stat->x == j && stat->y == i) {continue;}

            if (stat->map[i][j] == ASTER)
            {
                // order of params normalises the angle so that the +y axis starts from 0 going clockwise
                // normal parameters are atan2(i - stat->y, j - stat->x)
                // does not change the result of part 1
                angle = atan2(j - stat->x, stat->y - i);
                fnd = contain(angle, stat->los, cnt);

                if (fnd == -1)
                {
                    stat->los[cnt] = angle;
                    // store position of los asteroid
                    stat->los_pos[cnt][0] = j;
                    stat->los_pos[cnt][1] = i;
                    cnt++;
                }
                else if (dist(x, y, j, i) < dist(x, y, stat->los_pos[fnd][0], stat->los_pos[fnd][1]))
                {
                    // same angle, but closer to origin
                    stat->los[fnd] = angle;
                    stat->los_pos[fnd][0] = j;
                    stat->los_pos[fnd][1] = i;
                }
            }
        }
    }
    stat->nlos = cnt;
}

int main()
{
    int i, j, index, max;
    int best[2] = {0};
    double min;
    Station *stat = malloc(sizeof(*stat));
    if (!stat) {return -1;}
    
    i = 0;
    while (fgets(stat->map[i], sizeof(stat->map[i]), stdin)) {*strchr(stat->map[i++], '\n') = '\0';}
    stat->h = i;
    stat->w = strlen(stat->map[0]);

    // part 1 - find asteroid with line of sight of the most asteroids
    max = 0;
    for (i = 0; i < stat->h; i++)
    {
        for (j = 0; j < stat->w; j++)
        {
            if (stat->map[i][j] == ASTER)
            {
                // get asteroids that are in line of sight of current
                // zeroing the los array not needed 
                getlos(stat, j, i);
                if (stat->nlos > max)
                {
                    max = stat->nlos;
                    best[0] = j;
                    best[1] = i;
                }
            }
        }
    }
    printf("part 1 - %d asteroids visible at %d,%d\n", max, best[0], best[1]);

    // part 2 - for the asteroid from part 1 destroy the first 200 asteroids in line of sight
    // start north with clockwise rotation
    // we can cheat slightly because the number of asteroids found is > 200,
    // otherwise use buckets when storing los_pos or multiple passes
    getlos(stat, best[0], best[1]);
    
    // sort los_pos based on angle (quadrant check order a-d) with atan2(y, x) swapped to atan2(x, y)
    //       0
    //     d | a
    // -90 --|-- 90
    //     c | b
    //   -180/180
    // atan2 returns (-180, 180] so we can deal with negative angles by adding 2pi radians (360)

    // returned angle from getlos() is already normalised to CW rotation, just need to sort and extend to [0, 360)
    for (i = 0; i < stat->nlos; i++)
    {
        if (stat->los[i] < 0) {stat->los[i] += 2 * M_PI;}
    }

    // insertion sort
    for (i = 0; i < stat->nlos; i++)
    {
        min = stat->los[i];
        index = -1;
        for (j = i + 1; j < stat->nlos; j++)
        {
            if (stat->los[j] < min)
            {
                min = stat->los[j];
                index = j;
            }
        }

        // swap
        if (index > 0)
        {
            stat->los[index] = stat->los[i];
            stat->los[i] = min;
            swap(&stat->los_pos[i][0], &stat->los_pos[index][0]);
            swap(&stat->los_pos[i][1], &stat->los_pos[index][1]);
        }
    }

    best[0] = stat->los_pos[ASTREM - 1][0];
    best[1] = stat->los_pos[ASTREM - 1][1];
    printf("part 2 - 200th destroyed asteroid at %d,%d -> %d\n", best[0], best[1], best[0] * 100 + best[1]);

    free(stat);

    return 0;
}
