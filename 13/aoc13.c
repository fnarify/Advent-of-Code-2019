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

#define DIM 40
#define LEN   10000
#define DELIN ","

/**
 * needs to be able to store ~50 digit values, so we need to use int64_t
 */
struct Comp {
    int64_t *inst;
    int64_t ninst, ip, out[5], in[5], inoff, outoff, base, maxmem;
};
typedef struct Comp Comp;

// function prototypes
void    resetamp (Comp *, int64_t *, int64_t);
void    findmodes(int, int *);
int64_t detmode  (Comp *, int, int);
int64_t eval     (Comp *, int, int);

enum Mode {Position = 0, Immediate = 1, Relative = 2};
enum Code {ADD = 1, MUL = 2, WRITE = 3, READ = 4, JNZ = 5, JEZ = 6, CMPL = 7, CMPE = 8, RELB = 9, EXIT = 99};
enum Tile {Empty = 0, Wall = 1, Block = 2, Paddle = 3, Ball = 4};

int map[] = {
    [Empty] = ' ',
    [Wall] = '|',
    [Block] = '*',
    [Paddle] = '_',
    [Ball] = 'o'
};

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
                    printf("output: %lld from address %lld\n", amp->out[amp->outoff], detmode(amp, 1, modes[0]));
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

int main()
{
    const int64_t memsize = 70000;
    int cnt;
    int64_t opsize, p_loc, b_loc, ret, sum;
    int64_t *op;
    char *view, *input, *parse;
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

    view = malloc(sizeof(*view) * DIM * DIM);
    if (!view) {return -3;}

    // initial setup
    comp->maxmem = memsize;
    resetamp(comp, op, opsize);
    memset(view, map[Empty], sizeof(*view) * DIM * DIM);

    // part 1 - setup game
    cnt = 0;
    while (eval(comp, 0, 3) == NEED_OUT)
    {
        view[comp->out[1] * DIM + comp->out[0]] = map[comp->out[2]];

        if      (comp->out[2] == Block)  {cnt++;}
        else if (comp->out[2] == Paddle) {p_loc = comp->out[0];}
        else if (comp->out[2] == Ball)   {b_loc = comp->out[0];}

        comp->outoff = 0;
    }
    view[comp->out[1] * DIM + comp->out[0]] = map[comp->out[2]];
    printf("part 1 - %d block tiles\n", cnt);

    // part 2 - play game via simple AI
    resetamp(comp, op, opsize);
    free(op);
    sum = 0;
    comp->inst[0] = 2;
    // not needed, but just in case you access unset memory
    comp->out[0] = comp->out[1] = comp->out[2] = 0;

    ret = eval(comp, 0, 3);
    while (1)
    {
        while (ret == NEED_OUT)
        {
            if (comp->out[2] == Ball)   {b_loc = comp->out[0];}
            if (comp->out[2] == Paddle) {p_loc = comp->out[0];}
            comp->outoff = 0;
            ret = eval(comp, 0, 3);
        }

        // exit condition
        if (comp->out[0] == -1 && comp->out[1] == 0) {sum = comp->out[2];}
        // leftover output
        if (comp->out[2] == Ball)   {b_loc = comp->out[0];}
        if (comp->out[2] == Paddle) {p_loc = comp->out[0];}

        if (ret == NEED_IN)
        {
            comp->in[0] = b_loc > p_loc ? 1 : b_loc < p_loc ? -1 : 0;
            comp->outoff = 0;
            comp->inoff = 0;
            ret = eval(comp, 1, 3);
        }
        else if (ret == HALTED) {break;}
    }
    printf("part 2 - final score %lld\n", sum);

    free(view);
    free(comp->inst);
    free(comp);

    return 0;
}
