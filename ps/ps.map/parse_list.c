
/***********************************************************
 * parse_val_list (buf, list)
 *   char *buf; int **list;
 *
 * buf is a comma separated list of values
 * or value ranges: 1,2,6-10,12
 *
 * actual usage is
 *   char buf[300]; DCELL *list;
 *
 *   count = parse_val_list (buf, &list);
 *
 *   for (i = 0; i < count; i += 2)
 *	{
 *          min = list[i];
 *	    max = list[i+1];
 *	}
 *
 * count will be negative if list is not valid
 ********************************************************/
#include <grass/raster.h>

int parse_val_list(char *buf, DCELL ** list)
{
    int count;
    DCELL a, b;
    DCELL *lp;

    count = 0;
    lp = (DCELL *) G_malloc(sizeof(DCELL));
    while (*buf) {
	while (*buf == ' ' || *buf == '\t' || *buf == '\n' || *buf == ',')
	    buf++;
	if (sscanf(buf, "%lf-%lf", &a, &b) == 2) {
	    if (a > b) {
		DCELL t;

		t = a;
		a = b;
		b = t;
	    }

	    lp = (DCELL *) G_realloc(lp, (count + 2) * sizeof(DCELL));
	    lp[count++] = a;
	    lp[count++] = b;
	}
	else if (sscanf(buf, "%lf", &a) == 1) {
	    lp = (DCELL *) G_realloc(lp, (count + 2) * sizeof(DCELL));
	    lp[count++] = a;
	    lp[count++] = a;
	}
	else {
	    G_free(lp);
	    return -1;
	}
	while (*buf && (*buf != ','))
	    buf++;
    }
    *list = lp;
    return count;
}
