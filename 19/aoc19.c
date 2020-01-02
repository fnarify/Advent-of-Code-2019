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

#define BUFF  50
#define DIM   1000
#define LEN   3000
#define DELIN ","

/**
 * needs to be able to store ~50 digit values, so we need to use int64_t
 */
struct Comp {
    int64_t *inst;
    int64_t ninst, ip, out[BUFF], in[BUFF], inoff, outoff, base, maxmem;
};
typedef struct Comp Comp;

// function prototypes
void    resetamp (Comp *, int64_t *, int64_t);
void    findmodes(int, int *);
int64_t detmode  (Comp *, int, int);
int64_t eval     (Comp *, int, int);

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
    amp->inoff = 0;
    amp->outoff = 0;
}

void resetbuff(Comp *amp)
{
    amp->inoff = 0;
    amp->outoff = 0;
}

/**
 * returns output of pt x,y
 * 1 if it's in the beam's view
 * 0 if not
 */
int chkpt(Comp *comp, int64_t *op, int64_t opsize, int x, int y)
{
    resetamp(comp, op, opsize);
    comp->in[0] = x;
    comp->in[1] = y;
    eval(comp, 2, 1);
    return comp->out[0];
}

/**
 * checks if a square defined by [x, x + n) by [y, y + n)
 * is completely in the beams view, ergo all pts output 1
 */
int chksq(Comp *comp, int64_t *op, int64_t opsize, int x, int y, int n)
{
    int i, j;
    for (i = y; i < y + n; i++)
    {
        for (j = x; j < x + n; j++)
        {
            if (chkpt(comp, op, opsize, j, i) == 0) {return 0;}
        }
    }
    return 1;
}

/**
 * checks that each corner in the square n x n has a valid pt in the beam's
 * view and if it does return whether all points within that bound are in the beam's view
 */
int chkbnd(Comp *c, int64_t *op, int64_t opsize, int x, int y, int n)
{
    return chkpt(c, op, opsize, x, y)
        && chkpt(c, op, opsize, x + n - 1, y)
        && chkpt(c, op, opsize, x, y + n - 1)
        && chkpt(c, op, opsize, x + n - 1, y + n - 1)
        ? chksq(c, op, opsize, x, y, n) : 0;
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

void print(char *s, int n)
{
    int i, j;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++) {putchar(s[i * n + j]);}
        putchar('\n');
    }
}

int main()
{
    const int64_t memsize = 5000;
    const int sq = 100, nsearch = 2000;
    int fnd, cx, cy;
    int64_t i, j, sum, opsize;
    int64_t *op;
    char *input, *parse;
    Comp *comp;

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
    opsize = 0;
    comp->maxmem = memsize;
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

    // part 1
    sum = 0;
    for (i = 0; i < 50; i++)
    {
        for (j = 0; j < 50; j++)
        {
            if (chkpt(comp, op, opsize, j, i) == 1) {sum++;}
        }
    }
    printf("part 1 - %lld points affected by tractor beam\n", sum);

    // part 2 - reduce problem to a 100 x 100 space by finding the earliest instance
    // of a fillable square
    fnd = cx = cy = 0;
    for (i = 0; i < nsearch && !fnd; i += sq)
    {
        for (j = 0; j < nsearch && !fnd; j += sq)
        {
            if (chkbnd(comp, op, opsize, i, j, sq))
            {
                cx = i;
                cy = j;
                fnd = 1;
            }
        }
    }

    fnd = 0;
    cx -= 2 * sq;
    cy -= 2 * sq;
    for (i = cx; i < cx + sq && !fnd; i++)
    {
        for (j = cy; j < cy + sq && !fnd; j++)
        {
            if (chkbnd(comp, op, opsize, i, j, sq))
            {
                printf("part 2 - %d\n", i * 10000 + j);
                fnd = 1;
            }
        }
    }

    free(op);
    free(comp->inst);
    free(comp);

    return 0;
}

