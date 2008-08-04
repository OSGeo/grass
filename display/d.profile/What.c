#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "profile.h"

int What(char *name,
	 char *mapset, struct Cell_head window, double east, double north)
{
    struct Categories cat;
    int NoCatStrings = 0, row, col, fd;
    CELL *buf;
    char txt_buf[1024];

    row = (window.north - north) / window.ns_res;
    col = (east - window.west) / window.ew_res;

    buf = G_allocate_cell_buf();

    fd = G_open_cell_old(name, mapset);
    if (fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), name);

    if (G_read_cats(name, mapset, &cat) < 0)
	NoCatStrings = 1;

    if (G_get_map_row(fd, buf, row) < 0)
	G_fatal_error(_("Error reading raster map"));
    else {
	R_standard_color(D_translate_color(DEFAULT_BG_COLOR));
	D_erase_window();
	R_standard_color(D_translate_color("red"));
	R_flush();
	sprintf(txt_buf, "%s in mapset %s", name, mapset);
	DrawText(22, 1, 1, txt_buf);
	R_standard_color(D_translate_color(DEFAULT_FG_COLOR));
	sprintf(txt_buf, "EAST: %10.2f", east);
	DrawText(22, 2, 1, txt_buf);
	sprintf(txt_buf, "NORTH: %10.2f", north);
	DrawText(22, 3, 1, txt_buf);
	if (NoCatStrings)
	    sprintf(txt_buf, "(%d)", buf[col]);
	else
	    sprintf(txt_buf, "(%d) %s", buf[col], G_get_cat(buf[col], &cat));
	DrawText(22, 4, 1, txt_buf);
	R_stabilize();
    }
    G_unopen_cell(fd);

    return 0;
}
