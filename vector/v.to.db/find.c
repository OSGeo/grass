#include <stdlib.h>
#include <string.h>
#include  "global.h"

/* returns index to array of values, inserts new if necessary */
int find_cat(int cat, int add)
{
    int i;

    for (i = 0; i < vstat.rcat; i++)
	if (Values[i].cat == cat)
	    return i;

    if (!add)
	return -1;

    /* Not found -> add new */
    Values[vstat.rcat].cat = cat;
    Values[vstat.rcat].count1 = 0;
    Values[vstat.rcat].count1 = 0;
    Values[vstat.rcat].i1 = -1;
    Values[vstat.rcat].i2 = -1;
    Values[vstat.rcat].d1 = 0.0;
    Values[vstat.rcat].d2 = 0.0;
    Values[vstat.rcat].qcat = NULL;
    Values[vstat.rcat].nqcats = 0;
    Values[vstat.rcat].aqcats = 0;
    vstat.rcat++;

    return (vstat.rcat - 1);
}
