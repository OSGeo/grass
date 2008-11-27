#include <grass/gis.h>
#include "flag.h"

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
