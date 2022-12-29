/*!
  \file cluster/c_reclass.c
  
  \brief Cluster library - Reclass data
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/cluster.h>

/*!
  \brief Reclass data

  \param C pointer to Cluster structure
  \param minsize minimum class size

  \return 0 on success
  \return 1 no change
*/
int I_cluster_reclass(struct Cluster *C, int minsize)
{
    int band, c, hole, move, p;

    for (c = 0; c < C->nclasses; c++)
	C->reclass[c] = c;

    /* find first `empty' class */
    for (hole = 0; hole < C->nclasses; hole++)
	if (C->count[hole] < minsize)
	    break;

    /* if none, just return */
    if (hole >= C->nclasses)
	return 1;

    for (move = hole; move < C->nclasses; move++)
	if (C->count[move] >= minsize) {
	    C->reclass[move] = hole;
	    C->count[hole] = C->count[move];
	    for (band = 0; band < C->nbands; band++)
		C->sum[band][hole] = C->sum[band][move];
	    hole++;
	}
	else
	    C->reclass[move] = -1;	/* eliminate this class */

    for (p = 0; p < C->npoints; p++)
	C->class[p] = C->reclass[C->class[p]];
    C->nclasses = hole;

    return 0;
}
