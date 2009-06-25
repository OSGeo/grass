/**
   \file vdigit/vertex.cpp

   \brief wxvdigit - Vertex manipulation

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008-2009 by Martin Landa, and the GRASS development team

   \author Martin Landa <landa.martin gmail.com>
*/

extern "C" {
#include <grass/vedit.h>
}
#include "driver.h"
#include "digit.h"

/**
   \brief Move vertex

   \param x,y,z coordinates (z is used only if map is 3d)
   \param move_x,move_y,move_z direction for moving vertex
   \param bgmap  map of background map or NULL
   \param snap snap mode (see vector/v.edit/lib/vedit.h)
   \param thresh_coords threshold value to identify vertex position
   \param thresh_snap threshold value to snap moved vertex

   \param id id of the new feature
   \param 0 nothing changed
   \param -1 error
*/
int Digit::MoveVertex(double x, double y, double z,
		      double move_x, double move_y, double move_z,
		      const char *bgmap, int snap,
		      double thresh_coords, double thresh_snap) {

    int ret;
    int changeset, nlines;
    struct line_pnts *point;
    struct Map_info **BgMap; /* backgroud vector maps */
    int nbgmaps;             /* number of registrated background maps */

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    if (display->selected.ids->n_values != 1)
	return 0;

    BgMap = NULL;
    nbgmaps = 0;
    if (bgmap && strlen(bgmap) > 0) {
	BgMap = OpenBackgroundVectorMap(bgmap);
	if (!BgMap) {
	    display->BackgroundMapMsg(bgmap);
	    return -1;
	}
	else {
	    nbgmaps = 1;
	}
    }

    point = Vect_new_line_struct();
    Vect_append_point(point, x, y, z);

    nlines = Vect_get_num_lines(display->mapInfo);

    changeset = AddActionsBefore();

    /* move only first found vertex in bbox */
    ret = Vedit_move_vertex(display->mapInfo, BgMap, nbgmaps, 
			    display->selected.ids,
			    point, thresh_coords, thresh_snap,
			    move_x, move_y, move_z,
			    1, snap);

    if (ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }
    
    if (ret > 0 && settings.breakLines) {
	BreakLineAtIntersection(Vect_get_num_lines(display->mapInfo), NULL, changeset);
    }

    if (BgMap && BgMap[0]) {
	Vect_close(BgMap[0]);
    }

    Vect_destroy_line_struct(point);

    return nlines + 1; // feature is write at the end of the file
}

/**
   \brief Add or remove vertex

   Shape of line/boundary is not changed when adding new vertex.
   
   \param add add or remove vertex?
   \param x,y,z coordinates (z is used only if map is 3d
   \param thresh threshold value to identify vertex position

   \param id id of the new feature
   \param 0 nothing changed
   \param -1 error
*/
int Digit::ModifyLineVertex(int add, double x, double y, double z,
			    double thresh)
{
    int ret;
    int changeset, nlines;
    struct line_pnts *point;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    if (display->selected.ids->n_values != 1)
	return 0;

    point = Vect_new_line_struct();
    Vect_append_point(point, x, y, z);

    nlines = Vect_get_num_lines(display->mapInfo);

    changeset = AddActionsBefore();

    if (add) {
	ret = Vedit_add_vertex(display->mapInfo, display->selected.ids,
			       point, thresh);
    }
    else {
	ret = Vedit_remove_vertex(display->mapInfo, display->selected.ids,
				  point, thresh);
    }

    if (ret > 0) {
	AddActionsAfter(changeset, nlines);
    }
    else {
	changesets.erase(changeset);
    }
    
    if (!add && ret > 0 && settings.breakLines) {
	BreakLineAtIntersection(Vect_get_num_lines(display->mapInfo), NULL, changeset);
    }

    Vect_destroy_line_struct(point);

    return nlines + 1; // feature is write at the end of the file
}
