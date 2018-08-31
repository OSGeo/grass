/*!
  \file cluster/c_point.c
  
  \brief Cluster library - Add point 
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/raster.h>
#include <grass/cluster.h>

static int extend(struct Cluster *, int);
static int all_zero(struct Cluster *, int);

/*!
  \brief Adds the point x to the list of data points to be "clustered"
  
  The dimension of x must agree with the number of bands specified
  in the initializing call to I_cluster_begin()
  
  Note: if all values in x are zero, the point is rejected
  
  \return 0 ok
  \return -1 out of memory, point not added
  \return 1 all values are null, point not added
*/
int I_cluster_point(struct Cluster *C, DCELL * x)
{
    int band;

    /* reject points which contain nulls in one of the bands */
    for (band = 0; band < C->nbands; band++)
	if (Rast_is_d_null_value(&x[band]))
	    return 1;		/* fixed 11/99 Agus Carr */
    /*
       if (band >= C->nbands)
       return 1;
     */

    /* extend the arrays for each band, if necessary */
    if (!extend(C, 1))
	return -1;

    /* add the point to the points arrays */
    for (band = 0; band < C->nbands; band++) {
	register double z;

	/*       if(Rast_is_d_null_value(&x[band])) continue; */
	z = C->points[band][C->npoints] = x[band];
	C->band_sum[band] += z;
	C->band_sum2[band] += z * z;
    }
    C->npoints++;
    return 0;
}

/*!
  \brief Begin point set

  \param C pointer to Cluster structure
  \param n ?

  \return 0 on success
  \return -1 on error
*/
int I_cluster_begin_point_set(struct Cluster *C, int n)
{
    return extend(C, n) ? 0 : -1;
}

/*!
  \brief ?

  \param C pointer to Cluster structure
  \param x cell value
  \param band band number
  \param n ?

  \return 0 ok
  \return -1 out of memory, point not added
  \return 1 all values are null, point not added
*/  
int I_cluster_point_part(struct Cluster *C, DCELL x, int band, int n)
{
    DCELL tmp = x;

    if (Rast_is_d_null_value(&tmp))
	return 1;
    C->points[band][C->npoints + n] = x;
    C->band_sum[band] += x;
    C->band_sum2[band] += x * x;

    return 0;
}

/*!
  \brief ?

  \param C pointer to Cluster structure
  \param n ?

  \return number of points
*/
int I_cluster_end_point_set(struct Cluster *C, int n)
{
    int band;
    int cur, next;

    cur = C->npoints;
    n += C->npoints;
    for (next = cur; next < n; next++) {
	if (!all_zero(C, next)) {
	    if (cur != next)
		for (band = 0; band < C->nbands; band++)
		    C->points[band][cur] = C->points[band][next];
	    cur++;
	}
    }
    return C->npoints = cur;
}

static int all_zero(struct Cluster *C, int i)
{
    int band;

    for (band = 0; band < C->nbands; band++)
	if (C->points[band][i])
	    return 0;
    return 1;
}

static int extend(struct Cluster *C, int n)
{
    int band;

    while ((C->npoints + n) > C->np) {
	C->np += 128;
	for (band = 0; band < C->nbands; band++) {
	    C->points[band] =
		(DCELL *) I_realloc(C->points[band], C->np * sizeof(DCELL));
	    if (C->points[band] == NULL)
		return 0;
	}
    }
    return 1;
}
