/**
   \file vertex.cpp

   \brief Vertex manipulation

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008 by The GRASS development team

   \author Martin Landa <landa.martin gmail.com>

   \date 2008 
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

   \param 1 vertex moved
   \param 0 nothing changed
   \param -1 error
*/
int Digit::MoveVertex(double x, double y, double z,
		      double move_x, double move_y, double move_z,
		      const char *bgmap, int snap,
		      double thresh_coords, double thresh_snap) {

    int ret;
    struct line_pnts *point;
    struct Map_info **BgMap; /* backgroud vector maps */
    int nbgmaps;             /* number of registrated background maps */

    if (!display->mapInfo) {
	DisplayMsg();
	return -1;
    }

    if (display->selected.values->n_values != 1)
	return 0;

    BgMap = NULL;
    nbgmaps = 0;
    if (bgmap && strlen(bgmap) > 0) {
	BgMap = OpenBackgroundVectorMap(bgmap);
	if (!BgMap) {
	    BackgroundMapMsg(bgmap);
	    return -1;
	}
	else {
	    nbgmaps = 1;
	}
    }

    point = Vect_new_line_struct();
    Vect_append_point(point, x, y, z);

    /* register changeset */
    AddActionToChangeset(changesets.size(), REWRITE, display->selected.values->value[0]);

    /* move only first found vertex in bbox */
    ret = Vedit_move_vertex(display->mapInfo, BgMap, nbgmaps, 
			    display->selected.values,
			    point, thresh_coords, thresh_snap,
			    move_x, move_y, move_z,
			    1, snap); 

    if (ret > 0) {
	/* updates feature id (id is changed since line has been rewriten) */
	changesets[changesets.size()-1][0].line = Vect_get_num_lines(display->mapInfo);
    }
    else {
	changesets.erase(changesets.size()-1);
    }

    if (BgMap && BgMap[0]) {
	Vect_close(BgMap[0]);
    }

    Vect_destroy_line_struct(point);

    return ret;
}

/**
   \brief Add or remove vertex

   Shape of line/boundary is not changed when adding new vertex.
   
   \param add add or remove vertex?
   \param x,y,z coordinates (z is used only if map is 3d
   \param thresh threshold value to identify vertex position

   \param 1 vertex added/removed
   \param 0 nothing changed
   \param -1 error
*/
int Digit::ModifyLineVertex(int add, double x, double y, double z,
			    double thresh)
{
    int ret;
    struct line_pnts *point;

    if (!display->mapInfo) {
	DisplayMsg();
	return -1;
    }

    if (display->selected.values->n_values != 1)
	return 0;

    point = Vect_new_line_struct();
    Vect_append_point(point, x, y, z);

    /* register changeset */
    AddActionToChangeset(changesets.size(), REWRITE, display->selected.values->value[0]);

    if (add) {
	ret = Vedit_add_vertex(display->mapInfo, display->selected.values,
			       point, thresh);
    }
    else {
	ret = Vedit_remove_vertex(display->mapInfo, display->selected.values,
				  point, thresh);
    }

    if (ret > 0) {
	/* updates feature id (id is changed since line has been rewriten) */
	changesets[changesets.size()-1][0].line = Vect_get_num_lines(display->mapInfo);
    }
    else {
	changesets.erase(changesets.size()-1);
    }

    Vect_destroy_line_struct(point);

    return ret;
}
