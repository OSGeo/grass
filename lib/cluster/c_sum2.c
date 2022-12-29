/*!
  \file cluster/c_sum2.c
  
  \brief Cluster library - Sum of squares
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/cluster.h>

/*!
  \brief Compute sum of squares for each class

  \param C pointer to Cluster structure

  \return 0
*/
int I_cluster_sum2(struct Cluster *C)
{
    int p, band, class;
    double q;

    G_debug(3, "I_cluster_sum2(npoints=%d,nclasses=%d,nbands=%d)",
	    C->npoints, C->nclasses, C->nbands);
    
    for (class = 0; class < C->nclasses; class++)
	for (band = 0; band < C->nbands; band++)
	    C->sum2[band][class] = 0;

    for (p = 0; p < C->npoints; p++) {
	class = C->class[p];
	if (class < 0)
	    continue;
	for (band = 0; band < C->nbands; band++) {
	    q = C->points[band][p];
	    C->sum2[band][class] += q * q;
	}
    }

    return 0;
}
