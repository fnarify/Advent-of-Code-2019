#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define LEN 5000
#define DELIN ","

enum Mode {Position = 0, Immediate = 1};
enum Code {ADD = 1, MUL = 2, WRITE = 3, READ = 4, JNZ = 5, JEZ = 6, CMPL = 7, CMPE = 8, EXIT = 99};

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
 * handle instruction and return instruction length
 * mode A will never apply to an instruction that is writing, ergo assignment
 */
void eval(int *inst, int size, int input)
{
    int ip;
    int modes[3];

    ip = 0;
    while (ip < size)
    {
        findmodes(inst[ip], modes);
        switch(inst[ip] % 100)
        {
            case ADD:
                inst[ inst[ip + 3] ] = detmode(inst, ip + 1, modes[0]) + detmode(inst, ip + 2, modes[1]);
                ip += 4;
                break;
            case MUL:
                inst[ inst[ip + 3] ] = detmode(inst, ip + 1, modes[0]) * detmode(inst, ip + 2, modes[1]);
                ip += 4;
                break;
            case WRITE:
                printf("write: %d to address %d\n", input, inst[ip + 1]);
                inst[ inst[ip + 1] ] = input;
                ip += 2;
                break;
            case READ:
                printf("output: %d\n", detmode(inst, ip + 1, modes[0]));
                ip += 2;
                break;
            case JNZ:
                if (detmode(inst, ip + 1, modes[0])) {ip = detmode(inst, ip + 2, modes[1]);}
                else {ip += 3;}
                break;
            case JEZ:
                if (!detmode(inst, ip + 1, modes[0])) {ip = detmode(inst, ip + 2, modes[1]);}
                else {ip += 3;}
                break;
            case CMPL:
                inst[ inst[ip + 3] ] = (detmode(inst, ip + 1, modes[0]) < detmode(inst, ip + 2, modes[1])) ? 1 : 0;
                ip += 4;
                break;
            case CMPE:
                inst[ inst[ip + 3] ] = (detmode(inst, ip + 1, modes[0]) == detmode(inst, ip + 2, modes[1])) ? 1 : 0;
                ip += 4;
                break;
            case EXIT:
                printf("program halting\n\n");
                ip = size;
                break;
            default:
                printf("unknown op %d at address %d, exiting\n", inst[ip], ip);
                ip = size;
                break;
        }
    }
}

int main()
{
    int opsize;
    char *input, *parse;
    int *op, *opcopy;
    
    op = malloc(sizeof(*op) * LEN);
    input = malloc(sizeof(*input) * LEN); 
    if (!input || !op) {return -1;}

    // create list of codes as integers
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
    free(input);

    opcopy = malloc(sizeof(*opcopy) * opsize);
    if (!opcopy) {return -2;}

    memcpy(opcopy, op, sizeof(*op) * opsize);
    eval(opcopy, opsize, 1); // part 1
    free(opcopy);
    eval(op, opsize, 5); // part 2

    free(op);
    return 0;
}
