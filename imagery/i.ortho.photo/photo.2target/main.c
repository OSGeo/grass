/****************************************************************************
 *
 * MODULE:       photo.2target
 * AUTHOR(S):    Mike Baba,  DBA Systems, Inc. (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Radim Blazek <radim.blazek gmail.com>
 * PURPOSE:      allow user to mark control points on an image to be 
 *               ortho-rectified and then input the coordinates of each point 
 *               for calculation of rectification parameters
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* main.c */

#define GLOBAL
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/ortholib.h>
#include <grass/glocale.h>
#include "globals.h"
#include "local_proto.h"

int main (int argc, char *argv[])
{
    char *name, *location, *mapset, *camera, msg[100];
    struct GModule *module;
    struct Option *group_opt, *map_opt, *target_map_opt;
    struct Cell_head cellhd;
    int ok;
    int nfiles;

    G_gisinit (argv[0]);

    module = G_define_module();
    module->keywords = _("imagery");
    module->description = _("Creates control points on an image "
                            "to be ortho-rectified.");

    group_opt = G_define_option();
    group_opt->key = "group";
    group_opt->type = TYPE_STRING;
    group_opt->required = YES;
    group_opt->multiple = NO;
    group_opt->description= _("Name of imagery group");

    map_opt = G_define_standard_option(G_OPT_R_MAP);
    map_opt->required = NO;
    map_opt->description= _("Name of image to be rectified which will "
                            " be initialy drawn on screen.");

    target_map_opt = G_define_standard_option(G_OPT_R_MAP);
    target_map_opt->key = "target";
    target_map_opt->required = NO;
    target_map_opt->description= _("Name of a map from target mapset which "
                               " will be initialy drawn on screen.");

    if (G_parser(argc, argv))
	exit(1);

    G_suppress_masking();	/* need to do this for target location */
    name     = (char *) G_malloc (40 * sizeof (char));
    location = (char *) G_malloc (80 * sizeof (char));
    mapset   = (char *) G_malloc (80 * sizeof (char));
    camera   = (char *) G_malloc (40 * sizeof (char));

    interrupt_char = G_intr_char();
    tempfile1 = G_tempfile();
    tempfile2 = G_tempfile();
    tempfile_dot = G_tempfile();
    tempfile_dot2 = G_tempfile();
    tempfile_win = G_tempfile();
    tempfile_win2 = G_tempfile();
    cell_list = G_tempfile();
    vect_list = G_tempfile();
    group_list = G_tempfile();
    digit_points = G_tempfile();

    if (R_open_driver() != 0)
	G_fatal_error (_("No graphics device selected"));

    /* get group ref */
    strcpy (group.name, group_opt->answer );
    if (!I_find_group (group.name))
    {
	fprintf (stderr, "Group [%s] not found\n", group.name);
	exit(1);
    }


    /* get the group ref */    
    I_get_group_ref (group.name, &group.group_ref);
    nfiles = group.group_ref.nfiles;

    /* write block files to block list file */
    prepare_group_list();

    /** look for camera info  for this group**/
    G_suppress_warnings(1);
    if (!I_get_group_camera (group.name,camera))
    {   sprintf (msg, "No camera reference file selected for group [%s]\n", 
		group.name);
	G_fatal_error(msg);
    }

    if (!I_get_cam_info (camera, &group.camera_ref))
    {   sprintf (msg, "Bad format in camera file for group [%s]\n",group.name);
	G_fatal_error(msg);
    }
    G_suppress_warnings(0);

    /* get initial camera exposure station, if any*/
    if (! (ok = I_find_initial(group.name)))
    {
       sprintf (msg, "No initial camera exposure station for group[%s]\n",
		group.name);
       G_warning(msg);
    }

    if (ok)
      if (!I_get_init_info (group.name, &group.camera_exp))
      {
         sprintf (msg, "Bad format in initial camera exposure station for group [%s]\n", group.name);
         G_warning (msg);
      }

    /* get target info and enviroment */
    G_suppress_warnings(1);
    get_target();
    find_target_files();
    G_suppress_warnings(0);

    /* read group reference points, if any */
    G_suppress_warnings(1);
    if (!I_get_ref_points (group.name, &group.photo_points))
      {
        G_suppress_warnings(0);
        if (group.photo_points.count == 0)
           sprintf (msg, "No photo points for group [%s].\n", group.name);
        else if (group.ref_equation_stat == 0)
           sprintf (msg, "Poorly placed photo points for group [%s].\n", 
		group.name);
        G_fatal_error (msg);
      }
    G_suppress_warnings(0);

    /* determine tranformation equation */
    Compute_ref_equation(); 

    /* read group control points, format: image x,y,cfl; target E,N,Z*/
    G_suppress_warnings(1);
    if (!I_get_con_points (group.name, &group.control_points))
	group.control_points.count = 0;
    G_suppress_warnings(0);

    /* compute image coordinates of photo control points */
    /********
    I_convert_con_points (group.name, &group.control_points, 
			 &group.control_points, group.E12, group.N12);
    ********/

    /* determine tranformation equation */
    fprintf (stderr,"Computing equations ...\n");
    if (group.control_points.count > 0)
        Compute_ortho_equation();


/*   signal (SIGINT, SIG_IGN); */
/*   signal (SIGQUIT, SIG_IGN); */

    select_current_env();
    Init_graphics();
    display_title (VIEW_MAP1);
    select_target_env ();
    display_title (VIEW_MAP2);
    select_current_env ();

    Begin_curses();
    G_set_error_routine (error);

/*
#ifdef SIGTSTP
    signal (SIGTSTP, SIG_IGN);
#endif
*/

    /* Set image to be rectified */
    if ( map_opt->answer )
    {
        char *ms;
        ms = G_find_cell ( map_opt->answer, "");
        if (ms == NULL) {
           G_fatal_error(_("Raster map <%s> not found"), map_opt->answer);
        }
        strcpy ( name, map_opt->answer );
        strcpy ( mapset, ms );
        if ( G_get_cellhd (name, mapset, &cellhd) < 0 )
        {
           G_fatal_error(_("Unable to read head of %s"), map_opt->answer);
        }
    }
    else
    {
	/* ask user for group file to be displayed */
	do
	{
	    if(!choose_groupfile (name, mapset))
		quit(0);
	    /* display this file in "map1" */
	} while (G_get_cellhd (name, mapset, &cellhd) < 0);
    }

    G_adjust_window_to_box (&cellhd, &VIEW_MAP1->cell.head, VIEW_MAP1->nrows, 
			    VIEW_MAP1->ncols);
    Configure_view (VIEW_MAP1, name, mapset, cellhd.ns_res, cellhd.ew_res);

    drawcell(VIEW_MAP1);

    /* Set target map if specified */
    if ( target_map_opt->answer )
    {
        char *ms;

        select_target_env();
        ms = G_find_cell ( target_map_opt->answer, "");
        if (ms == NULL) {
           G_fatal_error(_("Raster map <%s> not found"), target_map_opt->answer);
        }
        strcpy ( name, target_map_opt->answer );
        strcpy ( mapset, ms );
        if ( G_get_cellhd (name, mapset, &cellhd) < 0 )
        {
           G_fatal_error(_("Unable to read head of %s"), target_map_opt->answer);
        }

	G_adjust_window_to_box (&cellhd, &VIEW_MAP2->cell.head, VIEW_MAP2->nrows, 
				VIEW_MAP2->ncols);
	Configure_view (VIEW_MAP2, name, mapset, cellhd.ns_res, cellhd.ew_res);

        drawcell(VIEW_MAP2);

        from_flag = 1;
        from_keyboard = 0;
        from_screen = 1;
    }

    display_conz_points(1);

    Curses_clear_window (PROMPT_WINDOW);

    /* determine initial input method. */
    setup_digitizer();
    if (use_digitizer)
    {
	from_digitizer = 1;
	from_keyboard  = 0;
	from_flag = 1;
    }

    /* go do the work */
    driver();

    quit(0);
}

int quit (int n)
{
    char command[1024];

    End_curses();
    R_close_driver();
    if (use_digitizer)
    {
	sprintf (command, "%s/etc/geo.unlock %s",
	    G_gisbase(), digit_points);
	system (command);
    }
    unlink (tempfile1);
    unlink (tempfile2);
    unlink (cell_list);
    unlink (group_list);
    unlink (vect_list);
    unlink (digit_points);
    unlink (tempfile_elev);
    unlink (tempfile_dot);
    unlink (tempfile_dot2);
    unlink (tempfile_win);
    unlink (tempfile_win2);
    exit(n);
}

int error (const char *msg, int fatal)
{
    char buf[200];
    int x,y,button;

Curses_clear_window (PROMPT_WINDOW);
Curses_write_window (PROMPT_WINDOW,1,1, "LOCATION:\n");
Curses_write_window (PROMPT_WINDOW,1,12,G_location());
Curses_write_window (PROMPT_WINDOW,2,1, "MAPSET:\n");
Curses_write_window (PROMPT_WINDOW,2,12,G_mapset());
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



