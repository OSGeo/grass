/*!
  \file cluster/c_means.c
  
  \brief Cluster library - Means value
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <math.h>
#include <grass/cluster.h>

/*!
  \brief Calculate means value

  \param C pointer to Cluster structure

  \return 0
*/
int I_cluster_means(struct Cluster *C)
{
    int band;
    int class;
    double m, v;		/* m=mean, v=variance then std dev */
    double s;

    G_debug(3, "I_cluster_means(nbands=%d,nclasses=%d)",
	    C->nbands, C->nclasses);
    
    for (band = 0; band < C->nbands; band++) {
	s = C->band_sum[band];
	m = s / C->npoints;
	v = C->band_sum2[band] - s * m;
	v = sqrt(v / (C->npoints - 1));
	for (class = 0; class < C->nclasses; class++)
	    C->mean[band][class] = m;
	if (C->nclasses > 1)
	    for (class = 0; class < C->nclasses; class++)
		C->mean[band][class] +=
		    ((2.0 * class) / (C->nclasses - 1) - 1.0) * v;
    }

    return 0;
}
