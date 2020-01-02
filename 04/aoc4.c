#include <stdio.h>
#include <stdlib.h>

#define DIGITS 6

int main()
{
    int i, min, max, val, dubs, cnt, rcnt, rdubs;
    char pass[DIGITS + 1] = {0};
    min = 197487;
    max = 673251;

    cnt = rcnt = 0;
    for (val = min; val <= max; val++)
    {
        itoa(val, pass, 10);
        
        // increasing sequence and must have at least two adjacent digits be the same
        dubs = rdubs = 0;
        for (i = 0; i < DIGITS - 1 && pass[i] <= pass[i + 1]; i++)
        {
            if (pass[i] == pass[i + 1]) {dubs = 1;}

            // part 2: valid passwords cannot have more than 2 adjacent digits be the same
            // null byte for i + 2 at upper bound
            if (i == 0 && pass[i] == pass[i + 1] && pass[i] != pass[i + 2]) {rdubs = 1;}
            else if (pass[i] == pass[i + 1] && pass[i] != pass[i - 1] && pass[i] != pass[i + 2]) {rdubs = 1;}
        }
        if (i == DIGITS - 1)
        {
            if (dubs) {cnt++;}
            if (rdubs) {rcnt++;}
        }
    }
    printf("%d valid passwords\n%d valid passwords with ONLY 2 identical adjacent digits", cnt, rcnt);

    return 0;
}
