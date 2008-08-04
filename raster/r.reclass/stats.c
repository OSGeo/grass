#include <grass/gis.h>
#include "rule.h"

#define LIST struct Histogram_list

void new_stats(char *name, struct Reclass *reclass)
{
    struct Histogram histo, histo2;
    struct Range range;
    CELL cat, cat2;
    int i;
    CELL min, max;

    min = reclass->min;
    max = reclass->max;

    /* read histogram for original file */
    G_suppress_warnings(1);
    i = G_read_histogram(reclass->name, reclass->mapset, &histo);
    G_suppress_warnings(0);
    if (i <= 0)
	return;

    /* compute data rage for reclass */
    G_init_range(&range);

    for (i = 0; i < histo.num; i++) {
	cat = histo.list[i].cat;
	if (cat < min || cat > max)
	    continue;
	cat2 = reclass->table[cat - min];
	G_update_range(cat2, &range);
    }
    G_write_range(name, &range);

    /* now generate a histogram from the original */

    /* allocate histogram list */
    histo2.num += range.max - range.min + 1;

    histo2.list = (LIST *) G_calloc(histo2.num, sizeof(LIST));

    /* set all counts to 0 */
    i = 0;
    for (cat = range.min; cat <= range.max; cat++) {
	histo2.list[i].cat = cat;
	histo2.list[i++].count = 0;
    }

    /* go thru original histogram and add into histo2 */
    for (i = 0; i < histo.num; i++) {
	cat = histo.list[i].cat;
	if (cat < min || cat > max)
	    G_set_c_null_value(&cat, 1);
	else
	    cat2 = reclass->table[cat - min];
	if (!G_is_c_null_value(&cat))
	    histo2.list[cat2 - range.min].count += histo.list[i].count;
    }
    G_write_histogram(name, &histo2);
}
