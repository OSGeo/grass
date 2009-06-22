#include <grass/gis.h>
#include <grass/raster.h>
/* build index of cell values from list */
/* -1 means not found in list */

int lookup_class(CELL * cats,	/* input: category numbers to lookup */
		 int ncats,	/* input: number of categories to translate */
		 CELL * list,	/* input: list of known categories (sorted) */
		 int nlist,	/* input: "size" of the list - number of cats in list. */
		 CELL * class	/* output: resultant class - where each cat is found in list */
    )
{
    int left, right, cur;
    CELL c;

    while (ncats-- > 0) {
	c = *cats++;		/* extract the category */
	if (Rast_is_c_null_value(&c)) {
	    *class++ = -1;
	    continue;
	}
	left = 0;
	right = nlist - 1;
	for (;;) {
	    cur = (left + right) / 2;
	    if (c < list[cur])
		right = cur - 1;
	    else
		left = cur + 1;
	    if (c == list[cur]) {
		*class++ = cur;
		break;
	    }
	    else if (left > right) {
		*class++ = -1;	/* this should never happen */
		break;
	    }
	}
    }

    return 0;
}
