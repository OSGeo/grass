/*!
  \file cluster/c_execmem.c
  
  \brief Cluster library - Allocate cluster
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/cluster.h>

/*!
  \brief Allocate Cluster structure

  \param C pointer to Cluster structure

  \return 1 on success
  \return 0 on error
*/
int I_cluster_exec_allocate(struct Cluster *C)
{
    G_debug(1, "I_cluster_exec_allocate(npoints=%d,nclasses=%d,nbands=%d)",
	    C->npoints, C->nclasses, C->nbands);
    
    C->class = I_alloc_int(C->npoints);
    C->reclass = I_alloc_int(C->nclasses);
    C->count = I_alloc_int(C->nclasses);
    C->countdiff = I_alloc_int(C->nclasses);
    C->sum = I_alloc_double2(C->nbands, C->nclasses);
    C->sumdiff = I_alloc_double2(C->nbands, C->nclasses);
    C->sum2 = I_alloc_double2(C->nbands, C->nclasses);
    C->mean = I_alloc_double2(C->nbands, C->nclasses);
    if (C->class == NULL || C->reclass == NULL ||
	C->sum == NULL || C->sumdiff == NULL ||
	C->count == NULL || C->countdiff == NULL ||
	C->sum2 == NULL || C->mean == NULL) {
	I_cluster_exec_free(C);
	return 0;
    }
    return 1;
}

/*!
  \brief Free allocated Cluster structure

  \param C pointer to Cluster structure

  \return 0
*/
int I_cluster_exec_free(struct Cluster *C)
{
    I_free(C->class);
    I_free(C->reclass);
    I_free(C->count);
    I_free(C->countdiff);
    I_free_double2(C->sum2);
    I_free_double2(C->sum);
    I_free_double2(C->sumdiff);
    I_free_double2(C->mean);

    C->class = NULL;
    C->count = NULL;
    C->countdiff = NULL;
    C->sum = NULL;
    C->sumdiff = NULL;
    C->sum2 = NULL;
    C->mean = NULL;

    return 0;
}
