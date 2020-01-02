#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_IN   30
#define MAX_MEM  5000
#define MAX_LINE 100
#define DELIN    ", "
#define BASE     36

int setval = 1;
int *settrue = &setval;

struct Reaction {
    int64_t react[MAX_IN]; // each reaction has 2 entries (size, name)
    int64_t out, rsize, left;
    int *set; // there's probably a better way of checking nullity, without a separate map
};
typedef struct Reaction Reaction;

int64_t getindex(char *s)
{
    return strtol(s, NULL, BASE) % MAX_MEM;
}

/**
 * determine amount of materials for item (provided as its index in re)
 * recursive function, amt is the amount of the material to make
 *
 * could also make this tail recursive without much trouble
 */
int64_t make(Reaction *re, int64_t item, int64_t amt)
{
    int i;
    int64_t sum = 0, ratio = 0;
    if (re[item].left >= amt)
    {
        re[item].left -= amt;
        sum = 0;
    }
    else
    {
        // if the amount is greater than what is output by the reaction formula then we calculate the ratio
        // of the required amount vs output, otherwise we take the formula as is and add to the leftover
        amt -= re[item].left;
        ratio = amt > re[item].out ? 1 + ((amt - 1) / re[item].out) : 1;
        re[item].left = re[item].out * ratio - amt;

        for (i = 0; i < re[item].rsize; i += 2)
        {
            if (!re[re[item].react[i + 1]].set) {sum += ratio * re[item].react[i];}
            else                                {sum += make(re, re[item].react[i + 1], ratio * re[item].react[i]);}
        }
    }
    return sum;
}

/**
 * all outputs are distinct, and all values are A-Z uppercase only with no numerals
 * however, we still need to use base 36 to properly convert them using strtol
 * but we can restrict the output size uniquely to massively save on memory
 */
int main()
{
    char line[MAX_LINE], input[MAX_LINE], output[MAX_LINE / 4];
    char *parse;
    int cnt;
    const int64_t maxore = 1000000000000;
    int64_t index, outsize, ore, low, high, mid;
    Reaction *reactions = malloc(sizeof(*reactions) * MAX_MEM);
    Reaction *origre = malloc(sizeof(*reactions) * MAX_MEM);
    if (!reactions || !origre) {return -1;}

    while (fgets(line, sizeof(line), stdin))
    {
        // mingw used %I64d for long long
        sscanf(line, "%[^=]=> %I64d %s\n", input, &outsize, output);
        input[strlen(input) - 1] = '\0'; // remove trailing space

        index = getindex(output);
        reactions[index].out = outsize;
        reactions[index].rsize = 0;
        reactions[index].set = settrue;
        reactions[index].left = 0;

        cnt = 0;
        parse = strtok(input, DELIN);
        while (parse != NULL)
        {
            // save into react in the format (input size, input [as a base 36 index])
            reactions[index].react[cnt++] = atol(parse);
            parse = strtok(NULL, DELIN);

            reactions[index].react[cnt++] = getindex(parse);
            parse = strtok(NULL, DELIN);

            // shouldn't ever be called
            if (cnt >= MAX_IN) {printf("need a longer string for react\n"); return -2;}
        }
        reactions[index].rsize = cnt;
    }

    // probably not undefined behaviour copying ununitialised variables in a struct with memcpy
    memcpy(origre, reactions, sizeof(*reactions) * MAX_MEM);

    // part 1
    index = getindex("FUEL");
    ore = make(origre, index, 1);
    printf("part 1 - 1 FUEL needs %lld ORE\n", ore);

    // part 2, using a binary search
    low = maxore / ore;
    high = maxore - 1;
    while (low <= high)
    {
        mid = (low + high) / 2;
        if (low == mid) {break;}

        memcpy(origre, reactions, sizeof(*reactions) * MAX_MEM);
        ore = make(origre, index, mid);

        if (ore < maxore) {low = mid;}
        else {high = mid;}
    }
    printf("part 2 - %lld FUEL can be made from 1 trillion ORE (%lld ORE used)\n", low, ore);

    free(reactions);
    free(origre);

    return 0;
}
