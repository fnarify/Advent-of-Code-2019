#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#define LEN 100
#define N_STEP 1000
#define N_MOON 4
#define DIM 3

void print(int pos[N_MOON][DIM], int vel[N_MOON][DIM])
{
    int i;
    for (i = 0; i < N_MOON; i++)
    {
        printf("pos=<x=%d, y=%d, z=%d>, vel=<x=%d, y=%d, z=%d>\n",
                pos[i][0], pos[i][1], pos[i][2], vel[i][0], vel[i][1], vel[i][2]);
    }
}

int64_t gcd(int64_t a, int64_t b)
{
    while (a != b)
    {
        if (a > b) {a -= b;}
        else       {b -= a;}
    }
    return a;
}

int64_t lcm(int64_t a, int64_t b)
{
    if (a == b) {return a;}
    return (a * b) / gcd(a, b);
}

int check(int a, int b)
{
    int ret = 0;
    if      (a < b) {ret = 1;}
    else if (a > b) {ret = -1;}
    return ret;
}

/**
 * check if the 2-d array a is the same as b for fixed x = n
 * vel just check it's all 0 as the velocity array starts at {0}
 */
int equal(int a[N_MOON][DIM], int b[N_MOON][DIM], int vel[N_MOON][DIM], int n)
{
    int i;
    for (i = 0; i < N_MOON; i++)
    {
        if (a[i][n] != b[i][n] || vel[i][n]) {return 0;}
    }
    return 1;
}

int main()
{
    char line[LEN];
    int moon[N_MOON][DIM], orig_m[N_MOON][DIM], velocity[N_MOON][DIM] = {0};
    int potential[N_MOON] = {0}, kinetic[N_MOON] = {0};
    int64_t cycle[DIM] = {0};
    int i, j, k, sum;
    int64_t n;

    i = 0;
    while (fgets(line, sizeof(line), stdin))
    {
        sscanf(line, "<x=%d, y=%d, z=%d>\n", &moon[i][0], &moon[i][1], &moon[i][2]);
        i++;
    }

    // set initial state
    for (i = 0; i < N_MOON; i++)
    {
        for (j = 0; j < DIM; j++) {orig_m[i][j] = moon[i][j];}
    }

    n = sum = 0;
    // exit when all cycles have been detected
    while (n++ < LLONG_MAX - 1 && (!cycle[0] || !cycle[1] || !cycle[2]))
    {
        // calculate velocity
        for (i = 0; i < N_MOON; i++)
        {
            for (j = 0; j < N_MOON; j++)
            {
                if (i == j) {continue;}
                for (k = 0; k < DIM; k++) {velocity[i][k] += check(moon[i][k], moon[j][k]);}
            }
        }

        // add velocity
        for (i = 0; i < N_MOON; i++)
        {
            for (j = 0; j < DIM; j++)
            {
                moon[i][j] += velocity[i][j];
            }
        }

        // part 1
        if (n == N_STEP)
        {
            // calculate energy of system
            for (i = 0; i < N_MOON; i++)
            {
                for (j = 0; j < DIM; j++)
                {
                    potential[i] += abs(moon[i][j]);
                    kinetic[i] += abs(velocity[i][j]);
                }
            }
            for (i = 0; i < N_MOON; i++) {sum += potential[i] * kinetic[i];}
            printf("part 1 - total energy of %d after %d steps\n", sum, N_STEP);
        }

        // part 2 - record cycle for each dimension individually as they're independant
        for (i = 0; i < DIM; i++)
        {
            if (!cycle[i] && equal(orig_m, moon, velocity, i)) {cycle[i] = n;}
        }
    }
    printf("%lld steps to return to starting state\n", lcm(lcm(cycle[0], cycle[1]), cycle[2]));

    return 0;
}
