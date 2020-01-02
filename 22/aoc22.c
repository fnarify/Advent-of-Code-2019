#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <gmp.h>

#define MAX_LINE 40
#define AT_FIND  2020            // 3036 for part 1
#define N_CARD   119315717514047 // 10007 for part 1
#define N_SHUF   101741582076661 // 1 for part 1

/**
 * you need to install and use the GNU GMP library for big nums to solve this problem
 * need to use gmp big nums as otherwise the values overflow when calculating the power modulus
 * so build with:
 * gcc -std=c99 in.c -lgmp
 * at minimum
 *
 * it may be possible otherwise to use GCC 128-bit integers if you're compiling with a 64-bit system
 * but the support is wonky at times, and minGW for Windows is by default 32-bit
 *
 * the whole operation on the decks are linear functions, and can be represented using a linear equation
 * since mod is linear
 *
 * represent as y = mx + b
 * which we represent as a matrix
 */
int main()
{
    int val;
    char line[MAX_LINE];
    mpz_t ncard, ncard_2, nshuf, offd, incm, incshuf, offshuf, temp, one, index;

    // all variables have to be initialised before they can be set
    mpz_inits(ncard, ncard_2, nshuf, offd, incm, incshuf, offshuf, temp, one, index, (mpz_ptr) NULL);

    mpz_set_ui(one, 1);
    mpz_set_ui(incm, 1);
    mpz_set_ui(index, AT_FIND);
    mpz_set_ui(ncard, N_CARD);
    mpz_set_ui(ncard_2, N_CARD - 2);
    mpz_set_ui(nshuf, N_SHUF);

    while (fgets(line, sizeof(line), stdin))
    {
        if (!strcmp(line, "deal into new stack\n")) // reverse
        {
            // offd = (offd + incm) % ncard;
            mpz_mul_si(incm, incm, -1);           // incm *= -1
            mpz_mod(incm, incm, ncard);           // incm = (incm * -1) % ncard
            mpz_add(offd, offd, incm);            // offd += incm
            mpz_mod(offd, offd, ncard);           // offd %= ncard
        }
        else if (sscanf(line, "deal with increment %d\n", &val)) // offset modulo insert
        {
            // incm = [incm * powm(val, ncard - 2, ncard)] % ncard
            mpz_set_si(temp, val);                // temp = val
            mpz_powm(temp, temp, ncard_2, ncard); // temp = temp^(ncard - 2) % ncard
            mpz_mul(incm, incm, temp);            // incm *= temp
            mpz_mod(incm, incm, ncard);           // incm %= ncard
        }
        else if (sscanf(line, "cut %d\n", &val)) // offset rearrange
        {
            // offd = (offd + val * incm) % ncard
            mpz_set_si(temp, val);                // temp = val
            mpz_mul(temp, temp, incm);            // temp *= incm
            mpz_add(offd, offd, temp);            // offd += temp
            mpz_mod(offd, offd, ncard);           // offd %= ncard
        }
    }

    // incshuf = incm^nshuf % ncard
    mpz_powm(incshuf, incm, nshuf, ncard);       // incshuf = incm^nshuf % ncard

    // offshuf = offd * (1 - incshuf) * powm([1 - incm] % ncard, ncard - 2, ncard)
    mpz_sub(temp, one, incshuf);                  // temp = 1 - incshuf
    mpz_mul(offshuf, offd, temp);                 // offshuf = offd * temp
    mpz_sub(temp, one, incm);                     // temp = 1 - incm
    mpz_mod(temp, temp, ncard);                   // temp = temp % ncard
    mpz_powm(temp, temp, ncard_2, ncard);         // temp = temp^(ncard - 2) % ncard
    mpz_mul(offshuf, offshuf, temp);              // offshuf *= offshuf * temp

    // answer = (offshuf % ncard + 2020 * incshuf) % ncard
    mpz_mod(offshuf, offshuf, ncard);             // offshuf = offshuf % ncard
    mpz_mul(index, index, incshuf);               // index = index (2020) * incshuf
    mpz_add(offshuf, offshuf, index);             // offshuf = offshuf + index
    mpz_mod(offshuf, offshuf, ncard);             // offshuf %= ncard

    // 70618172909245
    gmp_printf("part 2 - value at index 2020 is %Zd\n", offshuf);

    return 0;
}
