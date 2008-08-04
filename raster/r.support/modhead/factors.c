#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include "local_proto.h"

/*
 * factors()
 *
 * RETURN: EXIT_SUCCESS
 */
int factors(FILE * fd, off_t n, int div)
{
    long m;
    int len;
    int totlen = 0;
    char buf[30];


    /* Determine number of chars in largest factor that will be printed */
    sprintf(buf, "%ld", n);
    len = strlen(buf);
    n /= div;

    /* Find the factors */
    for (m = 1;; m++) {
	if (n % m == 0) {
	    off_t x = n / m;

	    if (x < m)
		break;

	    if (sizeof(off_t) > sizeof(long) && x > ULONG_MAX)
		continue;

	    sprintf(buf, "%%%ld * %%-%ld", m, x);
	    len = strlen(buf) + 3;
	    if (totlen + len > 75) {
		fprintf(fd, "\n");
		totlen = 0;
	    }

	    fprintf(fd, "%s   ", buf);
	    totlen += len;
	}
    }

    if (totlen)
	fprintf(fd, "\n");

    return EXIT_SUCCESS;
}
