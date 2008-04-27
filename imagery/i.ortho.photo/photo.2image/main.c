/****************************************************************************
 *
 * MODULE:       photo.2image
 * AUTHOR(S):    Mike Baba,  DBA Systems, Inc. (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      mark fiducial or reseau points on an image
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#define GLOBAL

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include "globals.h"


int main (int argc, char *argv[])
{
    char *name, *location, *mapset, *camera;
    int nfiles;
    struct Cell_head cellhd;
    /* struct Ortho_Image_Group    group;   -- in globals.h */

    if (argc != 2)
	G_fatal_error("usage: %s group.", argv[0]);

    G_gisinit (argv[0]);
    G_suppress_masking();	/* need to do this for target location */
    name     = (char *) G_malloc (40 * sizeof (char));
    location = (char *) G_malloc (80 * sizeof (char));
    mapset   = (char *) G_malloc (80 * sizeof (char));
    camera   = (char *) G_malloc (40 * sizeof (char));

    interrupt_char = G_intr_char();
    tempfile1 = G_tempfile();
    tempfile2 = G_tempfile();
    tempfile3 = G_tempfile();
    cell_list = G_tempfile();
    vect_list = G_tempfile();
    group_list = G_tempfile();
    digit_points = G_tempfile();

    if (R_open_driver() != 0)
           G_fatal_error ("No graphics device selected");

    /* get image group and image group referenc file */
    name = argv[1];
    strcpy  (group.name, name);
    if (!I_find_group (group.name))
	G_fatal_error("Image Group [%s] not found.", group.name);

    /* get the group ref */    
    I_get_group_ref (group.name, &group.group_ref);
    nfiles = group.group_ref.nfiles;

    /* write block files to block list file */
    prepare_group_list();

    /** look for camera info  for this block **/
    G_suppress_warnings(1);    
    if (!I_get_group_camera (group.name,camera))
        G_fatal_error("No camera reference file selected for group [%s].", group.name);

    if (!I_get_cam_info (camera, &group.camera_ref))
        G_fatal_error("Bad format in camera file for group [%s].", group.name);
    G_suppress_warnings(0);

    /* read block reference points, if any */
    G_suppress_warnings(1);
    if (!I_get_ref_points(group.name, &group.photo_points))
	group.photo_points.count = 0;
    G_suppress_warnings(0);

    /* determine tranformation equation */
    Compute_equation();

    signal (SIGINT, SIG_IGN);
#ifdef SIGQUIT
    signal (SIGQUIT, SIG_IGN); 
#endif

    Init_graphics();
    display_title (VIEW_MAP1);
    select_current_env ();

    Begin_curses();
    G_set_error_routine (error);

/*
#ifdef SIGTSTP
    signal (SIGTSTP, SIG_IGN);
#endif
*/

    /* ask user for raster map to be displayed */
    do
    {  if(!choose_groupfile (name, mapset)) 
	    quit(0);
    } while (G_get_cellhd (name, mapset, &cellhd) < 0);

    /* display this file in "map1" */
    G_adjust_window_to_box (&cellhd, &VIEW_MAP1->cell.head, VIEW_MAP1->nrows, VIEW_MAP1->ncols);
    Configure_view (VIEW_MAP1, name, mapset, cellhd.ns_res, cellhd.ew_res);

    drawcell(VIEW_MAP1);
    display_ref_points(1);
    Curses_clear_window (PROMPT_WINDOW);

    /* determine initial input method. */
    if (setup_camera_file() < 0) quit(0); 
    if (use_camera_file)
      { from_keyboard  = 0;
        from_screen = 1;
	from_flag = 1;
      }
    else
      { from_keyboard  = 1;
        from_screen = 0;
	from_flag = 0;
      }


    /* go do the work */
    driver();
    /* leave */
    quit(0);
}

int quit (int n)
{
    End_curses();
    R_close_driver();
    unlink (tempfile1);
    unlink (tempfile2);
    unlink (tempfile3);
    unlink (cell_list);
    unlink (group_list);
    unlink (vect_list);
    unlink (digit_points);
    exit(n);
}

int error (const char *msg, int fatal)
{
    static char buf[200];
    int x,y,button;

Curses_clear_window (PROMPT_WINDOW);
Curses_write_window (PROMPT_WINDOW,1,1, "LOCATION:\n");
Curses_write_window (PROMPT_WINDOW,1,12,G_location());
Curses_write_window (PROMPT_WINDOW,2,1, "MAPSET:\n");
Curses_write_window (PROMPT_WINDOW,2,12,G_location());
    Beep();
    if (fatal)
	sprintf (buf, "ERROR: %s", msg);
    else
	sprintf (buf, "WARNING: %s (click mouse to continue)", msg);
    Menu_msg (buf);

    if (fatal)
	quit(1);
    Mouse_pointer (&x, &y, &button);
    Curses_clear_window (PROMPT_WINDOW);

    return 0;
}



