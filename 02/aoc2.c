#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define LEN 1000
#define DELIN ","

// parse instructions/opcodes
int run(int *in, int s)
{
    int i = 0;
    if (!in) {return -1;}
    
    while (i < s && s - i > 3)
    {
        switch(in[i])
        {
            case 1: // addition
                in[ in[i + 3] ] = in[ in[i + 1] ] + in[ in[i + 2] ];
                break;
            case 2: // multiplication
                in[ in[i + 3] ] = in[ in[i + 1] ] * in[ in[i + 2] ];
                break;
            case 99: // exit
                i = s;
                break;
            default:
                printf("unknown op at pos=%d of val=%d, exiting\n", i, in[i]);
                i = s;
                break;
        }
        i += 4;
    }
    return in[0];
}

int main()
{
    int opsize, noun, verb;
    char *input, *parse;
    int *op, *ocopy;
    
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

    ocopy = malloc(sizeof(*ocopy) * opsize);
    if (!ocopy) {return -2;}
    memcpy(ocopy, op, opsize * sizeof(*op));

    // part 1
    op[1] = 12;
    op[2] = 2;
    printf("part 1: op[0]=%d\n", run(op, opsize));

    // part 2
    // can be sped up by noting that n,v == v,n due to linearity
    for (noun = 1; noun < 100; noun++)
    {
        for (verb = 1; verb < 100; verb++)
        {
            memcpy(op, ocopy, opsize * sizeof(*ocopy));
            op[1] = noun;
            op[2] = verb;

            if (run(op, opsize) == 19690720)
            {
                printf("part 2: found at %d, %d -> %d\n", noun, verb, 100 * noun + verb);
                noun = verb = 100;
            }
        }
    }

    free(op);
    free(ocopy);
    return 0;
}
