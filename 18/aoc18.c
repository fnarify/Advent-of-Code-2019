#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#define DIM 81
#define MID (DIM / 2)
#define NKEY 26

enum Tile      {Wall = '#', Move = '.'};
enum Direction {North = 1, South = 2, West = 3, East = 4};

const int vec[][2] = {
    [North] = {0, -1},
    [South] = {0, 1},
    [West]  = {-1, 0},
    [East]  = {1, 0}
};

struct Point {
    int x, y;
};
typedef struct Point Point;

struct Queue {
    int *val;
    int front, rear, size;
};
typedef struct Queue Queue;

int  qempty   (Queue *);
void enq      (Queue *, int);
int  deq      (Queue *);
int  iskey    (char);
int  isdoor   (char);
void printdist(int *, Point *, char, int);
int  search   (char [DIM][DIM + 3], int, int, int, Queue *, int *);

int qempty(Queue *q)
{
    return q->front > q->rear;
}

void enq(Queue *q, int item)
{
    if (q->rear == q->size - 1)
    {
        printf("Queue is full\n");
        q->rear = -1;
        q->front = 0;
    }
    q->val[++q->rear] = item;
}

int deq(Queue *q)
{
    if (q->front == q->size)
    {
        printf("Queue is empty\n");
        q->front = 0;
        return INT_MIN;
    }
    return q->val[q->front++];
}

int iskey(char c)
{
    return isalpha(c) && islower(c);
}

int isdoor(char c)
{
    return isalpha(c) && isupper(c);
}

void printdist(int *dist, Point *loc, char orig, int w)
{
    int i, cur;
    printf("\nstarting from key %c:\n", orig);
    for (i = 'a'; i < 'z'; i++)
    {
        cur = loc[i].y * w + loc[i].x;
        if (i == orig || !dist[cur]) {continue;}
        printf("dist from %c to %c: %d\n", orig, i, dist[cur]);
    }
}

/**
 * maybe should be a depth-first search as it would let me properly add key/door pairs
 * currently does a breadth-first search from the point x, y of a maze w x w in size
 * OR just stop the search when you hit a door that you don't have a key for
 *
 * returns how many keys it can reach
 */
int search(char maze[DIM][DIM + 3], int x, int y, int w, Queue *que, int *dist)
{
    int i, cur, cx, cy, next, nx, ny, pos, kfound;

    kfound = 0;
    que->front = 0;
    que->rear = -1;
    enq(que, x);
    enq(que, y);

    while (!qempty(que))
    {
        cx = deq(que);
        cy = deq(que);
        cur = cy * w + cx;

        // add all adjacents
        for (i = North; i <= East; i++)
        {
            nx = cx + vec[i][0];
            ny = cy + vec[i][1];
            next = ny * w + nx;
            pos = maze[ny][nx];

            if ((pos == Move || iskey(pos)) && !dist[next]
                && cx && cy && cx < w && cy < w)
            {
                enq(que, nx);
                enq(que, ny);
                dist[next] = dist[cur] + 1;
                if (iskey(pos)) {kfound++;}
            }
        }
    }
    return kfound;
}

int main()
{
    const int lsize = 'z' + 1;
    int i, j, msize;
    char maze[DIM][DIM + 3];
    int **dist;
    Queue *que;
    Point loc[lsize];

    i = 0;
    while (fgets(maze[i++], sizeof(maze[0]), stdin)) {;}

    // record key and door locations
    // keys are lowercase, doors are uppercase
    for (i = 0; i < DIM; i++)
    {
        for (j = 0; j < DIM; j++)
        {
            if (isalpha(maze[i][j]))
            {
                loc[(int) maze[i][j]].x = j;
                loc[(int) maze[i][j]].y = i;
            }
        }
    }

    msize = DIM * DIM;
    que = malloc(sizeof(*que));
    dist = malloc(sizeof(*dist) * (NKEY + 1));
    if (!que || !dist) {return -1;}

    for (i = 0; i < NKEY + 1; i++)
    {
        dist[i] = calloc(msize, sizeof(*dist[0]));
        if (!dist[i]) {return -2;}
    }

    que->size = msize * 2;
    que->val = malloc(sizeof(*que->val) * que->size);
    if (!que->val) {return -3;}

    // fill dist array with distance from each key (and origin) to every other key
    search(maze, MID, MID, DIM, que, dist[NKEY]);
    printdist(dist[NKEY], loc, '@', DIM);
    for (i = 'a'; i < 'z'; i++)
    {
        search(maze, loc[i].x, loc[i].y, DIM, que, dist[i - 'a']);
        printdist(dist[i - 'a'], loc, i, DIM);
    }

    free(que->val);
    free(que);
    for (i = 0; i < NKEY + 1; i++) {free(dist[i]);}
    free(dist);

    return 0;
}
