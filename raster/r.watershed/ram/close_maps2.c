#include "Gwater.h"
#include <unistd.h>
#include <grass/glocale.h>

int close_array_seg(void)
{
    struct Colors colors;
    int incr, max, red, green, blue, rd, gr, bl, flag;
    int c, r, map_fd;
    CELL *cellrow, value;
    CELL *theseg;
    RAMSEG thesegseg;

    cellrow = Rast_allocate_c_buf();
    if (seg_flag || bas_flag || haf_flag) {
	if (seg_flag) {
	    theseg = bas;
	    thesegseg = bas_seg;
	}
	else if (bas_flag) {
	    theseg = bas;
	    thesegseg = bas_seg;
	}
	else {
	    theseg = haf;
	    thesegseg = haf_seg;
	}
	max = n_basins;
	G_debug(1, "%d basins created", max);
	Rast_init_colors(&colors);
	if (max > 0)
	    Rast_make_random_colors(&colors, 1, max);
	else {
	    G_warning(_("No basins were created. Verify threshold and region settings."));
	    Rast_make_random_colors(&colors, 1, 2);
	}

	if (max < 1000 && max > 0) {
	    Rast_set_c_color((CELL) 0, 0, 0, 0, &colors);
	    r = 1;
	    incr = 0;
	    while (incr >= 0) {
		G_percent(r, max, 2);
		for (gr = 130 + incr; gr <= 255; gr += 20) {
		    for (rd = 90 + incr; rd <= 255; rd += 30) {
			for (bl = 90 + incr; bl <= 255; bl += 40) {
			    flag = 1;
			    while (flag) {
				Rast_get_c_color(&r, &red, &green, &blue,
						 &colors);
				/* if existing rule is too dark then append a new
				   rule to override it */
				if ((blue * .11 + red * .30 + green * .59) <
				    100) {
				    Rast_set_c_color(r, rd, gr, bl, &colors);
				    flag = 0;
				}
				if (++r > max) {
				    gr = rd = bl = 300;
				    flag = 0;
				    incr = -1;
				}
			    }
			}
		    }
		}
		if (incr >= 0) {
		    incr += 15;
		    if (incr > 120)
			incr = 7;
		}
	    }
	    G_percent(r - 1, max, 3);	/* finish it */
	}
	else if (max >= 1000)
	    G_debug(1,
		    "Too many subbasins to reasonably check for color brightness");
	/* using the existing stack of while/for/for/for/while loops ... */
    }

    /* stream segments map */
    if (seg_flag) {
	map_fd = Rast_open_c_new(seg_name);
	for (r = 0; r < nrows; r++) {
	    Rast_set_c_null_value(cellrow, ncols);	/* reset row to all NULL */
	    for (c = 0; c < ncols; c++) {
		value = FLAG_GET(swale, r, c);
		if (value)
		    cellrow[c] = bas[SEG_INDEX(bas_seg, r, c)];
	    }
	    Rast_put_row(map_fd, cellrow, CELL_TYPE);
	}
	Rast_close(map_fd);
	Rast_write_colors(seg_name, this_mapset, &colors);
    }

    /* basins map */
    if (bas_flag) {
	map_fd = Rast_open_c_new(bas_name);
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		cellrow[c] = bas[SEG_INDEX(bas_seg, r, c)];
		if (cellrow[c] == 0)
		    Rast_set_c_null_value(cellrow + c, 1);
	    }
	    Rast_put_row(map_fd, cellrow, CELL_TYPE);
	}
	Rast_close(map_fd);
	Rast_write_colors(bas_name, this_mapset, &colors);
    }

    /* half_basins map */
    if (haf_flag) {
	map_fd = Rast_open_c_new(haf_name);
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		cellrow[c] = haf[SEG_INDEX(haf_seg, r, c)];
		if (cellrow[c] == 0)
		    Rast_set_c_null_value(cellrow + c, 1);
	    }
	    Rast_put_row(map_fd, cellrow, CELL_TYPE);
	}
	Rast_close(map_fd);
	Rast_write_colors(haf_name, this_mapset, &colors);
    }

    if (seg_flag || bas_flag || haf_flag)
	Rast_free_colors(&colors);

    G_free(haf);
    G_free(bas);
    G_free(cellrow);
    if (arm_flag)
	fclose(fp);
    close_maps();

    return 0;
}
