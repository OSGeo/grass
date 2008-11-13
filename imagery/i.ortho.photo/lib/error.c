#include <stdio.h>

/*
 * error: print error message and return 0
 */

FILE *Bugs2;

int error(char *s)
{
    fprintf(stderr, "%s library error routine!", s);
    fclose(Bugs2);
    return 0;
}
