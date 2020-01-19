#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#define DIM 81
#define NKEY 26

enum Tile      {Key = 0, Door = 1, Wall = '#', Move = '.', Robot = '@'};
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

/**
 * Function headers
 */
int  qempty   (Queue *);
void enq      (Queue *, int);
int  deq      (Queue *);
int  istype   (char);
int  contain  (char, int);
void addkey   (char, int *);
void remkey   (char, int *);
int  allkeyfnd(int);
char showdist (int *, int, Point *, char, int, int);
void search   (char [DIM][DIM + 3], int, int, int, Queue *, int *, int);

/**
 * Queue functions
 */
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

/**
 * Key functions
 */
int istype(char c)
{
    return isalpha(c) ? islower(c) ? Key : Door : c;
}

int contain(char c, int keys)
{
    return keys & (1 << (tolower(c) - 'a'));
}

void addkey(char c, int *keys)
{
    *keys |= 1 << (tolower(c) - 'a');
}

void remkey(char c, int *keys)
{
    *keys &= ~(1 << (tolower(c) - 'a'));
}

int allkeyfnd(int keys)
{
    int i, n;
    for (i = 0, n = 0; i < NKEY; i++) {n += (1 << i);} // 67108863
    return keys == n;
}

/**
 * prints the distances from orig to all reachable keys
 * also calculates the key that is the shortest distance away that hasn't already
 * been found and returns it's name
 */
char showdist(int *dist, int w, Point *loc, char orig, int keys, int print)
{
    char minchar = 'A';
    int i, index, minval = INT_MAX;
    printf("\nstarting from key %c:\n", orig);
    for (i = 'a'; i <= 'z'; i++)
    {
        index = loc[i].y * w + loc[i].x;
        if (i == orig || !dist[index]) {continue;}
        if (dist[index] < minval && !contain(i, keys))
        {
            minval = dist[index];
            minchar = i;
        }
        if (print) {printf("dist from %c to %c: %d\n", orig, i, dist[index]);}
    }
    if (print)
    {
        if (minchar != 'A') {printf(" -smallest dist to %c is %d\n", minchar, minval);}
        else                {printf(" -no keys can be moved to\n");}
    }
    return minchar;
}

/**
 * breadth-first search from the point x, y of a maze w x w in size
 * the search will not go past a key if it detects one, or a door if the key for that door
 * in keys is not found, thus it will only record immediately reachable points
 * keys are stored in a bitmap as there are only 26 maximum keys
 */
void search(char maze[DIM][DIM + 3], int x, int y, int w, Queue *que, int *dist, int keys)
{
    int i, cur, cx, cy, next, nx, ny, pos;

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

            // don't need bounds checks as they're all walls
            if ((!dist[next] || dist[cur] + 1 < dist[next]) && pos != Wall)
            {
                dist[next] = dist[cur] + 1;
                // we don't add keys that we already have
                if (pos == Move || (contain(pos, keys) && isalpha(pos)))
                {
                    enq(que, nx);
                    enq(que, ny);
                }
            }
        }
    }
}

int main()
{
    const int lsize = 'z' + 1;
    int i, j, msize, fndkey = 0, min, cmin, rx, ry;
    char maze[DIM][DIM + 3];
    int **dist;
    Queue *que;
    Point loc[lsize];

    i = 0;
    while (fgets(maze[i++], sizeof(maze[0]), stdin)) {;}

    // record key and door locations -- keys are lowercase, doors are uppercase
    rx = ry = 0;
    for (i = 0; i < DIM; i++)
    {
        for (j = 0; j < DIM; j++)
        {
            if (isalpha(maze[i][j]))
            {
                loc[(int) maze[i][j]].x = j;
                loc[(int) maze[i][j]].y = i;
            }
            if (maze[i][j] == Robot)
            {
                rx = j;
                ry = i;
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

    // fill dist array with distance from each key (and origin) to every other reachable key
    // i.e, we ignore keys that are blocked by doors or other keys
    min = cmin = INT_MAX;
    search(maze, rx, ry, DIM, que, dist[NKEY], fndkey);
    showdist(dist[NKEY], DIM, loc, '@', fndkey, 1);
    for (i = 'a'; i <= 'z'; i++)
    {
        search(maze, loc[i].x, loc[i].y, DIM, que, dist[i - 'a'], fndkey);
        showdist(dist[i - 'a'], DIM, loc, i, fndkey, 1);
    }

    free(que->val);
    free(que);
    for (i = 0; i < NKEY + 1; i++) {free(dist[i]);}
    free(dist);

    return 0;
}
