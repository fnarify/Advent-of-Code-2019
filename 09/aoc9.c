#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#define DEBUG 0 // extra printing details

#define LEN 5000
#define DELIN ","

/**
 * need C99 standards (std=c99) for this program as you need int64_t 
 * (or long long) to be defined, also to print lld thanks to Windows' poor C standards compliance
 * you can use PRId64 for example for Windows.
 */
#define NEED_IN LLONG_MIN
#define HALTED  LLONG_MIN + 1
#define UNKNOWN LLONG_MIN + 2

enum Mode {Position = 0, Immediate = 1, Relative = 2};
enum Code {ADD = 1, MUL = 2, WRITE = 3, READ = 4, JNZ = 5, JEZ = 6, CMPL = 7, CMPE = 8, RELB = 9, EXIT = 99};

/**
 * needs to be able to store ~50 digit values, so we need to use int64_t
 */
struct Amp {
    int64_t *inst;
    int64_t ninst, ip, out, in[2], offset, base, maxmem;
};
typedef struct Amp Amp;

/**
 * resets amp program to default state
 */
void resetamp(Amp *amp, int64_t *op, int64_t opsize)
{
    memcpy(amp->inst, op, sizeof(*op) * opsize);
    amp->ip = 0;
    amp->out = 0;
    amp->base = 0;
    amp->offset = 0; // remove if not needed
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
int64_t detmode(Amp *a, int ioff, int mode)
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
 * amp->offset is changed to 1 once the first input has been written, makes day 7 easier that way
 * but take care of it if you're passing new values in as it will read from amp->in[1] for future runs if not reset
 */
int64_t eval(Amp *amp, int n)
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
                if (n)
                {
#if DEBUG
                    printf("write: %lld to address %lld\n", amp->in[amp->offset], detmode(amp, 1, modes[0]));
#endif
                    amp->inst[detmode(amp, 1, modes[0])] = amp->in[amp->offset];
                    amp->offset = 1; // TAKE CARE OF THIS OFFSET
                    n--;
                    amp->ip += 2;
                }
                else {return NEED_IN;} // error code
                break;
            case READ:
                amp->out = amp->inst[detmode(amp, 1, modes[0])];
#if DEBUG
                printf("output: %lld to address %lld\n", amp->out, detmode(amp, 1, modes[0]));
#endif
                amp->ip += 2;
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
    const int64_t memsize = 3000000;
    int64_t opsize;
    int64_t *op;
    char *input, *parse;
    Amp *comp;

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

    // part 1
    comp->maxmem = memsize;
    resetamp(comp, op, opsize);
    comp->in[0] = 1;
    eval(comp, 1);
    printf("part 1 - %lld\n", comp->out);

    // part 2
    resetamp(comp, op, opsize);
    comp->in[0] = 2;
    eval(comp, 1);
    printf("part 2 - %lld\n", comp->out);

    free(op);
    free(comp->inst);
    free(comp);

    return 0;
}
