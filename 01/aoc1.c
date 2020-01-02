#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_LINE 20

int main()
{
    int sumf = 0, redf = 0, temp;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), stdin))
    {
        temp = atoi(line) / 3 - 2;
        sumf += temp;

        // reduce fuel
        while (temp > 0)
        {
            redf += temp;
            temp = temp / 3 - 2;
        }
    }

    printf("sum fuel = %d; reduced fuel = %d\n", sumf, redf);

    return 0;
}
