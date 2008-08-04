#include <grass/gis.h>
/*
 * creates new category and color structures from the patching
 * files category and color files
 *
 * the first patch file is used as the basis. Its cats and colr
 * are read into the final cats/colr structures.
 * Then the other patching layers cats/colr are added to the
 * final cats/colr only if these patching layers actually
 * contributed new categories to the final result
 */

int support(char **names,
	    struct Cell_stats *statf, int nfiles,
	    struct Categories *cats,
	    int *cats_ok,
	    struct Colors *colr, int *colr_ok, RASTER_MAP_TYPE out_type)
{
    int i;
    char *mapset;
    struct Categories pcats;
    struct Colors pcolr;
    CELL n;
    long count;
    int red, grn, blu;
    int do_cats, do_colr;

    mapset = G_find_cell(names[0], "");
    *cats_ok = 1;
    *colr_ok = 1;
    if (G_read_cats(names[0], mapset, cats) < 0)
	*cats_ok = 0;
    G_suppress_warnings(1);
    if (G_read_colors(names[0], mapset, colr) < 0)
	*colr_ok = 0;
    G_suppress_warnings(0);

    if (*cats_ok == 0 && *colr_ok == 0)
	return 0;

    for (i = 1; i < nfiles; i++) {
	mapset = G_find_cell(names[i], "");
	do_cats = *cats_ok && (G_read_cats(names[i], mapset, &pcats) >= 0);
	G_suppress_warnings(1);
	do_colr = *colr_ok && (G_read_colors(names[i], mapset, &pcolr) >= 0);
	G_suppress_warnings(0);
	if (!do_cats && !do_colr)
	    continue;
	if (out_type == CELL_TYPE) {
	    G_rewind_cell_stats(statf + i);
	    while (G_next_cell_stat(&n, &count, statf + i))
		if (n && !G_find_cell_stat(n, &count, statf)) {
		    if (do_cats) {
			G_update_cell_stats(&n, 1, statf);
			G_set_cat(n, G_get_cat(n, &pcats), cats);
		    }
		    if (do_colr) {
			G_get_color(n, &red, &grn, &blu, &pcolr);
			G_set_color(n, red, grn, blu, colr);
		    }
		}
	}
	/* else the color will be the color of the first map */

	if (do_cats)
	    G_free_cats(&pcats);
	if (do_colr)
	    /* otherwise this memory is used in colr pointer */
	    G_free_colors(&pcolr);
    }
    return 1;
}
