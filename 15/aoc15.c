#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBUG 0 // extra printing details

/**
 * need C99 standards (-std=c99) for this program as we need int64_t 
 * (or long long) to be defined, also to print lld thanks to Windows' poor C standards compliance
 * you can use PRId64 for example for Windows.
 */
#define NEED_IN  LLONG_MIN
#define HALTED   LLONG_MIN + 1
#define UNKNOWN  LLONG_MIN + 2
#define NEED_OUT LLONG_MIN + 3

#define BUFF  5
#define DIM   42
#define LEN   10000
#define DELIN ","

/**
 * needs to be able to store ~50 digit values, so we need to use int64_t
 */
struct Comp {
    int64_t *inst;
    int64_t ninst, ip, out[BUFF], in[BUFF], inoff, outoff, base, maxmem;
};
typedef struct Comp Comp;

struct Stack {
    int *val;
    int loc, size;
};
typedef struct Stack Stack;

struct Queue {
    int *val;
    int front, rear, size;
};
typedef struct Queue Queue;

// function prototypes
void    push     (Stack *, int);
int     pop      (Stack *);
bool    sempty   (Stack *);
void    enq      (Queue *, int);
int     deq      (Queue *);
bool    qempty   (Queue *);
void    resetamp (Comp *, int64_t *, int64_t);
void    resetbuff(Comp *);
void    findmodes(int, int *);
int64_t detmode  (Comp *, int, int);
int64_t eval     (Comp *, int, int);
int     moveto   (Comp *, int);
void    search   (Comp *, Stack *, char *, bool *, int *, int, int, int, int);
void    bfs      (char *, Queue *, bool *, int *, int);

enum Mode {Position = 0, Immediate = 1, Relative = 2};
enum Code {ADD = 1, MUL = 2, WRITE = 3, READ = 4, JNZ = 5, JEZ = 6, CMPL = 7, CMPE = 8, RELB = 9, EXIT = 99};
enum Location {Wall = 0, Moved = 1, Oxygen = 2};
enum Move {North = 1, South = 2, West = 3, East = 4};

const int tile[] = {
    [Wall] = '#',
    [Moved] = '.',
    [Oxygen] = 'O',
};

const int inv[] = {
    [North] = South,
    [South] = North,
    [West] = East,
    [East] = West,
};

const int vec[][2] = {
    [North] = {0, -1},
    [South] = {0, 1},
    [West] = {-1, 0},
    [East] = {1, 0}
};

void push(Stack *s, int item)
{
    if (s->loc < s->size) {s->val[s->loc++] = item;}
    else {printf("Stack is full\n");}
}

int pop(Stack *s)
{
    int ret = INT_MIN;
    if (!sempty(s)) {ret = s->val[--s->loc];}
    else {printf("Stack is empty\n");}
    return ret;
}

bool sempty(Stack *s)
{
    return s->loc < 1;
}

bool qempty(Queue *q)
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
 * resets amp program to default state
 */
void resetamp(Comp *amp, int64_t *op, int64_t opsize)
{
    memcpy(amp->inst, op, sizeof(*op) * opsize);
    amp->ip = 0;
    amp->base = 0;
    amp->inoff = 0;
    amp->outoff = 0;
}

void resetbuff(Comp *amp)
{
    amp->inoff = 0;
    amp->outoff = 0;
}

/**
 * a is of the form ABCDE where they are digits
 * DE - 2-digit opcode
 * C - 1st param mode
 * B - 2nd param mode
 * A - 3rd param mode
 * A, B, C don't always have to be present
 *
 * a is never greater than 5 digits right now, so truncating to an int is fine
 */
void findmodes(int a, int m[3])
{
    int i;
    a /= 10;
    for (i = 0; i < 3; i++)
    {
        a /= 10;
        m[i] = a % 10;
    }
}

/**
 * if mode is 2 (relative) return base + value of the index
 * if mode is 1 (immediate) return index
 * if mode is 0 (position) return value of its index
 *
 * this provides the index to access in a->inst[]
 */
int64_t detmode(Comp *a, int ioff, int mode)
{
    switch (mode)
    {
        case Position:
            return a->inst[a->ip + ioff];
        case Immediate:
            return a->ip + ioff;
        case Relative:
            return a->inst[a->ip + ioff] + a->base;
        default:
            printf("Unknown mode\n");
            return 0; // return 0 so it doesn't segfault, this shouldn't be reached
    }
}

/**
 * handle instruction and return last output
 * mode A will never apply to an instruction that is writing, ergo assignment
 * BUT -- relative offsets will apply to assignment
 *
 * int n is the maximum amount of inputs to read
 * int m is the maximum amount of outputs to store
 * amp->inoff is changed to 1 once the first input has been written, makes day 7 easier that way
 * but take care of it if you're passing new values in as it will read from amp->in[1] for future runs if not reset
 */
int64_t eval(Comp *amp, int ninp, int nout)
{
    int modes[3];
    int64_t ret = UNKNOWN;

    while (amp->ip < amp->maxmem)
    {
        findmodes((int) amp->inst[amp->ip], modes);
        switch(amp->inst[amp->ip] % 100)
        {
            case ADD:
                amp->inst[detmode(amp, 3, modes[2])] =  amp->inst[detmode(amp, 1, modes[0])] + amp->inst[detmode(amp, 2, modes[1])];
                amp->ip += 4;
                break;
            case MUL:
                amp->inst[detmode(amp, 3, modes[2])] =  amp->inst[detmode(amp, 1, modes[0])] * amp->inst[detmode(amp, 2, modes[1])];
                amp->ip += 4;
                break;
            case WRITE:
                if (ninp && amp->inoff < BUFF)
                {
                    amp->inst[detmode(amp, 1, modes[0])] = amp->in[amp->inoff];
                    #if DEBUG
                    printf("write from input: %lld to address %lld\n", amp->in[amp->inoff], detmode(amp, 1, modes[0]));
                    #endif
                    amp->inoff++; // TAKE CARE OF THIS OFFSET
                    ninp--;
                    amp->ip += 2;
                }
                else
                {
                    #if DEBUG
                    printf("input buffer full\n");
                    #endif
                    return NEED_IN;
                }
                break;
            case READ:
                if (nout && amp->outoff < BUFF)
                {
                    amp->out[amp->outoff] = amp->inst[detmode(amp, 1, modes[0])];
                    #if DEBUG
                    printf("read from output: %lld from address %lld\n", amp->out[amp->outoff], detmode(amp, 1, modes[0]));
                    #endif
                    amp->outoff++;
                    nout--;
                    amp->ip +=2;
                }
                else 
                {
                    #if DEBUG
                    printf("output buffer full\n");
                    #endif
                    return NEED_OUT;
                }
                break;
            case JNZ:
                if (amp->inst[detmode(amp, 1, modes[0])]) {amp->ip = amp->inst[detmode(amp, 2, modes[1])];}
                else {amp->ip += 3;}
                break;
            case JEZ:
                if (!amp->inst[detmode(amp, 1, modes[0])]) {amp->ip = amp->inst[detmode(amp, 2, modes[1])];}
                else {amp->ip += 3;}
                break;
            case CMPL:
                amp->inst[detmode(amp, 3, modes[2])] = (amp->inst[detmode(amp, 1, modes[0])] < amp->inst[detmode(amp, 2, modes[1])]) ? 1 : 0;
                amp->ip += 4;
                break;
            case CMPE:
                amp->inst[detmode(amp, 3, modes[2])] = (amp->inst[detmode(amp, 1, modes[0])] == amp->inst[detmode(amp, 2, modes[1])]) ? 1 : 0;
                amp->ip += 4;
                break;
            case RELB:
                amp->base += amp->inst[detmode(amp, 1, modes[0])];
                amp->ip += 2;
                break;
            case EXIT:
                #if DEBUG
                printf("program halting\n\n");
                #endif
                amp->ip = amp->maxmem;
                return HALTED;
            default:
                #if DEBUG
                printf("unknown op %lld at address %lld, exiting\n", amp->inst[amp->ip], amp->ip);
                #endif
                amp->ip = amp->maxmem;
                return UNKNOWN;
        }
    }
    return ret;
}

/**
 * attempt to move the drone to direction dir
 */
int moveto(Comp *comp, int dir)
{
    resetbuff(comp);
    comp->in[0] = dir;
    eval(comp, 1, 1);
    return comp->out[0];
}

/**
 * depth-first search of the maze due to how movement works via the IntCode
 * as we're stuck moving one step at a time without an easy way to return to previous points
 * without moving back the way we came
 *
 * when moving through the maze we push the direction we move and then the location we moved to onto the stack if valid
 * this is so that when going backwards we can just push off the stack to get the direction we moved thus we can
 * move the opposite way back
 */
void search(Comp *comp, Stack *move, char *maze, bool *visit, int *dist, int prevdist, int x, int y, int len)
{
    int i, cur, next, ret;

    cur = y * len + x;
    if (maze[cur] == tile[Wall] || visit[cur] || sempty(move)) {return;}

    cur = pop(move);
    visit[cur] = true;
    dist[cur] = prevdist + 1;

    for (i = North; i <= East; i++)
    {
        next = (y + vec[i][1]) * len + (x + vec[i][0]);
        if (!visit[next] && x && y && x < len && y < len)
        {
            ret = moveto(comp, i);
            maze[next] = tile[ret];
            if (ret)
            {
                push(move, i);
                push(move, next);
                search(comp, move, maze, visit, dist, dist[cur],
                        x + vec[i][0], y + vec[i][1], len);
            }
            else {visit[next] = true;}
        }
    }

    if (sempty(move)) {return;}

    // move back the way it came
    next = pop(move);
    moveto(comp, inv[next]);
    search(comp, move, maze, visit, dist, dist[cur],
            x + vec[inv[next]][0], y + vec[inv[next]][1], len);
}

/**
 * variant breadth-first search over the completed maze to build the
 * map of distances from starting point x, y too all other points in the maze
 */
void bfs(char *maze, Queue *que, bool *visit, int *dist, int len)
{
    int i, cur, cx, cy, next, nx, ny;

    while (!qempty(que))
    {
        cx = deq(que);
        cy = deq(que);
        cur = cy * len + cx;
        visit[cur] = true;

        for (i = North; i <= East; i++)
        {
            nx = cx + vec[i][0];
            ny = cy + vec[i][1];
            next = ny * len + nx;
            if (maze[next] != tile[Wall] && !visit[next]
                && cx && cy && cx < len && cy < len)
            {
                enq(que, nx);
                enq(que, ny);
                dist[next] = dist[cur] + 1;
            }
        }
    }
}

int main()
{
    const int64_t memsize = 10000;
    int64_t *op, opsize, msize, i, x, y, ret;
    int *dist;
    char *input, *parse, *maze;
    bool fnd, *visit;
    Comp *comp;
    Stack *move;
    Queue *que;

    comp = malloc(sizeof(*comp));
    op = malloc(sizeof(*op) * LEN);
    input = malloc(sizeof(*input) * LEN); 
    if (!comp || !input || !op) {return -1;}

    comp->inst = calloc(memsize, sizeof(*comp->inst));
    if (!comp->inst) {return -2;}

    // convert list of op codes to integers
    opsize = 0;
    if (fgets(input, sizeof(*input) * LEN, stdin) != NULL)
    {
        parse = strtok(input, DELIN);
        while (parse != NULL)
        {
            op[opsize++] = atoll(parse);
            parse = strtok(NULL, DELIN);
        }
    }
    free(input);

    // part 1 - initial setup
    comp->maxmem = memsize;
    resetamp(comp, op, opsize);
    free(op);

    msize = DIM * DIM;
    maze = malloc(sizeof(*maze) * msize);
    dist = calloc(msize, sizeof(*dist));
    visit = calloc(msize, sizeof(*visit));
    move = malloc(sizeof(*move));
    if (!maze  || !dist || !visit || !move) {return -3;}

    memset(maze, ' ', sizeof(*maze) * msize);
    move->loc = 0;
    move->size = msize;
    move->val = malloc(sizeof(*move->val) * move->size);
    if (!move->val) {return -4;}

    x = y = DIM / 2;
    push(move, y * DIM + x);
    maze[y * DIM + x] = tile[Moved];

    // depth first search to build the maze
    search(comp, move, maze, visit, dist, -1, x, y, DIM);
    fnd = false;
    for (y = 0; y < DIM && !fnd; y++)
    {
        for (x = 0; x < DIM && !fnd; x++)
        {
            if (maze[y * DIM + x] == tile[Oxygen])
            {
                printf("part 1 - %d steps to oxygen tank (%lld, %lld)\n", dist[y * DIM + x], x, y);
                fnd = true;
            }
        }
    }
    free(move->val);
    free(move);
    
    // part 2 - recursive bfs using a queue
    que = malloc(sizeof(*que));
    if (!que) {return -5;}
    que->front = 0;
    que->rear = -1;
    que->size = msize * 2;
    que->val = malloc(sizeof(*que->val) * que->size);
    if (!que->val) {return -6;}

    // start from the oxygen tank
    enq(que, --x);
    enq(que, --y);
    memset(visit, 0, sizeof(*visit) * msize);
    memset(dist, 0, sizeof(*dist) * msize);
    bfs(maze, que, visit, dist, DIM);

    ret = 0;
    for (i = 0; i < msize; i++)
    {
        if (dist[i] > ret) {ret = dist[i];}
    }
    printf("part 2 - %d seconds to fill maze with oxygen\n", ret);

    free(maze);
    free(visit);
    free(dist);
    free(que->val);
    free(que);
    free(comp->inst);
    free(comp);

    return 0;
}
