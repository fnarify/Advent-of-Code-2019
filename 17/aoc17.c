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

#define BUFF  1000
#define DIM   60
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

// function prototypes
void    resetamp (Comp *, int64_t *, int64_t);
void    findmodes(int, int *);
int64_t detmode  (Comp *, int, int);
int64_t eval     (Comp *, int, int);

enum Mode {Position = 0, Immediate = 1, Relative = 2};
enum Code {ADD = 1, MUL = 2, WRITE = 3, READ = 4, JNZ = 5, JEZ = 6, CMPL = 7, CMPE = 8, RELB = 9, EXIT = 99};
enum Move {Scaff = '#', Open = '.'};

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
 * checks if the output we received has a non-ascii value
 * while also resetting the output offset
 */
void clearoutput(Comp *amp)
{
    while (amp->outoff > 0 && amp->out[--amp->outoff] < 255) {;}
    if (amp->outoff > 0)
    {
        printf("part 2 - %lld dust collected\n", amp->out[amp->outoff]);
        amp->outoff = 0;
    }
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
    const int64_t memsize = 50000;
    int64_t i, j, w, h, n, ret, opsize, vsize;
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

    vsize = DIM * DIM;
    view = malloc(sizeof(*view) * vsize);
    if (!view) {return -3;}

    // initial setup
    comp->maxmem = memsize;
    resetamp(comp, op, opsize);

    // part 1
    n = w = h = 0;
    while (1)
    {
        ret = eval(comp, 0, 20);

        for (i = 0; i < comp->outoff; i++)
        {
            view[n++] = comp->out[i];
            if (comp->out[i] == '\n')
            {
                h++;
                if (!w) {w = n;}
            }
        }
        resetbuff(comp);

        if (ret == HALTED) {break;}
    }
    view[n - 1] = '\0';

    ret = 0;
    for (i = 1; i < h - 1; i++)
    {
        for (j = 1; j < w - 1; j++)
        {
            // check that all adjacent tiles are Scaffolding including the current one
            if (view[i * w + j] == Scaff && view[i * w + j - 1] == Scaff && view[i * w + j + 1] == Scaff
                && view[(i - 1) * w + j] == Scaff && view[(i + 1) * w + j] == Scaff)
            {
                ret += j * i;
            }
        }
    }
    printf("part 1 - sum of intersections %lld\n", ret);

    /**
     * part 2 - movement pattern
     * done by hand, since it's leagues easier than the solution for the general case
     * or disassembling the intcode by hand
     *
     * Movement:
     * R10, R8, L10, L10, R8, L6, L6, R8, L6, L6, R10, R8, L10, L10, L10, R10, L6,
     * R8, L6, L6, L10, R10, L6, L10, R10, L6, R8, L6, L6, R10, R8, L10, L10
     *
     * Substrings:
     * A, B, B, A, C, B, C, C, B, A 
     *  A - R10, R8, L10, L10
     *  B - R8, L6, L6
     *  C - L10, R10, L6
     *
     *  all that is passed as input is the movement function, followed by function A, B & C
     *  and then y or n for whether to display visual feedback
     */
    char moves[] = {
        "A,B,B,A,C,B,C,C,B,A\n" // movement func
        "R,10,R,8,L,10,L,10\n"  // A
        "R,8,L,6,L,6\n"         // B
        "L,10,R,10,L,6\n"       // C
        "n\n"                   // video
    };

    resetamp(comp, op, opsize);
    comp->inst[0] = 2;

    for (i = 0; i < sizeof(moves); i++) {comp->in[i] = moves[i];}

    while (eval(comp, sizeof(moves), BUFF) == NEED_OUT) {clearoutput(comp);}
    clearoutput(comp);

    free(op);
    free(view);
    free(comp->inst);
    free(comp);

    return 0;
}
