/*!
  \file lib/vector/vedit/distance.c

  \brief Vedit library - distance calculation
  
  (C) 2007-2008 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/vedit.h>

/*!
  \brief Calculate distances between two lines
  
  \todo LL projection
  
  \param Points1 first line geometry
  \param Points2 second line geometry
  \param with_z WITH_Z for 3D data
  \param[out] mindistidx index of minimal distance
  
  \return minimal distance between two lines (their nodes)
*/
double Vedit_get_min_distance(struct line_pnts *Points1,
			      struct line_pnts *Points2, int with_z,
			      int *mindistidx)
{
    unsigned int i;
    double distances[4];

    /*
       distances[0] = first-first
       distances[1] = first-last
       distances[2] = last-first
       distances[3] = last-last
     */

    distances[0] =
	Vect_points_distance(Points1->x[0], Points1->y[0], Points1->z[0],
			     Points2->x[0], Points2->y[0], Points2->z[0],
			     with_z);

    distances[1] =
	Vect_points_distance(Points1->x[0], Points1->y[0], Points1->z[0],
			     Points2->x[Points2->n_points - 1],
			     Points2->y[Points2->n_points - 1],
			     Points2->z[Points2->n_points - 1], with_z);

    distances[2] = Vect_points_distance(Points1->x[Points1->n_points - 1],
					Points1->y[Points1->n_points - 1],
					Points1->z[Points1->n_points - 1],
					Points2->x[0], Points2->y[0],
					Points2->z[0], with_z);

    distances[3] = Vect_points_distance(Points1->x[Points1->n_points - 1],
					Points1->y[Points1->n_points - 1],
					Points1->z[Points1->n_points - 1],
					Points2->x[Points2->n_points - 1],
					Points2->y[Points2->n_points - 1],
					Points2->z[Points2->n_points - 1],
					with_z);

    /* find the minimal distance between first or last point of both lines */
    *mindistidx = 0;
    for (i = 0; i < sizeof(distances) / sizeof(double); i++) {
	if (distances[i] >= 0.0 && distances[i] < distances[*mindistidx])
	    *mindistidx = i;
    }

    G_debug(3, "Vedit_get_min_distance(): dists=%f,%f,%f,%f",
	    distances[0], distances[1], distances[2], distances[3]);

    return distances[*mindistidx];
}
