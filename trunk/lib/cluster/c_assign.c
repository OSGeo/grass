/*!
  \file cluster/c_assign.c
  
  \brief Cluster library - Assign cluster
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <math.h>
#include <grass/cluster.h>

/*!
  \brief Assign cluster

  \param C pointer to Cluster structure
  \param interrupted ?
  
  \return -1 on interrupted
  \return 0 on success
*/
int I_cluster_assign(struct Cluster *C, int *interrupted)
{
    int p, c;
    int class, band;
    double d, q;
    double dmin;

    G_debug(3, "I_cluster_assign(npoints=%d,nclasses=%d,nbands=%d)",
	    C->npoints, C->nclasses, C->nbands);
    
    for (p = 0; p < C->npoints; p++) {
	if (*interrupted)
	    return -1;

	dmin = HUGE_VAL;
	class = 0;
	for (c = 0; c < C->nclasses; c++) {
	    d = 0.0;
	    for (band = 0; band < C->nbands; band++) {
		q = C->points[band][p];
		q -= C->mean[band][c];
		d += q * q;
	    }
	    if (c == 0 || d < dmin) {
		class = c;
		dmin = d;
	    }
	}
	C->class[p] = class;
	C->count[class]++;
	for (band = 0; band < C->nbands; band++)
	    C->sum[band][class] += C->points[band][p];
    }

    return 0;
}
