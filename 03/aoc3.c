#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#define LEN 2000
#define PTS LEN / 2
#define NLINE 2
#define DELIN ","

struct Point {
    int x;
    int y;
};
typedef struct Point Point;

struct Line {
    Point p1;
    Point p2;
};
typedef struct Line Line;

static const Point EmptyPoint = {0};

// direction mapping vector
int dvector[][2] = {
    ['U'] = {0, -1}, // up
    ['D'] = {0, 1},  // down
    ['L'] = {-1, 0}, // left
    ['R'] = {1, 0}   // right
};

int manhattan(Point a, Point b)
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

/**
 * If a and b are perpendicular to each other than return
 * 'h' if a is a horizontal line or 'v' if a is vertical
 * otherwise return 'n' if neither.
 */
char perpendicular(Line a, Line b)
{
    char ret = 'n';
    if      (a.p1.x == a.p2.x && b.p1.y == b.p2.y) {ret = 'v';}
    else if (a.p1.y == a.p2.y && b.p1.x == b.p2.x) {ret = 'h';}
    return ret;
}

/**
 * vl (vertical) and hl (horizontal) are perpendicular lines, so check if they intersect
 * returns an empty point (EmptyPoint) if no intersection is found, otherwise
 * returns the (singular) point at which they intersect.
 */
Point intersection(Line vl, Line hl)
{
    // ignore intersections at the origin
    Point p = EmptyPoint;
    if (vl.p1.x >= hl.p1.x && vl.p1.x <= hl.p2.x)
    {
        if (hl.p1.y >= vl.p1.y && hl.p1.y <= vl.p2.y)
        {
            p.x = vl.p1.x;
            p.y = hl.p1.y;
        }
    }
    return p;
}

void swap(Point *a, Point *b)
{
    Point temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * Closest intersection to origin (0,0) using manhattan distance
 */
void min_intersection(Point *inter, int size)
{
    int i, min, dist, index;
    index = 0;
    min = INT_MAX;

    for (i = 0; i < size; i++)
    {
        dist = abs(inter[i].x) + abs(inter[i].y);
        if (dist < min)
        {
            min = dist;
            index = i;
        }
    }
    printf("minimum intersection at %d,%d -> %d distance\n", inter[index].x, inter[index].y, min);
}

int main()
{
    char *line, *parse;
    char direction;
    int *totaldist;
    int npts[NLINE] = {0}, linedist[NLINE] = {0};
    int i, j, val, ninter, xprev, yprev, min, maxinter;
    Point *inter;
    Point pprev[NLINE], tpt[NLINE], ptemp;
    Line ln1, ln2;
    
    Point **p = malloc(sizeof(*p) * NLINE);
    if (!p) {return -1;}
    for (i = 0; i < NLINE; i++)
    {
        p[i] = malloc(sizeof(**p) * PTS);
        if (!p[i]) {return -2;}
    }

    line = malloc(sizeof(*line) * LEN);if (!line) {return -1;}

    // parse input and convert into arrays of points per line
    i = 0;
    while (fgets(line, sizeof(*line) * LEN, stdin) && i < NLINE)
    {
        xprev = yprev = 0;
        parse = strtok(line, DELIN);
        while (parse != NULL)
        {
            // remove the first character so it can be converted to an integer
            direction = parse[0];
            parse[0] = ' ';
            val = atoi(parse);

            // stores the point relative to the origin (0,0), rather than each other
            p[i][npts[i]].x = xprev + dvector[(int) direction][0] * val;
            p[i][npts[i]].y = yprev + dvector[(int) direction][1] * val;
            xprev = p[i][npts[i]].x;
            yprev = p[i][npts[i]].y;

            npts[i]++;

            parse = strtok(NULL, DELIN);
        }
        i++;
    }

    // find intersections, starting point is the origin
    // there's really only 2 lines, so cheat here
    ninter = 0;
    maxinter = npts[0] * npts[1];
    inter = calloc(maxinter, sizeof(*inter));
    totaldist = calloc(maxinter, sizeof(*totaldist));  
    if (!totaldist || !inter) {return -3;}

    pprev[0] = EmptyPoint;
    for (i = 0; i < npts[0]; i++)
    {
        ln1.p1 = pprev[0];
        ln1.p2 = tpt[0] = p[0][i];
        pprev[0] = ln1.p2;

        linedist[0] += manhattan(ln1.p1, ln1.p2);
        linedist[1] = 0;

        pprev[1] = EmptyPoint;
        for (j = 0; j < npts[1]; j++)
        {
            ln2.p1 = pprev[1];
            ln2.p2 = tpt[1] = p[1][j];
            pprev[1] = ln2.p2;

            linedist[1] += manhattan(ln2.p1, ln2.p2);

            switch (perpendicular(ln1, ln2))
            {
                case 'h':
                    // swap points in each line so that the smaller point is first
                    if (ln1.p1.x > ln1.p2.x) {swap(&ln1.p1, &ln1.p2);}
                    if (ln2.p1.y > ln2.p2.y) {swap(&ln2.p1, &ln2.p2);}
                    ptemp = intersection(ln2, ln1);
                    break;
                case 'v':
                    if (ln1.p1.y > ln1.p2.y) {swap(&ln1.p1, &ln1.p2);}
                    if (ln2.p1.x > ln2.p2.x) {swap(&ln2.p1, &ln2.p2);}
                    ptemp = intersection(ln1, ln2);
                    break;
                default:
                    continue;
            }

            // non-origin points only
            if (ptemp.x || ptemp.y)
            {
                inter[ninter] = ptemp;
                totaldist[ninter] = linedist[0] + linedist[1] - manhattan(tpt[0], ptemp) - manhattan(tpt[1], ptemp);
                ninter++;
            }
        }
    }

    min_intersection(inter, ninter);

    for (i = 0; i < ninter; i++)
    {
        if (min && totaldist[i] < min) {min = totaldist[i];}
    }
    printf("least steps taken to closest intersection %d\n", min);

    for (i = 0; i < NLINE; i++) {free(p[i]);}
    free(p);
    free(totaldist);

    return 0;
}
