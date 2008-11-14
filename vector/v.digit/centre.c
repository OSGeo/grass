/* ***************************************************************
 * *
 * * MODULE:       v.digit
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               
 * * PURPOSE:      Edit vector
 * *              
 * * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 * *
 * *               This program is free software under the 
 * *               GNU General Public License (>=v2). 
 * *               Read the file COPYING that comes with GRASS
 * *               for details.
 * *
 * **************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "global.h"
#include "proto.h"

/* This function is started from the GUI, it regularly updates GUI and checks GUI requirements. 
 *  If Tool_next is set by GUI, the tool is started by the tool_centre()
 */
void tool_centre(void)
{
    /* Init variables */
    var_init();

    /* Init snap */
    var_seti(VAR_SNAP, 1);
    var_seti(VAR_SNAP_MODE, SNAP_SCREEN);
    var_seti(VAR_SNAP_SCREEN, 10);
    var_setd(VAR_SNAP_MAP, 10);

    G_get_window(&window);

    /* Set tool */
    Tool_next = TOOL_NOTHING;

    /* Display the map */
    symb_init();
    G_get_window(&window);
    driver_open();
    display_erase();
    display_bg();
    display_map();
    driver_close();

    symb_init_gui();
    i_prompt("Select tool");
}

void next_tool(void)
{
    switch (Tool_next) {
    case TOOL_EXIT:
	G_debug(2, "Quit");
	end();
	break;
    case TOOL_NEW_POINT:
	/* Tool_next = TOOL_NOTHING; *//* Commented -> Draw next one once first is done */
	new_line(GV_POINT);
	break;
    case TOOL_NEW_LINE:
	new_line(GV_LINE);
	break;
    case TOOL_NEW_BOUNDARY:
	new_line(GV_BOUNDARY);
	break;
    case TOOL_NEW_CENTROID:
	new_line(GV_CENTROID);
	break;
    case TOOL_MOVE_VERTEX:
	Tool_next = TOOL_NOTHING;
	move_vertex();
	break;
    case TOOL_ADD_VERTEX:
	Tool_next = TOOL_NOTHING;
	add_vertex();
	break;
    case TOOL_RM_VERTEX:
	Tool_next = TOOL_NOTHING;
	rm_vertex();
	break;
    case TOOL_SPLIT_LINE:
	Tool_next = TOOL_NOTHING;
	split_line();
	break;
    case TOOL_EDIT_LINE:
	Tool_next = TOOL_NOTHING;
	edit_line();
	break;
    case TOOL_MOVE_LINE:
	Tool_next = TOOL_NOTHING;
	move_line();
	break;
    case TOOL_DELETE_LINE:
	Tool_next = TOOL_NOTHING;
	delete_line();
	break;
    case TOOL_DISPLAY_CATS:
	Tool_next = TOOL_NOTHING;
	display_cats();
	break;
    case TOOL_COPY_CATS:
	Tool_next = TOOL_NOTHING;
	copy_cats();
	break;
    case TOOL_DISPLAY_ATTRIBUTES:
	Tool_next = TOOL_NOTHING;
	display_attributes();
	break;
    case TOOL_DISPLAY_SETTINGS:
	Tool_next = TOOL_NOTHING;
	Tcl_Eval(Toolbox, "settings");
	break;
    case TOOL_ZOOM_WINDOW:
	Tool_next = TOOL_NOTHING;
	zoom_window();
	break;
    case TOOL_ZOOM_OUT_CENTRE:
	Tool_next = TOOL_NOTHING;
	zoom_centre(2);
	break;
    case TOOL_ZOOM_PAN:
	Tool_next = TOOL_NOTHING;
	zoom_pan();
	break;
    case TOOL_ZOOM_DEFAULT:
	Tool_next = TOOL_NOTHING;
	zoom_default();
	break;
    case TOOL_ZOOM_REGION:
	Tool_next = TOOL_NOTHING;
	zoom_region();
	break;
    case TOOL_REDRAW:
	Tool_next = TOOL_NOTHING;
	driver_open();
	display_redraw();
	driver_close();
	break;
    case TOOL_NOTHING:
	break;
    }
}

/* This function is regularly called from R_get_location_*() functions to enable GUI to kill running tool */
void update(int wx, int wy)
{
    double x, y;

    G_debug(5, "Update function wx = %d wy = %d", wx, wy);

    if (wx != COOR_NULL && wy != COOR_NULL) {
	x = D_d_to_u_col(wx);
	y = D_d_to_u_row(wy);
	i_coor(x, y);
    }
}

void end(void)
{
    G_debug(1, "end()");
    Vect_build_partial(&Map, GV_BUILD_NONE);
    Vect_build(&Map);
    Vect_close(&Map);

    if (1 == G_put_window(&GRegion))
	G_message(_("Region restored to original extent."));

    /* clear the screen */
    Tcl_Eval(Toolbox, ".screen.canvas delete all");

    exit(EXIT_SUCCESS);
}
