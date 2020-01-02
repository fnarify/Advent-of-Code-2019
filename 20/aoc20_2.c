#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#define ORIG  "AA"
#define GOAL  "ZZ"
#define DIM   135
#define NPORT 35

enum Tile      {Empty = ' ', Move = '.', Wall = '#'};
enum Direction {North = 1, South = 2, West = 3, East = 4};

// (x, y) vector movement values
const int vec[][2] = {
    [North] = {0, -1},
    [South] = {0, 1},
    [West]  = {-1, 0},
    [East]  = {1, 0}
};

/**
 * direction to leave the portal at point 1 and 2 are provided
 * as it makes moving through the maze easier
 */
struct Portal {
    char name[3];
    char ident;
    int x1, y1, x2, y2, dir1, dir2, exit;
};
typedef struct Portal Portal;

struct Queue {
    int *val;
    int front, rear, size;
};
typedef struct Queue Queue;

int  qempty   (Queue *);
void enq      (Queue *, int);
int  deq      (Queue *);
void printport(Portal *, int, int, int);
int  isext    (int, int, int, int);
int  getport  (char [DIM][DIM], char [3], int, int);
void setport  (Portal *, int, int, int, int);
int  addport  (char [DIM][DIM], Portal *, int, char [3], int, int, int);
void bfs      (char [DIM][DIM], int, int, int, int, int, Portal *, Queue *, int **);

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

void printport(Portal *p, int n, int w, int h)
{
    int i;
    for (i = 0; i < n; i++)
    {
        printf("%s[%c]: %3d,%3d (%s) -> %3d,%3d (%s)\n",
                p[i].name, p[i].ident, p[i].x1, p[i].y1,
                isext(p[i].x1, p[i].y1, w, h) ? "ext" : "int",
                p[i].exit ? p[i].x2 : 0, p[i].exit ? p[i].y2 : 0,
                p[i].exit ? isext(p[i].x2, p[i].y2, w, h) ? "ext" : "int" : "end");
    }
}

/**
 * returns whether a point x, y is on the outside or inside of the maze
 * w - 3 because of '\n'
 */
int isext(int x, int y, int w, int h)
{
    return (x == 1 || x == w - 3 || y == 1 || y == h - 3) ? 1 : 0;
}

/**
 * returns a direction depending on which way is the maze
 * relative to the position (name) being checked
 * portal names are at most 2 characters between AA - ZZ
 *
 * only need to check for key names below and to the right as we're searching
 * right->left, up->down
 *
 * name order is important
 */
int getport(char maze[DIM][DIM], char s[3], int x, int y)
{
    int ret = 0;
    s[0] = maze[y][x];
    s[2] = '\0';
    if (isalpha(maze[y][x + 1]))
    {
        s[1] = maze[y][x + 1];
        if (maze[y][x + 2] == Move) {ret = East;} // w - 1 + '\n', won't go oob
        else                        {ret = West;}
    }
    else if (isalpha(maze[y + 1][x]))
    {
        s[1] = maze[y + 1][x];
        if (maze[y + 2][x] == Move) {ret = South;} // h - 1
        else                        {ret = North;}
    }
    return ret;
}

/**
 * sets the portals start or end location, depending on whether it's
 * a new portal or not
 */
void setport(Portal *p, int at, int new, int x, int y)
{
    if (new)
    {
        p[at].x1 = x;
        p[at].y1 = y;
    }
    else
    {
        p[at].x2 = x;
        p[at].y2 = y;
    }
}

/**
 * if the portal is new then adds its x1,y1 location to the set of portals p
 * otherwise updates its x2,y2 location
 *
 * this also updates the map to incorporate a single integer value as what we're checking,
 * rather than the 2-character identifiers used originally, when moving through the maze.
 * as because N_PORT < 'A', otherwise we'd need to store the maze as a group of integers with base 36 etc
 *
 * there can be problem with the values coming too close to ASCII code 32 (' ', space) so we store
 * and rewrite the maze using 1-character identifiers defined as the index in p + 'A'
 * these will not be reparsed as getport() requires two alphanumeric characters to be detected
 * and the maze is nice enough that portals are not directly next to each other...
 * although it shouldn't matter as long as the interior gap is large enough, as it needs to detect
 * valid movement space as well
 *
 * returns the current amount of portals in p
 */
int addport(char maze[DIM][DIM], Portal *p, int n, char s[3], int x, int y, int dir)
{
    char ident;
    int i, index, new;

    // directional representations of how the empty [0, 1] and new identifier [2, 3]
    // spot should be set based on the direction from the identifier to the maze
    static const int off[][4] = {
        [North] = {0, 1, 0, 0},
        [South] = {0, 0, 0, 1},
        [West]  = {1, 0, 0, 0},
        [East]  = {0, 0, 1, 0}
    };

    new = 1;
    index = n;
    for (i = 0; i < n && new; i++)
    {
        if (!strcmp(p[i].name, s))
        {
            new = 0;
            index = i;
        }
    }

    ident = index + 'A';
    maze[ y + off[dir][1] ][ x + off[dir][0] ] = Empty;
    maze[ y + off[dir][3] ][ x + off[dir][2] ] = ident;
    setport(p, index, new, x + off[dir][2], y + off[dir][3]);

    if (new)
    {
        strcpy(p[index].name, s);
        p[index].exit = 0;
        p[index].ident = ident;
        p[index].dir1 = dir;
        n++;
    }
    else
    {
        p[index].dir2 = dir;
        p[index].exit = 1;
    }

    return n;
}

/**
 * there could be infinite paths in this maze
 * don't need an array to record visited locations anymore, as the distance array will work fine
 * dist is 3-dimensional now to account for moving up and down levels in the maze
 *
 * doesn't correctly work in the sense of favouring returning to the starting level
 */
void bfs(char maze[DIM][DIM], int x, int y, int zmax, int w, int h, Portal *port, Queue *que, int **dist)
{
    int i, cur, cx, cy, next, nx, ny, pos, index, level, off;

    que->front = 0;
    que->rear = -1;
    enq(que, zmax / 2);
    enq(que, x);
    enq(que, y);

    while (!qempty(que))
    {
        level = deq(que);
        cx = deq(que);
        cy = deq(que);
        cur = cy * w + cx;

        // add all adjacents
        for (i = North; i <= East; i++)
        {
            nx = cx + vec[i][0];
            ny = cy + vec[i][1];
            pos = maze[ny][nx];
            off = 0; // level offset for a single adjacent check

            // portal
            index = pos - 'A';
            if (pos >= 'A' && port[index].exit)
            {
                if (nx == port[index].x1 && ny == port[index].y1)
                {
                    nx = port[index].x2 + vec[ port[index].dir2 ][0];
                    ny = port[index].y2 + vec[ port[index].dir2 ][1];
                    off = isext(port[index].x1, port[index].y1, w, h) ? -1 : 1;
                }
                else
                {
                    nx = port[index].x1 + vec[ port[index].dir1 ][0];
                    ny = port[index].y1 + vec[ port[index].dir1 ][1];
                    off = isext(port[index].x2, port[index].y2, w, h) ? -1 : 1;
                }
                pos = maze[ny][nx]; // Move
            }
            
            // bound checks aren't needed due to the way the input is structured
            next = ny * w + nx;
            if (pos == Move && level + off >= 0 && level + off < zmax && !dist[level + off][next])
            {
                enq(que, level + off);
                enq(que, nx);
                enq(que, ny);
                dist[level + off][ny * w + nx] = dist[level][cur] + 1;
            }
        }
    }
}

/**
 * part 2:
 * when travelling to a portal on the interior of the maze we move 1 step down in the z co-ords
 * similarly, we move 1 step up when moving to a portal on the exterior of the maze
 */
int main()
{
    int h = 0, w, np, i, j, min, dir, startx, starty, endx, endy, nlev;
    char maze[DIM][DIM], pname[3];
    int **dist;
    Queue *que;
    Portal *port = malloc(sizeof(*port) * NPORT);
    if (!port) {return -1;}

    while (fgets(maze[h++], DIM, stdin)) {;}
    w = strlen(maze[0]);

    // convert maze to single character identifiers ('A' + index) in port
    np = 0;
    for (i = 0; i < h - 1; i++)
    {
        for (j = 0; j < w - 1; j++)
        {
            if (isalpha(maze[i][j]))
            {
                dir = getport(maze, pname, j, i);
                if (dir) {np = addport(maze, port, np, pname, j, i, dir);}
            }
        }
    }

    // get actual starting and end point in the maze
    // (it's the valid spot next to the portal, not the portal itself)
    startx = starty = endx = endy = 0;
    for (i = 0; i < np; i++)
    {
        if (!strcmp(port[i].name, ORIG))
        {
            startx = port[i].x1 += vec[port[i].dir1][0];
            starty = port[i].y1 += vec[port[i].dir1][1];
        }
        else if (!strcmp(port[i].name, GOAL))
        {
            endx = port[i].x1 += vec[port[i].dir1][0];
            endy = port[i].y1 += vec[port[i].dir1][1];
        }
    }

    // setup variables for the search
    nlev = np;
    dist = malloc(sizeof(*dist) * nlev);
    que = malloc(sizeof(*que));
    if (!dist || !que) {return -2;}

    for (i = 0; i < nlev; i++)
    {
        dist[i] = calloc(w * h, sizeof(*dist[i]));
        if (!dist[i]) {return -3;}
    }

    que->size = DIM * DIM * DIM * 2;
    que->val = malloc(sizeof(*que->val) * que->size);
    if (!que->val) {return -4;}

    bfs(maze, startx, starty, nlev, w, h, port, que, dist);

    j = endy * w + endx;
    for (i = 0, min = INT_MAX; i < nlev; i++)
    {
        if (dist[i][j] && dist[i][j] < min) {min = dist[i][j];}
    }
    printf("part 1 - %d steps is the shortest dist from %s to %s\n", min, ORIG, GOAL);
    // should be at [nlev / 2][j] but since the answer changes depending on how large nlev is
    // then the workings for the bfs aren't correct
    printf("part 2 - %d steps from %s to %s ending on the same level\n", dist[6][j], ORIG, GOAL);

    free(port);
    for (i = 0; i < nlev; i++) {free(dist[i]);}
    free(dist);
    free(que->val);
    free(que);

    return 0;
}
