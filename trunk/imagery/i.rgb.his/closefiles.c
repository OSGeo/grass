#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"

/* This routine closes up the cell maps, frees up the row buffers and
   use a less than perfect way of setting the color maps for the output
   to grey scale.  */

int closefiles(char *h_name, char *i_name, char *s_name,
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
    Rast_read_range(h_name, mapset, &range);
    Rast_get_range_min_max(&range, &min, &max);
    Rast_make_grey_scale_colors(&colors, min, max);
    Rast_write_colors(h_name, mapset, &colors);

    Rast_read_range(i_name, mapset, &range);
    Rast_get_range_min_max(&range, &min, &max);
    Rast_make_grey_scale_colors(&colors, min, max);
    Rast_write_colors(i_name, mapset, &colors);

    Rast_read_range(s_name, mapset, &range);
    Rast_get_range_min_max(&range, &min, &max);
    Rast_make_grey_scale_colors(&colors, min, max);
    Rast_write_colors(s_name, mapset, &colors);

    /* write metadata */
    Rast_short_history(h_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(h_name, &history);
    Rast_put_cell_title(h_name, "Image hue");

    Rast_short_history(i_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(i_name, &history);
    Rast_put_cell_title(i_name, "Image intensity");

    Rast_short_history(s_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(s_name, &history);
    Rast_put_cell_title(s_name, "Image saturation");

    return 0;
}

