/*!
  \file cluster/c_nclasses.c
  
  \brief Cluster library - Number of classes
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/cluster.h>

/*!
  \brief Get number of classes

  \param C pointer to Cluster structure
  \param minsize minimum class size

  \return number of classes
*/
int I_cluster_nclasses(struct Cluster *C, int minsize)
{
    int i, n;

    n = 0;
    for (i = 0; i < C->nclasses; i++)
	if (C->count[i] >= minsize)
	    n++;
    return n;
}
