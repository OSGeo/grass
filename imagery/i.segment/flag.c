#include <grass/gis.h>
#include "flag.h"

int flag_clear_all(FLAG * flags)
{
    register int r, c;

    for (r = 0; r < flags->nrows; r++) {
	for (c = 0; c < flags->leng; c++) {
	    flags->array[r][c] = 0;
	}
    }

    return 0;
}

FLAG *flag_create(int nrows, int ncols)
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
	(unsigned char *)G_malloc(nrows * new_flag->leng *
				  sizeof(unsigned char));

    for (i = 0; i < nrows; i++) {
	new_flag->array[i] = temp;
	temp += new_flag->leng;
    }

    return (new_flag);
}

int flag_destroy(FLAG * flags)
{
    G_free(flags->array[0]);
    G_free(flags->array);
    G_free(flags);

    return 0;
}
