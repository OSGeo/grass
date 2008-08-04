
/*-
 * G_clicker()
 * 
 * Print a clock hand (one of '|', '/', '-', '\') to stderr.
 * Used in place of G_percent for unknown number of iterations
 * 
 */
#include <stdio.h>

static int G_clicker_prev = 0;

int G_clicker(void)
{
    int x;
    static char clicks[] = "|/-\\";

    if (G_clicker_prev == -1 || G_clicker_prev == 3)
	x = 0;

    else
	x = G_clicker_prev + 1;

    fprintf(stderr, "%1c\b", clicks[x]);
    fflush(stderr);
    G_clicker_prev = x;

    return 0;
}
