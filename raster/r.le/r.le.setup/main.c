/*
 ************************************************************
 * MODULE: r.le.setup/main.c                                *
 *         Version 5.0beta            Oct. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To set up sampling areas, which can can then    *
 *         be used to obtain data using the r.le.dist,      *
 *         r.le.patch, and r.le.pixel programs.  The        *
 *         main.c code queries the user for the name of     *
 *         maps to be used during the setup operation       *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "setup.h"


static void get_pwd(void);

jmp_buf jmp;

				/* MAIN PROGRAM */

int main(int argc, char *argv[])
{

    struct GModule *module;
    struct Option *input, *vect;

    struct Cell_head window;
    int bot, right, t0, b0, l0, r0, clear = 0;
    double Rw_l, Rscr_wl;
    char *map_name = NULL, *v_name = NULL, *s_name = NULL;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* must run in a term window */
    G_putenv("GRASS_UI_TERM", "1");

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Interactive tool used to setup the sampling and analysis framework "
	 "that will be used by the other r.le programs.");

    input = G_define_standard_option(G_OPT_R_MAP);
    input->description = _("Raster map to use to setup sampling");

    vect = G_define_standard_option(G_OPT_V_INPUT);
    vect->key = "vect";
    vect->description = _("Vector map to overlay");
    vect->required = NO;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    setbuf(stdout, NULL);	/* unbuffered */
    setbuf(stderr, NULL);

    G_sleep_on_error(1);	/* error messages get lost on clear screen */


    map_name = input->answer;
    v_name = vect->answer;
    s_name = NULL;		/* sites not used in GRASS 6 */

    /* setup the r.le.para directory */
    get_pwd();

    /* query for the map to be setup */
    if (R_open_driver() != 0)
	G_fatal_error("No graphics device selected");

    /* setup the current window for display & clear screen */
    D_setup(1);

    Rw_l = (double)G_window_cols() / G_window_rows();
    /*R_open_driver(); */
    /* R_font("romant"); */
    G_get_set_window(&window);
    t0 = R_screen_top();
    b0 = R_screen_bot();
    l0 = R_screen_left();
    r0 = R_screen_rite();
    Rscr_wl = (double)(r0 - l0) / (b0 - t0);

    if (Rscr_wl > Rw_l) {
	bot = b0;
	right = l0 + (b0 - t0) * Rw_l;
    }

    else {
	right = r0;
	bot = t0 + (r0 - l0) / Rw_l;
    }
    D_new_window("a", t0, bot, l0, right);
    D_set_cur_wind("a");
    D_show_window(D_translate_color("green"));
    D_setup(clear);
    R_close_driver();

    /* invoke the setup modules */
    set_map(map_name, v_name, s_name, window, t0, bot, l0, right);

    return (EXIT_SUCCESS);
}




/* SETUP THE R.LE.PARA DIRECTORY */
static void get_pwd(void)
{
    DIR *dp;

    if (!(dp = opendir("r.le.para")))
	G_mkdir("r.le.para");
    else
	closedir(dp);
    return;

}
