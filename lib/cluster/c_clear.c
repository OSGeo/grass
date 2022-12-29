/*!
  \file cluster/c_clear.c
  
  \brief Cluster library - Clear structures
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/cluster.h>

/*!
  \brief Clear Cluster structure

  \param C pointer to Cluster structure

  \return 0
*/
int I_cluster_clear(struct Cluster *C)
{
    C->points = NULL;
    C->band_sum = NULL;
    C->band_sum2 = NULL;
    C->class = NULL;
    C->reclass = NULL;
    C->count = NULL;
    C->countdiff = NULL;
    C->sum = NULL;
    C->sumdiff = NULL;
    C->sum2 = NULL;
    C->mean = NULL;
    C->nbands = 0;
    I_init_signatures(&C->S, 0);

    return 0;
}
