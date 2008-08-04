
/****************************************************************************
 *
 * MODULE:       d.rast.edit
 * AUTHOR(S):    Chris Rewerts, Agricultural Engineering, Purdue University 
 *                    (original contributor)
 *               Roberto Flor <flor itc.it>, 
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>
 * PURPOSE:      interactive editing of cell values
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
 * Pre-CVS edit comments:
 * April 1991 Update:
 * March 1992 (add vector overlay, misc touchups) Update: May 1992   (removed
 * parser routine to ask about grid color, since that can be changed easily
 * enough with the menu. added a lookup of cell value ranges, and ability to
 * edit negative cell values. also more misc touchups)
 * 
 * d.rast.edit is a graphically interactive cell editor which allows the user
 * to display a cell layer on the graphics monitor and use the mouse cursor
 * to indicate which cell values to edit by pointing to cells displayed.
 * 
 * d.rast.edit determines the name and mapset of the cell layer displayed on the
 * monitor, then makes a copy of the layer's values in a temporary file.
 * Note: current window settings and masks are ignored when making the copy,
 * so that the view on the screen can be various "zoom" views to allow the
 * user to find individual cells. After editing, a new cell layer is created.
 * 
 */
#include <stdlib.h>
#include <string.h>
#define GLOBAL
#include <grass/glocale.h>
#include "edit.h"


int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *outmap;
    char temp[128], line[128];
    char *m;
    struct FPRange fp_range;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display, raster");
    module->description =
	_("Interactively edit cell values in a raster map.");

    outmap = G_define_standard_option(G_OPT_R_OUTPUT);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /*
     * note: grid color used to be an option set with the help of the
     * parser, but it was more bother than worth. if black is not what
     * the user wants, they can reset it with the menu
     */

    strcpy(grid_color_name, DEFAULT_FG_COLOR);
    grid_color = D_translate_color(grid_color_name);

    if (R_open_driver() != 0)
	G_fatal_error("No graphics device selected");
    /*
     * if the monitor is divided into more than one window, find which is
     * the choosen, current one being used
     */

    if (D_get_cur_wind(temp))
	error(1, "No current graphics window");

    if (D_set_cur_wind(temp))
	error(1, "Current graphics window not available");

    /*
     * now that we have established what monitor window we are to work
     * with, find out the name of the cell layer currently displayed
     * there
     */

    D_get_cell_name(temp);
    if (*temp == 0 || strcmp(temp, "full_screen") == 0)
	error(1, "no map displayed in monitor.");

    m = G_find_cell2(temp, "");
    if (m == NULL) {
	sprintf(line, " %s - raster map not found\n", temp);
	error(1, line);
    }
    sscanf(m, "%s", orig_mapset);
    sscanf(temp, "%s", orig_name);

    fprintf(stderr, "\n\nName of original raster layer: [%s] in [%s]\n",
	    orig_name, orig_mapset);

    G_read_fp_range(orig_name, orig_mapset, &fp_range);
    G_get_fp_range_min_max(&fp_range, &min_value, &max_value);

    /* get the name for the new raster map to be created (the one we will edit) */
    strncpy(new_name, outmap->answer, 39);
    new_name[39] = '\0';

    /* get names straight  */
    sscanf(m, "%s", user_mapset);
    strcpy(current_name, orig_name);
    strcpy(current_mapset, orig_mapset);

    /*
     * we are not in the business of resampling the old file to a new
     * raster map with a different window setting, so when we make a copy
     * of the raster map to be edited, we will ignor the current window
     * and find out what the cell_head window is (and call that
     * "real_window")(the current_window could be anything, since the
     * user will probably have to use zoom to get a detailed view of what
     * they are editing).
     */

    G_get_cellhd(orig_name, orig_mapset, &real_window);

    real_nrows = real_window.rows;
    real_ncols = real_window.cols;

    main_menu();

    ext();
}

/*---------------------------------------------------------------*/
int do_edit(
	       /*
	        * called by the edit() routine. given a row and col location a value for the
	        * cell at that location, find that location in the temporary file and write
	        * in the new value
	        */
	       int c_row, int c_col, double new_value)
{

    int fd;
    long offset;
    DCELL c;

    fd = open(tempfile, 2);
    if (c_row > real_nrows - 1)
	error(0, "bad row number\n");
    if (c_col > real_ncols - 1)
	error(0, "bad col number\n");

    /* how many bytes into the temporary file will this value be? */
    offset = ((c_row * real_ncols) + c_col) * cellsize;

    lseek(fd, offset, 0);
    read(fd, &c, cellsize);

    lseek(fd, offset, 0);
    G_set_raster_value_d((void *)&c, new_value, map_type);
    if (write(fd, &c, cellsize) != cellsize)
	error(0, "was not able to write new value");

    close(fd);

    return 0;
}

/*------------------------------------------------------------------*/
int error(
	     /*
	      * some sort of function to deal with errors. code 0: print warning and
	      * continue code 1: print error message, close things down, and die
	      */
	     int code, char message[128]
    )
{

    /* int cellfd; */

    if (code == 0)
	fprintf(stderr, "\n\7WARNING: %s\n", message);
    else if (code == 1)
	fprintf(stderr, "\n\7ERROR: %s\n", message);
    if (code == 1) {
	/* G_unopen_cell(cellfd); */
	unlink(tempfile);
	R_close_driver();
	fprintf(stderr,
		"\n     +-------------------------------------------+\n");
	fprintf(stderr,
		"     |                d.rast.edit aborts         |\n");
	fprintf(stderr,
		"     +-------------------------------------------+\n\n");
	exit(-1);
    }

    return 0;
}

/*-------------------------------------------------------------*/

int ext(void)
{
    R_close_driver();
    fprintf(stderr, "\n     +-------------------------------------------+\n");
    fprintf(stderr, "     |                 d.rast.edit exits         |\n");
    fprintf(stderr, "     +-------------------------------------------+\n\n");
    exit(0);
}
