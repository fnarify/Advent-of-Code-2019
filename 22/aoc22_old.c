#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define N_CARDS 10007
#define MAX_LINE 40

struct List {
    struct List *next;
    int val;
};
typedef struct List List;

enum Action {Deal = 0, Cut = 1, Reverse = 2};

/**
 * it actually overflows with 64-bit unsigned integers... and we get a perfect 0 in the output
 * which messes everything else up
 */
uint64_t pow_m(uint64_t a, uint64_t b, uint64_t mod)
{
    uint64_t ret = 1;
    while (b > 0)
    {
        if (b % 2)
        {
            ret = (ret * a) % mod;
        }
        a = (a * a) % mod;
        b /= 2;
    }
    return ret;
}

/**
 * reverses the list if n is 0
 * else if n > 0, offset each value in the list by n, wrapping around
 * ie, deal(list, vals, 3) will output 0 7 4 1 8 5 2 9 6 3 for a 10 element unchanged list
 */
List *deal(List *list, int *vals, int n)
{
    int i;
    List *next, *prev, *ret, *cur;

    ret = list;
    if (!n)
    {
        prev = NULL;
        while (list != NULL)
        {
            next = list->next;
            list->next = prev;
            prev = list;
            list = next;
        }
        ret = prev;
    }
    else if (n > 0)
    {
        // fill val array in order to rearrange cards
        // ie elements are added at n intervals modulo size
        // N_CARDS is prime so it'll work correctly for any n
        cur = list;
        i = 0;
        while (cur != NULL)
        {
            vals[i] = cur->val;
            cur = cur->next;
            i = (i + n) % N_CARDS;
        }

        cur = list;
        i = 0;
        while (cur != NULL)
        {
            cur->val = vals[i++];
            cur = cur->next;
        }
    }
    return ret;
}

/**
 * move the first n entries in the list to the end of the list retaining order
 * if n is negative then we move the last abs(n) entries to the start of the list
 * retaining order
 */
List *cut(List *list, int n)
{
    List *init, *cur, *ret;
    init = ret = list;
    if (n < 0) {n += N_CARDS;}
    if (n)
    {
        while (list != NULL && n > 1)
        {
            list = list->next;
            n--;
        }
        cur = list->next;
        ret = cur;
        list->next = NULL;
        while (cur->next != NULL)
        {
            cur = cur->next;
        }
        cur->next = init;
    }
    return ret;
}

int main()
{
    char line[MAX_LINE];
    int i, n, temp;
    int *vals = malloc(sizeof(*vals) * N_CARDS);
    List *cards = malloc(sizeof(*cards) * N_CARDS);
    List *cur;
    if (!cards || !vals) {return -1;}

    cur = cards;
    for (i = 0; i < N_CARDS - 1; i++)
    {
        cards[i].val = i;
        cards[i].next = &cards[i + 1];
    }
    cards[i].next = NULL;
    cards[i].val = i;

    n = temp = 0;
    while (fgets(line, sizeof(line), stdin))
    {
        if (!strcmp(line, "deal into new stack\n"))
        {
            cur = deal(cur, vals, 0);
        }
        else if (sscanf(line, "deal with increment %d\n", &temp))
        {
            cur = deal(cur, vals, temp);
        }
        else if (sscanf(line, "cut %d\n", &temp))
        {
            cur = cut(cur, temp);
        }
        n++;
    }

    i = 0;
    while (cur != NULL)
    {
        if (cur->val == 2019) {printf("part 1 - %d position of original card 2019\n", i);}
        cur = cur->next;
        i++;
    }

    free(cards);
    free(vals);

    return 0;
}
