# Advent of Code 2019 #


All done in C.

Each program takes the input file via redirection of stdin a.exe < "input.txt", except for day 25 which uses fopen, as stdin is needed to actually play it.


### Compilation: ###
Every odd day after, and including, day 7 requires the flag -std=c99 as we need support for 64-bit integers (int64_t) which requires C99 standards compliance

Day 22 requires the GMP (https://gmplib.org/) library as it needs big numbers (greater than 64-bit precision), which means it should be compiled with the -lgmp flag appended as such "gcc aoc22.c -lgmp"


### Unfinished: ###
* Day 18 is unfinished.
* Day 20 pt2 is pseudo-working but doesn't provide the correct answer.
* Day 24 pt2 is the same as above.
