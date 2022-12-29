#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "watershed.h"


int valid_basins(char *accum_name, OUTPUT * output)
{
    int i, r, c, fd;

    /* int nrows, ncols; */
    CELL *buf;
    B_FACTS *basin_facts, *basin, *down_basin;
    char *mapset;
    struct Cell_head *window;

    window = &(output->window);
    /*
       nrows = Rast_window_rows ();
       ncols = Rast_window_cols ();
     */
    if (NULL == (mapset = G_find_raster(accum_name, ""))) {
	free_output(output);
	G_fatal_error(_("accum file missing in valid_basins()"));
    }
    fd = Rast_open_old(accum_name, mapset);

    buf = Rast_allocate_c_buf();
    basin_facts = output->basin_facts;
    for (i = output->num_basins - 1; i >= 0; i--) {
	basin = &(basin_facts[i]);
	if (basin->valid == 0) {
	    if (basin->down_basin >= 0)
		basin_facts[basin->down_basin].valid = 0;
	}
	else {
	    if (basin->down_basin >= 0)
		down_basin = &(basin_facts[basin->down_basin]);
	    else
		down_basin = NULL;
	    r = (int)(window->north - basin->northing) / window->ns_res - 1;
	    c = (int)(basin->easting - window->west) / window->ew_res;
	    /*
	       if (r < 0 || r >= nrows || c < 0 || c>= ncols) {
	       G_fatal_error("r:%d c:%d big error", r,c);
	       }
	     */
	    Rast_get_c_row(fd, buf, r);
	    if (buf[c] < 0) {
		basin->valid = 0;
		if (down_basin != NULL)
		    down_basin->valid = 0;
	    }
	}
    }
    G_free(buf);

    return 0;
}
