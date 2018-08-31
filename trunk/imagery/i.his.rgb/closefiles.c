#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"

/* This routine closes up the cell maps, frees up the row buffers and
   use a less than perfect way of setting the color maps for the output
   to grey scale.  */

int closefiles(char *r_name, char *g_name, char *b_name,
	       int fd_output[3], CELL * rowbuf[3])
{
    int i;
    struct Colors colors;
    struct Range range;
    struct History history;
    CELL min, max;
    const char *mapset;

    for (i = 0; i < 3; i++) {
	Rast_close(fd_output[i]);
	G_free(rowbuf[i]);
    }

    mapset = G_mapset();

    /* write colors */
    /*   set to 0,max_level instead of min,max ?? */
    Rast_read_range(r_name, mapset, &range);
    Rast_get_range_min_max(&range, &min, &max);
    Rast_make_grey_scale_colors(&colors, min, max);
    Rast_write_colors(r_name, mapset, &colors);

    Rast_read_range(g_name, mapset, &range);
    Rast_get_range_min_max(&range, &min, &max);
    Rast_make_grey_scale_colors(&colors, min, max);
    Rast_write_colors(g_name, mapset, &colors);

    Rast_read_range(b_name, mapset, &range);
    Rast_get_range_min_max(&range, &min, &max);
    Rast_make_grey_scale_colors(&colors, min, max);
    Rast_write_colors(b_name, mapset, &colors);

    /* write metadata */
    Rast_short_history(r_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(r_name, &history);
    Rast_put_cell_title(r_name, "Image red");

    Rast_short_history(g_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(g_name, &history);
    Rast_put_cell_title(g_name, "Image green");

    Rast_short_history(b_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(b_name, &history);
    Rast_put_cell_title(b_name, "Image blue");

    return 0;
}
