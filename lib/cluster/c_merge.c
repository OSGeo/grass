/*!
  \file cluster/c_merge.c
  
  \brief Cluster library - Merge
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/cluster.h>

/*!
  \brief ?
  
  \param C pointer to Cluster structure

  \return 0
*/
int I_cluster_merge(struct Cluster *C)
{
    int band, p;
    int c1, c2;

    c1 = C->merge1;
    c2 = C->merge2;

    for (p = 0; p < C->npoints; p++)
	if (C->class[p] == c2)
	    C->class[p] = c1;
    C->count[c1] += C->count[c2];
    C->count[c2] = 0;
    for (band = 0; band < C->nbands; band++) {
	C->sum[band][c1] += C->sum[band][c2];
	C->sum[band][c2] = 0;
    }

    return 0;
}
