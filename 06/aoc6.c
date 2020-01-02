#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_PLANETS 1000000
#define MAX_MAP 1500
#define BASE 36

int contain(unsigned *map, int msize, unsigned item)
{
    int i;
    for (i = 0; i < msize; i++)
    {
        if (map[i] == item) {return i;}
    }
    return -1;
}

/**
 * input is in the form s1)s2 for strings s1 and s2 
 * effectively a hashmap with key=orbit, value = parent
 * it's stored as a base 36 number since the input is only between
 * A-Z and 0-9 and we can truncated the value to save space as the input
 * is at most 3 characters long
 *
 * we can do this because one planet can only 'directly' orbit a single planet
 *
 */
int main()
{
    int i, msize, index, sum, found;
    char line[20], parent[4], orbit[4];
    unsigned *planets = calloc(MAX_PLANETS, sizeof(*planets));
    unsigned *map = malloc(sizeof(*map) * MAX_MAP);
    if (!planets || !map) {return -1;}

    msize = 0;
    while (fgets(line, sizeof(line), stdin))
    {
        // since sscanf is greedy change parenthesis to a space etc
        *strchr(line, ')') = ' ';
        if (sscanf(line, "%3s %3s", parent, orbit) != EOF)
        {
            index = (unsigned) strtoul(orbit, NULL, BASE);
            if (index >= MAX_PLANETS) {printf("Out of bounds, need more memory\n"); return -2;}
            planets[index] = strtoul(parent, NULL, BASE);
            map[msize++] = index;
        }
    }

    // part 1: count direct and indirect orbits
    sum = 0;
    for (i = 0; i < msize; i++)
    {
        index = map[i];
        while (planets[index])
        {
            sum++;
            index = planets[index];
        }
    }
    printf("total orbits: %d\n", sum);

    // part 2: total distance from YOU to SAN
    // build list of orbits from YOU to COM (origin) record size
    // then check orbits from SAN to COM until you find an orbit already in your prev list
    sum = msize = 0;
    index = (unsigned) strtoul("YOU", NULL, BASE);
    map[msize++] = index;
    while (planets[index])
    {
        index = planets[index];
        map[msize++] = index;
    }

    index = (unsigned) strtoul("SAN", NULL, BASE);
    while (planets[index])
    {
        found = contain(map, msize, planets[index]);
        if (found != -1)
        {
            printf("total distance: %d\n", sum + found - 1);
            break;
        }

        index = planets[index];
        sum++;
    }

    free(planets);
    free(map);

    return 0;
}
