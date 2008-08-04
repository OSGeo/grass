#include "flag.h"

int flag_clear_all(FLAG * flags)
{
    register int r, c;

    r = flags->nrows - 1;
    while (r--) {
	c = flags->leng - 1;
	while (c--)
	    flags->array[r][c] = 0;
    }

    return 0;
}
