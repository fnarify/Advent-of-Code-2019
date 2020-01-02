#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#define DEBUG 0

#define LEN 3000
#define DELIN ","
#define N_AMPS 5
#define N_PERMS 120

#define NEED_IN INT_MIN
#define HALTED INT_MIN + 1
#define UNKNOWN INT_MIN + 2

enum Mode {Position = 0, Immediate = 1};
enum Code {ADD = 1, MUL = 2, WRITE = 3, READ = 4, JNZ = 5, JEZ = 6, CMPL = 7, CMPE = 8, EXIT = 99};

int nset = 0;
char phaseset[N_PERMS][N_AMPS + 1];

struct Amp {
    int *inst;
    int ninst;
    int ip;
    int out;
    int in[2]; // only need at most two for this program at a time
    int offset;
};
typedef struct Amp Amp;

void swap(char *a, char *b)
{
    char temp = *b;
    *b = *a;
    *a = temp;
}

/**
 * find all permutations of string s starting at index a or size b
 */
void permutate(char *s, int a, int b)
{
    int i;
    if (a == b - 1)
    {
        // global variables
        strcpy(phaseset[nset++], s);
        return;
    }

    for (i = a; i < b; i++)
    {
        swap(s + a, s + i);
        permutate(s, a + 1, b);
        swap(s + a, s + i);
    }
}

/**
 * resets amps program to default state
 */
void resetamp(Amp *amp, int *op, int opsize)
{
    memcpy(amp->inst, op, sizeof(*op) * opsize);
    amp->ip = 0;
    amp->out = 0;
    amp->offset = 0; // remove if not needed
}

/**
 * a is of the form ABCDE where they are digits
 * DE - 2-digit opcode
 * C - 1st param mode
 * B - 2nd param mode
 * A - 3rd param mode
 * A, B, C don't always have to be present
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
 * if mode is 1 (immediate) return value of index
 * if mode is 0 (position) return value at the value of its index
 */
int detmode(int *inst, int index, int mode)
{
    return mode ? inst[index] : inst[inst[index]];
}

/**
 * handle instruction and return last output
 * mode A will never apply to an instruction that is writing, ergo assignment
 *
 * part 2 requires stopping temporarily when an output is read, so return is short-circuited
 *
 * int n is the maximum amount of inputs to read
 */
int eval(Amp *amp, int n)
{
    int modes[3];
    int ret = INT_MIN;

    while (amp->ip < amp->ninst)
    {
        findmodes(amp->inst[amp->ip], modes);
        switch(amp->inst[amp->ip] % 100)
        {
            case ADD:
                amp->inst[ amp->inst[amp->ip + 3] ] = detmode(amp->inst, amp->ip + 1, modes[0]) + detmode(amp->inst, amp->ip + 2, modes[1]);
                amp->ip += 4;
                break;
            case MUL:
                amp->inst[ amp->inst[amp->ip + 3] ] = detmode(amp->inst, amp->ip + 1, modes[0]) * detmode(amp->inst, amp->ip + 2, modes[1]);
                amp->ip += 4;
                break;
            case WRITE:
                if (n)
                {
#if DEBUG
                    printf("write: %d to address %d\n", amp->in[amp->offset], amp->inst[amp->ip + 1]);
#endif
                    amp->inst[ amp->inst[amp->ip + 1] ] = amp->in[amp->offset];
                    amp->offset = 1; // phase signal is only read once as input
                    n--;
                    amp->ip += 2;
                }
                else {return NEED_IN;} // error code
                break;
            case READ:
#if DEBUG
                printf("output: %d\n", detmode(amp->inst, amp->ip + 1, modes[0]));
#endif
                amp->out = detmode(amp->inst, amp->ip + 1, modes[0]);
                amp->ip += 2;
                break;
            case JNZ:
                if (detmode(amp->inst, amp->ip + 1, modes[0])) {amp->ip = detmode(amp->inst, amp->ip + 2, modes[1]);}
                else {amp->ip += 3;}
                break;
            case JEZ:
                if (!detmode(amp->inst, amp->ip + 1, modes[0])) {amp->ip = detmode(amp->inst, amp->ip + 2, modes[1]);}
                else {amp->ip += 3;}
                break;
            case CMPL:
                amp->inst[ amp->inst[amp->ip + 3] ] = (detmode(amp->inst, amp->ip + 1, modes[0]) < detmode(amp->inst, amp->ip + 2, modes[1])) ? 1 : 0;
                amp->ip += 4;
                break;
            case CMPE:
                amp->inst[ amp->inst[amp->ip + 3] ] = (detmode(amp->inst, amp->ip + 1, modes[0]) == detmode(amp->inst, amp->ip + 2, modes[1])) ? 1 : 0;
                amp->ip += 4;
                break;
            case EXIT:
#if DEBUG
                printf("program halting\n\n");
#endif
                amp->ip = amp->ninst;
                ret = HALTED; // error code
                break;
            default:
#if DEBUG
                printf("unknown op %d at address %d, exiting\n", amp->inst[amp->ip], amp->ip);
#endif
                amp->ip = amp->ninst;
                ret = UNKNOWN; // error code
                break;
        }
    }
    return ret;
}

int main()
{
    int i, j, opsize, output, max, state;
    int *op;
    char *input, *parse;
    char phase[] = {'0', '1', '2', '3', '4', '\0'}; // don't forget your null byte

    Amp *amps[N_AMPS];
    for (i = 0; i < N_AMPS; i++)
    {
        amps[i] = malloc(sizeof(struct Amp));
        if (!amps[i]) {return -1;}
    }

    op = malloc(sizeof(*op) * LEN);
    input = malloc(sizeof(*input) * LEN); 
    if (!input || !op) {return -1;}

    // convert list of op codes to integers
    if (fgets(input, sizeof(*input) * LEN, stdin) != NULL)
    {
        opsize = 0;
        parse = strtok(input, DELIN);
        while (parse != NULL)
        {
            op[opsize++] = atoi(parse);
            parse = strtok(NULL, DELIN);
        }
    }

    // initialise amps
    for (i = 0; i < N_AMPS; i++)
    {
        amps[i]->inst = malloc(sizeof(*amps[i]->inst) * opsize);
        if (!amps[i]->inst) {return -2;}
        resetamp(amps[i], op, opsize);
        amps[i]->ninst = opsize;
    }
    free(input);

    // part 1: build permutation list into phaseset (global)
    // the short circuit return of output doesn't affect for part 1
    permutate(phase, 0, N_AMPS);

    max = INT_MIN;
    for (i = 0; i < nset; i++)
    {
        output = 0;
#if DEBUG
        printf("Run %d:\n", i + 1);
#endif
        for (j = 0; j < N_AMPS; j++)
        {
            amps[j]->in[0] = phaseset[i][j] - '0';
            amps[j]->in[1] = output;
            if (eval(amps[j], 2) != HALTED) {printf("Error, part 1\n");}
            output = amps[j]->out;
        }

        if (output > max)
        {
            max = output;
            strcpy(phase, phaseset[i]);
        }

        for (j = 0; j < N_AMPS; j++) {resetamp(amps[j], op, opsize);}
    }
    printf("part 1 - largest signal %d at setting: %s\n", max, phase);

    // part 2: new phase settings
    strcpy(phase, "56789");
    nset = 0;
    permutate(phase, 0, N_AMPS);

    max = INT_MIN;
    for (i = 0; i < nset; i++)
    {
        // first run with phase settings
        output = 0;
        for (j = 0; j < N_AMPS; j++)
        {
            amps[j]->in[0] = phaseset[i][j] - '0';
            amps[j]->in[1] = output;
            eval(amps[j], 2);
            output = amps[j]->out;
        }

        // wait till last amplifier has halted
        state = 0;
        while (state != HALTED)
        {
            for (j = 0; j < N_AMPS; j++)
            {
                amps[j]->in[1] = output;
                state = eval(amps[j], 1);
                output = amps[j]->out;
            }
        }

        if (output > max)
        {
            max = output;
            strcpy(phase, phaseset[i]);
        }

        for (j = 0; j < N_AMPS; j++) {resetamp(amps[j], op, opsize);}
    }
    printf("part 2 - largest signal %d at setting %s\n", max, phase);

    for (i = 0; i < N_AMPS; i++)
    {
        free(amps[i]->inst);
        free(amps[i]);
    }
    free(op);

    return 0;
}
