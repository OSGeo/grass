/****************************************************************************
 *
 * MODULE:       d.rgb
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor)
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Markus Neteler <neteler itc.it>, 
 *               Radim Blazek <radim.blazek gmail.com>
 *               Martin Landa <landa.martin gmail.com> (nulls opaque)
 * PURPOSE:      Combine three rasters to form a colour image using red, green,
 *               and blue display channels
 * COPYRIGHT:    (C) 2001-2007, 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>

struct band
{
    struct Option *opt;
    int file;
    int type;
    void *array;
    struct Colors colors;
};

static char *const color_names[3] = { "red", "green", "blue" };

int main(int argc, char **argv)
{
    struct band B[3];
    int row;
    int next_row;
    int overlay;
    struct Cell_head window;
    struct GModule *module;
    struct Flag *flag_n;
    int i;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("raster"));
    G_add_keyword("RGB");
    module->description =
	_("Displays three user-specified raster maps "
	  "as red, green, and blue overlays in the active graphics frame.");

    flag_n = G_define_flag();
    flag_n->key = 'n';
    flag_n->description = _("Make null cells opaque");
    flag_n->guisection = _("Null cells");
    
    for (i = 0; i < 3; i++) {
	char buff[80];

	sprintf(buff, _("Name of raster map to be used for '%s'"),
		color_names[i]);

	B[i].opt = G_define_standard_option(G_OPT_R_MAP);
	B[i].opt->key = G_store(color_names[i]);
	B[i].opt->description = G_store(buff);
    }

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Do screen initializing stuff */
    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    
    overlay = !flag_n->answer;

    D_setup(0);
    D_set_overlay_mode(overlay);

    for (i = 0; i < 3; i++) {
	/* Get name of layer to be used */
	char *name = B[i].opt->answer;

	/* Make sure map is available */
	B[i].file = Rast_open_old(name, "");

	B[i].type = Rast_get_map_type(B[i].file);

	/* Reading color lookup table */
	if (Rast_read_colors(name, "", &B[i].colors) == -1)
	    G_fatal_error(_("Color file for <%s> not available"), name);

	B[i].array = Rast_allocate_buf(B[i].type);
    }

    /* read in current window */
    G_get_window(&window);

    D_cell_draw_begin();

    next_row = 0;
    for (row = 0; row < window.rows;) {
	G_percent(row, window.rows, 5);

	for (i = 0; i < 3; i++)
	    Rast_get_row(B[i].file, B[i].array, row, B[i].type);

	if (row == next_row)
	    next_row = D_draw_raster_RGB(next_row,
					 B[0].array, B[1].array, B[2].array,
					 &B[0].colors, &B[1].colors,
					 &B[2].colors, B[0].type, B[1].type,
					 B[2].type);
	else if (next_row > 0)
	    row = next_row;
	else
	    break;
    }
    G_percent(window.rows, window.rows, 5);
    D_cell_draw_end();
    
    D_save_command(G_recreate_command());
    D_close_driver();

    /* Close the raster maps */
    for (i = 0; i < 3; i++)
	Rast_close(B[i].file);

    exit(EXIT_SUCCESS);
}
