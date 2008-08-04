#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "watershed.h"


int read_basins(char *haf_name, OUTPUT * output)
{
    int bas_fd, fd, m, r, nrows, c, ncols, tot_basins;
    CELL v, *buf, *bas_buf, b;
    CAT *cat;
    MAP *map;
    char *mapset;
    B_FACTS *facts;

    nrows = G_window_rows();
    ncols = G_window_cols();
    buf = G_allocate_cell_buf();
    bas_buf = G_allocate_cell_buf();
    mapset = G_find_cell(haf_name, "");
    if (!mapset) {
	G_fatal_error(_("unable to open basin/half basin map"));
    }

    bas_fd = G_open_cell_old(haf_name, mapset);
    facts = output->basin_facts;
    for (r = nrows - 1; r >= 0; r--) {
	G_get_c_raster_row(bas_fd, bas_buf, r);
	for (c = ncols - 1; c >= 0; c--) {
	    b = bas_buf[c] / 2 - 1;
	    if (b >= 0)
		facts[b].num_cells++;
	}
    }

    tot_basins = output->num_basins;
    for (m = 0; m < output->num_maps; m++) {
	map = &(output->maps[m]);
	G_read_cats(map->name, map->mapset, &(map->cats));
	map->basins = (BASIN *) G_malloc(tot_basins * sizeof(BASIN));
	for (r = 0; r < tot_basins; r++) {
	    map->basins[r].first_cat.num_cat = -1;
	    map->basins[r].first_cat.cat_val = -123456789;
	    map->basins[r].first_cat.nxt = NULL;
	    map->basins[r].sum_values = 0.0;
	}
	fd = G_open_cell_old(map->name, map->mapset);
	if (fd >= 0) {
	    for (r = 0; r < nrows; r++) {
		G_get_c_raster_row(fd, buf, r);
		G_get_c_raster_row(bas_fd, bas_buf, r);
		for (c = 0; c < ncols; c++) {
		    v = buf[c];
		    b = bas_buf[c] / 2 - 1;
		    if (b >= 0) {
			map->basins[b].sum_values += v;
			if (map->do_cats != 0) {
			    cat = &(map->basins[b].first_cat);
			    if (cat->num_cat == -1) {
				cat->num_cat = 1;
				cat->cat_val = v;
			    }
			    else
				insert_cat(cat, v, (int)1);
			}
		    }
		}
	    }
	    G_close_cell(fd);
	}
    }

    return 0;
}
