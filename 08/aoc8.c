#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define M_LEN 20000
#define M_LAYERS 300
#define WT 25
#define HT 6

enum Colour {Black = '0', White = '1', Transparent = '2'};

int main()
{
    int i, j, nlay, min, cnt, ones, twos, index;
    char image[WT * HT];
    char *line, *parse;
    char *layers[M_LAYERS];
    
    line = malloc(sizeof(*line) * M_LEN);
    if (!line) {return -1;}

    for (i = 0; i < M_LAYERS; i++)
    {
        layers[i] = malloc(sizeof(*layers[i]) * (WT * HT + 1));
        if (!layers[i]) {return -2;}
    }

    if (fgets(line, sizeof(*line) * M_LEN, stdin))
    {
        parse = line;
        i = nlay = 0;
        while (*parse != '\0' && *parse != '\n' && nlay < M_LAYERS)
        {
            memcpy(layers[nlay], parse, sizeof(*parse) * WT * HT);
            layers[nlay][WT * HT] = '\0';
            nlay++;
            parse += WT * HT;
        }
        parse = NULL;
        free(line);

        // find layer with least 0's
        index = -1;
        min = INT_MAX;
        for (i = 0; i < nlay; i++)
        {
            cnt = 0;
            for (j = 0; j < WT * HT; j++)
            {
                if (layers[i][j] == '0') {cnt++;}
            }
            if (cnt < min)
            {
                min = cnt;
                index = i;
            }
        }

        // find how many 0's and 1's that layer has
        ones = twos = 0;
        for (i = 0; i < WT * HT; i++)
        {
            if (layers[index][i] == '1') {ones++;}
            else if (layers[index][i] == '2') {twos++;}
        }
        printf("part 1 - %d\n\n", ones * twos);

        char temp;
        // black and white take priority over a transparent pixel in an earlier layer
        for (i = 0; i < WT * HT; i++)
        {
            image[i] = Transparent;
            for (j = 0; j < nlay; j++)
            {
                if ((image[i] = layers[j][i]) != Transparent) {break;}
            }
        }

        for (i = 0; i < HT; i++)
        {
            for (j = 0; j < WT; j++)
            {
                putchar(image[i * WT + j] == Black ? ' ' : '*');
            }
            putchar('\n');
        }
    }

    free(line);
    for (i = 0; i < WT * HT; i++) {free(layers[i]);}

    return 0;
}
