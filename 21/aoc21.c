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

int main()
{
    const int64_t memsize = 15000;
    int64_t *op, opsize, i;
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

    /**
     * Part 1
     * Commands:
     * AND X Y - set Y to true if both X and Y are true, otherwise set Y to false
     * OR X Y - set Y to true is X or Y is true, otherwise set to false
     * NOT X Y - set Y to true is X is false, otherwise set to false
     *
     * Y must be a writeable register J or T
     * T is a temp register, J is the jump register
     * (ie, the robot will jump if it's true at program end)
     *
     * X can be any register A, B, C or D
     * A - D are true if there is ground 1 - 4 spots away,
     * otherwise they'll be false if there's no ground
     *
     * at most 15 commands + WALK command
     * just done by hand guess and run, see where it fails
     */
    char com[] = {
        "OR A T\n"  // gr1 (ground 1 spot away) set T to true
        "AND B T\n" // gr1 and gr2 set T true
        "AND C T\n" // gr1, gr2 and gr3 set T true
        "NOT T J\n" // negate T - jump if there's no ground 3 spots infront
        "AND D J\n" // jump if there's no ground 3 spots ahead but there's ground at gr4 (4 spots ahead)
        "WALK\n"
    };

    resetamp(comp, op, opsize);
    // remove ~20-line initial output

    for (i = 0; i < sizeof(com); i++) {comp->in[i] = com[i];}
    eval(comp, sizeof(com), BUFF);
    for (i = 0; i < comp->outoff; i++)
    {
        if (comp->out[i] > 255) {printf("part 1 - %lld hull damage\n", comp->out[i]);}
    }

    /**
     * part 2
     * even more registers added
     * E, F, G, H and I to indicate if there's ground 5-9 spots away
     */
    resetamp(comp, op, opsize);
    char extcom[] = {
        "OR A T\n"
        "AND B T\n"
        "AND C T\n"
        "NOT T J\n"
        "AND D J\n"
        "NOT J T\n"
        "OR E T\n"
        "OR H T\n"
        "AND T J\n"
        "RUN\n"
    };
    for (i = 0; i < sizeof(extcom); i++) {comp->in[i] = extcom[i];}
    eval(comp, sizeof(extcom), BUFF);
    for (i = 0; i < comp->outoff; i++)
    {
        if (comp->out[i] > 255) {printf("part 2 - %lld hull damage\n", comp->out[i]);}
    }

    free(op);
    free(comp->inst);
    free(comp);

    return 0;
}
