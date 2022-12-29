#include <stdlib.h>
#include <grass/gis.h>

#include "flag.h"

void FlagClearAll(FLAG * flags)
{
    register int r, c;

    for (r = 0; r < flags->nrows; r++) {
	for (c = 0; c < flags->leng; c++) {
	    flags->array[r][c] = 0;
	}
    }
}


FLAG *FlagCreate(int nrows, int ncols)
{
    unsigned char *temp;
    FLAG *new_flag;
    register int i;

    new_flag = (FLAG *) G_malloc(sizeof(FLAG));

    new_flag->nrows = nrows;
    new_flag->ncols = ncols;
    new_flag->leng = (ncols + 7) / 8;
    new_flag->array =
	(unsigned char **)G_malloc(nrows * sizeof(unsigned char *));

    temp =
	(unsigned char *)G_calloc(nrows * new_flag->leng,
				  sizeof(unsigned char));

    for (i = 0; i < nrows; i++) {
	new_flag->array[i] = temp;
	temp += new_flag->leng;
    }

    return (new_flag);
}


void FlagDestroy(FLAG * flags)
{
    G_free(flags->array[0]);
    G_free(flags->array);
    G_free(flags);
}


int FlagGet(FLAG * flags, int row, int col)
{
    return (flags->array[row][col >> 3] & (1 << (col & 7)));
}


void FlagSet(FLAG * flags, int row, int col)
{
    flags->array[row][col >> 3] |= (1 << (col & 7));
}


void FlagUnset(FLAG * flags, int row, int col)
{
    flags->array[row][col >> 3] &= ~(1 << (col & 7));
}
