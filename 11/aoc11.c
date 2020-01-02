#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#define DEBUG 0 // extra printing details

/**
 * need C99 standards (std=c99) for this program as you need int64_t 
 * (or long long) to be defined, also to print lld thanks to Windows' poor C standards compliance
 * you can use PRId64 for example for Windows.
 */
#define NEED_IN  LLONG_MIN
#define HALTED   LLONG_MIN + 1
#define UNKNOWN  LLONG_MIN + 2
#define NEED_OUT LLONG_MIN + 3

#define M_DIM 130
#define LEN   5000
#define DELIN ","

#define BLACK '.'
#define WHITE '#'
#define LEFT  '<'
#define RIGHT '>'
#define UP    '^'
#define DOWN  'v'

/**
 * needs to be able to store ~50 digit values, so we need to use int64_t
 */
struct Comp {
    int64_t *inst;
    int64_t ninst, ip, out[5], in[5], inoff, outoff, base, maxmem;
};
typedef struct Comp Comp;

struct Robot {
    char *map; // NOT null-terminated
    char facing;
    int (*visited)[2];
    int x, y, n, mapsize, nvisit;
};
typedef struct Robot Robot;

// function prototypes
void    resetamp (Comp *, int64_t *, int64_t);
void    findmodes(int, int *);
int64_t detmode  (Comp *, int, int);
int64_t eval     (Comp *, int, int);
void    paint    (Comp *, Robot *);
void    paintmap (Robot *);

char value[] = {BLACK, WHITE};
int colour[] = {[BLACK] = 0, [WHITE] = 1};
// direction to face based on 0 (turn left) or 1 (turn right) output
char direction[][2] = {
    [LEFT]  = {DOWN, UP},
    [RIGHT] = {UP, DOWN},
    [UP]    = {LEFT, RIGHT},
    [DOWN]  = {RIGHT, LEFT}
};

enum Mode {Position = 0, Immediate = 1, Relative = 2};
enum Code {ADD = 1, MUL = 2, WRITE = 3, READ = 4, JNZ = 5, JEZ = 6, CMPL = 7, CMPE = 8, RELB = 9, EXIT = 99};

/**
 * resets amp program to default state
 */
void resetamp(Comp *amp, int64_t *op, int64_t opsize)
{
    memcpy(amp->inst, op, sizeof(*op) * opsize);
    amp->ip = 0;
    amp->base = 0;
    // remove if not needed
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
 * a is never greater than 5 digits right now, so an int is fine
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
    int64_t ret = LLONG_MIN;

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
                if (ninp)
                {
#if DEBUG
                    printf("write: %lld to address %lld\n", amp->in[amp->inoff], detmode(amp, 1, modes[0]));
#endif
                    amp->inst[detmode(amp, 1, modes[0])] = amp->in[amp->inoff];
                    amp->inoff++; // TAKE CARE OF THIS OFFSET
                    ninp--;
                    amp->ip += 2;
                }
                else {return NEED_IN;} // error code
                break;
            case READ:
                if (nout)
                {
                    amp->out[amp->outoff] = amp->inst[detmode(amp, 1, modes[0])];
#if DEBUG
                    printf("output: %lld to address %lld\n", amp->out, detmode(amp, 1, modes[0]));
#endif
                    amp->outoff++;
                    nout--;
                    amp->ip +=2;
                }
                else {return NEED_OUT;}
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
                ret = HALTED; // error code
                break;
            default:
#if DEBUG
                printf("unknown op %lld at address %lld, exiting\n", amp->inst[amp->ip], amp->ip);
#endif
                amp->ip = amp->maxmem;
                ret = UNKNOWN; // error code
                break;
        }
    }
    return ret;
}

/**
 * print robot's map as it's not null-terminated...
 * messy way to print it without as much space
 */
void printmap(Robot *rob)
{
    int i, j, startx, starty, endx, endy;
    startx = starty = rob->n;
    endx = endy = 0;
    for (i = 0; i < rob->n; i++)
    {
        for (j = 0; j < rob->n; j++)
        {
            if (rob->map[i * rob->n + j] == WHITE)
            {
                if (i < starty) {starty = i;}
                if (i > endy)   {endy = i;}
                if (j < startx) {startx = j;}
                if (j > endx)   {endx = j;}
            }
        }
    }
    for (i = starty; i <= endy; i++)
    {
        for (j = startx; j <= endx; j++) {putchar(rob->map[i * rob->n + j] == BLACK ? ' ' : WHITE);}
        putchar('\n');
    }
}

/**
 * paint internal map of rob via the opcode program built into comp
 * records amount of spaces it has painted just once
 */
void paint(Comp *comp, Robot *rob)
{
    int i, contain;

    rob->nvisit = 0;
    rob->facing = UP;

    // set initial colour at origin (x, y)
    comp->in[0] = colour[(int) rob->map[rob->y * rob->n + rob->x]];
    while (eval(comp, 1, 2) == NEED_IN)
    {
        rob->map[rob->y * rob->n + rob->x] = value[comp->out[0]];
        rob->facing = direction[(int) rob->facing][comp->out[1]];

        switch (rob->facing)
        {
            case LEFT:
                rob->x--;
                break;
            case RIGHT:
                rob->x++;
                break;
            case UP:
                rob->y--;
                break;
            case DOWN:
                rob->y++;
                break;
            default:
                printf("unknown direction\n");
                break;
        }

        contain = 0;
        for (i = 0; i < rob->nvisit; i++)
        {
            if (rob->x == rob->visited[i][0] && rob->y == rob->visited[i][1]) {contain = 1;}
        }
        if (!contain)
        {
            rob->visited[rob->nvisit][0] = rob->x;
            rob->visited[rob->nvisit][1] = rob->y;
            rob->nvisit++;
        }

        // reset inputs/outputs for next run
        comp->in[0] = colour[(int) rob->map[rob->y * rob->n + rob->x]];
        comp->inoff = 0;
        comp->outoff = 0;
    }
}

int main()
{
    const int64_t memsize = 50000;
    int64_t opsize;
    int64_t *op;
    char *input, *parse;
    Comp *comp;
    Robot *robot;

    comp = malloc(sizeof(*comp));
    op = malloc(sizeof(*op) * LEN);
    input = malloc(sizeof(*input) * LEN); 
    if (!comp || !input || !op)
    {
        printf("not enough memory\n");
        return -1;
    }

    comp->inst = calloc(memsize, sizeof(*comp->inst));
    if (!comp->inst)
    {
        printf("not enough memory\n");
        return -2;
    }

    // convert list of op codes to integers
    if (fgets(input, sizeof(*input) * LEN, stdin) != NULL)
    {
        opsize = 0;
        parse = strtok(input, DELIN);
        while (parse != NULL)
        {
            op[opsize++] = atoll(parse);
            parse = strtok(NULL, DELIN);
        }
    }
    free(input);

    // init robot
    robot = malloc(sizeof(*robot));
    if (!robot) {return -3;}
    robot->n = M_DIM;
    robot->mapsize = M_DIM * M_DIM;
    robot->map = malloc(sizeof(*robot->map) * robot->mapsize);
    robot->visited = malloc(sizeof(*robot->visited) * robot->mapsize);
    if (!robot->map || !robot->visited) {return -4;}
    memset(robot->map, '.', sizeof(*robot->map) * robot->mapsize);

    comp->maxmem = memsize;
    resetamp(comp, op, opsize);

    // part 1 - set initial co-ords
    robot->x = robot->y = robot->n / 2;
    paint(comp, robot);
    printf("part 1 - %d panels painted just once\n", robot->nvisit);
    printmap(robot);

    // part 2
    resetamp(comp, op, opsize);
    memset(robot->map, BLACK, sizeof(*robot->map) * robot->mapsize);
    robot->nvisit = 0;
    // starting position now white
    robot->x = robot->y = robot->n / 2;
    robot->map[robot->y * robot->n + robot->x] = WHITE;
    paint(comp, robot);
    printmap(robot);

    free(comp->inst);
    free(comp);
    free(robot->map);
    free(robot->visited);
    free(robot);
    free(op);

    return 0;
}
