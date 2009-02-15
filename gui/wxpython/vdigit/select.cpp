/**
   \file vdigit/select.cpp

   \brief wxvdigit - Select lines (by query)

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
   \brief Select features by query (based on geometry)

   Currently supported:
    - QUERY_LENGTH, select all lines longer then threshold (or shorter if threshold is negative)
    - QUERY_DANGLE, select all dangles then threshold (or shorter if threshold is negative)

   \todo Rewrite dangle part to use Vector library functionality
   \todo 3D

   \param x1,y1,z1,x2,y2,z2 bounding box 
   \param query query (length, dangle, ...)
   \param thresh threshold value (< 0 for 'shorter', > 0 for 'longer')
   \param type feature type
   \param box query features in bounding box

   \return list of selected features
*/

std::vector<int> Digit::SelectLinesByQuery(double x1, double y1, double z1,
					   double x2, double y2, double z2, bool box,
					   int query, int type, double thresh)
{
    int layer;
    std::vector<int> ids;
    struct ilist *List;
    struct line_pnts *bbox;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return ids;
    }

    layer = 1; // TODO
    bbox  = NULL;

    List = Vect_new_list();

    if (box) {
	bbox = Vect_new_line_struct();
	
	Vect_append_point(bbox, x1, y1, z1);
	Vect_append_point(bbox, x2, y1, z2);
	Vect_append_point(bbox, x2, y2, z1);
	Vect_append_point(bbox, x1, y2, z2);
	Vect_append_point(bbox, x1, y1, z1);
	
	Vect_select_lines_by_polygon (display->mapInfo, bbox, 0, NULL, type, List);
	if (List->n_values == 0) {
	    return ids;
	}
    }
    
    G_debug(3, "wxDigit.SelectLinesByQuery(): lines=%d", List->n_values);

    Vedit_select_by_query(display->mapInfo,
			  type, layer, thresh, query,
			  List);
    
    ids = display->ListToVector(List); // TODO

    G_debug(3, "wxDigit.SelectLinesByQuery(): lines=%d", List->n_values);

    Vect_destroy_list(List);
    if (bbox) {
	Vect_destroy_line_struct(bbox);
    }

    return ids;
}
