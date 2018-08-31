
/****************************************************************
 *
 * MODULE:     v.edit
 *
 * PURPOSE:    Editing vector map.
 *
 * AUTHOR(S):  GRASS Development Team
 *             Wolf Bergenheim, Jachym Cepicky, Martin Landa
 *
 * COPYRIGHT:  (C) 2006-2008 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 * TODO:       3D support
 ****************************************************************/

#include "global.h"

/**
   \brief Global snapping function based on snapping library function.

   \param[in] Map vector map
   \param[in] List list of lines to be snapped
   \param[in] thresh threshold distance for snapping

   \return 1
 */
int snap_lines(struct Map_info *Map, struct ilist *List, double thresh)
{
    if (G_verbose() > G_verbose_min())
	G_important_message(SEP);

    Vect_snap_lines_list(Map, List, thresh, NULL);

    if (G_verbose() > G_verbose_min())
	G_important_message(SEP);

    return 1;
}

/**
   \brief Snap two selected lines

   \param[in] Map vector map
   \param[in] line1 reference line
   \param[in] line2 line to be snapped (to be modified)
   \param[in] thresh threshold distance for snapping (-1 for no limit)

   \return id of snapped line
   \return 0 lines not snapped
   \return -1 on error
*/
int snap_line2(struct Map_info *Map, int line1, int line2, double thresh)
{
    struct line_pnts *Points1, *Points2;
    struct line_cats *Cats2;
    int type2;
    int newline;
    double mindist;
    int mindistidx;

    Points1 = Vect_new_line_struct();
    Points2 = Vect_new_line_struct();
    Cats2 = Vect_new_cats_struct();

    Vect_read_line(Map, Points1, NULL, line1);
    type2 = Vect_read_line(Map, Points2, Cats2, line2);

    /* find mininal distance and its indexes */
    mindist = Vedit_get_min_distance(Points1, Points2, 0,	/* TODO 3D */
				     &mindistidx);

    if (thresh > 0.0 && mindist > thresh) {
	Vect_destroy_line_struct(Points1);
	Vect_destroy_line_struct(Points2);
	Vect_destroy_cats_struct(Cats2);
	return 0;
    }

    switch (mindistidx) {
    case 0:
	Points2->x[0] = Points1->x[0];
	Points2->y[0] = Points1->y[0];
	Points2->z[0] = Points1->z[0];
	break;
    case 1:
	Points2->x[Points2->n_points - 1] = Points1->x[0];
	Points2->y[Points2->n_points - 1] = Points1->y[0];
	Points2->z[Points2->n_points - 1] = Points1->z[0];
	break;
    case 2:
	Points2->x[0] = Points1->x[Points1->n_points - 1];
	Points2->y[0] = Points1->y[Points1->n_points - 1];
	Points2->z[0] = Points1->z[Points1->n_points - 1];
	break;
    case 3:
	Points2->x[Points2->n_points - 1] = Points1->x[Points1->n_points - 1];
	Points2->y[Points2->n_points - 1] = Points1->y[Points1->n_points - 1];
	Points2->z[Points2->n_points - 1] = Points1->z[Points1->n_points - 1];
	break;
    default:
	break;
    }

    newline = Vect_rewrite_line(Map, line2, type2, Points2, Cats2);
    if (newline < 0) {
	G_warning(_("Unable to rewrite line %d"), line2);
	return -1;
    }

    /*
       G_message(_("Line %d snapped to line %d"),
       line2, line1);
     */
    Vect_destroy_line_struct(Points1);
    Vect_destroy_line_struct(Points2);
    Vect_destroy_cats_struct(Cats2);

    return newline;
}
