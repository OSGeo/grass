/* select a monitor for graphics */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/monitors.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/display.h>

int 
main (int argc, char *argv[])
{
	char name[128];
	const char *fenc = getenv("GRASS_ENCODING");
	const char *font = getenv("GRASS_FONT");

	if (argc != 2)
	{
		G_fatal_error(_("Usage:  %s monitor_name"),argv[0]);
		exit(EXIT_FAILURE);
	}

	G_gisinit (argv[0]);

	G_unsetenv ("MONITOR");

	if (R_parse_monitorcap(MON_NAME,argv[1]) == NULL)
	{
		G_fatal_error(_("No such monitor as <%s>"),argv[1]);
		exit(EXIT_FAILURE);
	}

	/* change the environment variable */
	G__setenv("MONITOR",argv[1]);

/* now try to run the monitor to see if it is running and to lock it
 * set the font
 * if no current frame create a full screen window.
 */
	/* Don't do anything else if connecting to the driver fails */
	if (R_open_driver() != 0)
	    exit(EXIT_FAILURE);

	R_font(font ? font : "romans");

	if (fenc)
		R_charset(fenc);

	if (D_get_cur_wind(name) != 0)
		D_new_window("full_screen",
			     R_screen_top(), R_screen_bot(),
			     R_screen_left(), R_screen_rite());
	D_set_cur_wind("full_screen");

	R_close_driver();

	/* write the name to the .gisrc file */
	G__write_env();
        exit(EXIT_SUCCESS);
}
