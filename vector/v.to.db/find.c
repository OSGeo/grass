#include <stdlib.h>
#include <string.h>
#include  "global.h"


static int bsearch_cat(int cat)
{
    int mid, lo, hi;
    
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

/* returns index to array of values, inserts new if necessary */
int find_cat(int cat, int add)
{
    int i;

    if ((i = bsearch_cat(cat)) >= 0)
	    return i;

    if (!add)
	return -1;

    /* Not found -> add new */
    for (i = vstat.rcat; i > 0; i--) {
	if (Values[i - 1].cat < cat)
	    break;

	Values[i] = Values[i - 1];
    }
    Values[i].cat = cat;
    Values[i].count1 = 0;
    Values[i].count1 = 0;
    Values[i].i1 = -1;
    Values[i].i2 = -1;
    Values[i].d1 = 0.0;
    Values[i].d2 = 0.0;
    Values[i].qcat = NULL;
    Values[i].nqcats = 0;
    Values[i].aqcats = 0;

    vstat.rcat++;

    return (i);
}
