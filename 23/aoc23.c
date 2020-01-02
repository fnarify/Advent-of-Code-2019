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

#define BUFF  60
#define LEN   10000
#define DELIN ","
#define N_COMP 50

/**
 * needs to be able to store ~50 digit values, so we need to use int64_t
 */
struct Comp {
    int64_t *inst;
    int64_t ninst, ip, out[BUFF], in[BUFF], inoff, outoff, base, maxmem;
};
typedef struct Comp Comp;

struct Queue {
    int64_t *val;
    int64_t front, rear, size;
};
typedef struct Queue Queue;

// function prototypes
void    resetamp (Comp *, int64_t *, int64_t);
void    findmodes(int, int *);
int64_t detmode  (Comp *, int, int);
int64_t eval     (Comp *, int, int);

enum Mode {Position = 0, Immediate = 1, Relative = 2};
enum Code {ADD = 1, MUL = 2, WRITE = 3, READ = 4, JNZ = 5, JEZ = 6, CMPL = 7, CMPE = 8, RELB = 9, EXIT = 99};

int qempty(Queue *q)
{
    return q->front > q->rear;
}

void enq(Queue *q, int64_t item)
{
    if (q->rear == q->size - 1)
    {
        printf("Queue is full\n");
        q->rear = -1;
        q->front = 0;
    }
    q->val[++q->rear] = item;
}

int64_t deq(Queue *q)
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

int main()
{
    int fnd;
    const int64_t memsize = 15000;
    int64_t *op, opsize, i, j, dest, x, y, ninput, nat[2], lastnat[2] = {0};
    char *input, *parse;
    Comp *comp[N_COMP];
    Queue *que[N_COMP];

    for (i = 0; i < N_COMP; i++)
    {
        comp[i] = malloc(sizeof(*comp[i]));
        que[i] = malloc(sizeof(*que[i]));
        if (!comp[i] || !que[i]) {return -1;}

        comp[i]->inst = calloc(memsize, sizeof(*comp[i]->inst));
        que[i]->size = BUFF * 10;
        que[i]->rear = -1;
        que[i]->front = 0;
        que[i]->val = malloc(sizeof(*que[i]->val) * que[i]->size); 
        if (!comp[i]->inst || !que[i]->val) {return -2;}
    }

    op = malloc(sizeof(*op) * LEN);
    input = malloc(sizeof(*input) * LEN); 
    if (!input || !op)
    {
        printf("not enough memory\n");
        return -3;
    }

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

    // initialise computers
    for (i = 0; i < N_COMP; i++)
    {
        resetamp(comp[i], op, opsize);
        comp[i]->in[0] = i; // id
        comp[i]->maxmem = memsize;
    }
    free(op);

    // set network id
    for (i = 0; i < N_COMP; i++) {eval(comp[i], 1, 0);}

    // uses a separate queue to record output addresses from the computers
    // as one there the same address can be pointed to multiple times
    // we treat the output buffer as being large enough that the programs will block on the next input
    fnd = 0;
    while (1)
    {
        for (i = 0; i < N_COMP; i++)
        {
            // this probably shouldn't work as it doesn't correctly account for
            // unused input, but then again the output buffer is large enough it does not matter
            if (qempty(que[i]) && comp[i]->inoff)
            {
                comp[i]->inoff = 0;
                comp[i]->in[0] = -1;
                ninput = 1;
            }
            else if (!qempty(que[i]) && comp[i]->inoff)
            {
                comp[i]->inoff = 0;
                comp[i]->in[0] = deq(que[i]);
                comp[i]->in[1] = deq(que[i]);
                ninput = 2;
            }
            else {ninput = (comp[i]->in[0] == -1) ? 1 : 2;}

            comp[i]->outoff = 0;
            eval(comp[i], ninput, BUFF);

            for (j = 0; j < comp[i]->outoff && comp[i]->outoff >= 3; j += 3)
            {
                dest = comp[i]->out[j];
                x = comp[i]->out[j + 1];
                y = comp[i]->out[j + 2];

                if (dest == 255)
                {
                    if (!fnd)
                    {
                        printf("part 1 - %lld first y val sent to address 255\n", y);
                        fnd = 1;
                    }
                    nat[0] = x;
                    nat[1] = y;
                }
                else
                {
                    enq(que[dest], x);
                    enq(que[dest], y);
                }
            }
        }

        // check if all computers are idle - ie, all input queues are empty
        for (i = 0; i < N_COMP && qempty(que[i]); i++) {;}
        if (i == N_COMP)
        {
            if (nat[0] == lastnat[0] && nat[1] == lastnat[1])
            {
                printf("part 2 - %lld first y val sent twice in a row by the NAT\n", nat[1]);
                break;
            }
            enq(que[0], nat[0]);
            enq(que[0], nat[1]);
            lastnat[0] = nat[0];
            lastnat[1] = nat[1];
        }
    }

    for (i = 0; i < N_COMP; i++)
    {
        free(comp[i]->inst);
        free(comp[i]);
        free(que[i]->val);
        free(que[i]);
    }

    return 0;
}
