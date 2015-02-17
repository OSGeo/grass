#include <stdlib.h>
#include <string.h>
#include  "global.h"


static int bsearch_cat(int cat)
{
    int mid, lo, hi;

    /* tests */
    if (vstat.rcat < 1)
	return -1;

    lo = 0;
    hi = vstat.rcat - 1;
    
    if (hi == 0 || Values[lo].cat > cat || Values[hi].cat < cat)
	return -1;

    if (Values[hi].cat == cat)
	return hi;

    if (Values[lo].cat == cat)
	return lo;

    /* bsearch */
    while (lo < hi) {
	mid = (lo + hi) / 2;
	
	if (Values[mid].cat == cat)
	    return mid;

	if (Values[mid].cat > cat) {
	    hi = mid;
	}
	else {
	    lo = mid;
	}
    }
    
    return -1;
}

/* returns index to array of values, mark as used if requested */
int find_cat(int cat, int used)
{
    int i;

    i = bsearch_cat(cat);

    if (i >= 0 && used)
	Values[i].used = 1;

    return (i);
}
