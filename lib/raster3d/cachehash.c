#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/raster3d.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/
#ifndef GRASS_RASTER3D_H
typedef struct
{

    int nofNames;
    int *index;
    char *active;
    int lastName;
    int lastIndex;
    int lastIndexActive;

} Rast3d_cache_hash;
#endif

/*---------------------------------------------------------------------------*/

void Rast3d_cache_hash_reset(Rast3d_cache_hash * h)
{
    int i;

    for (i = 0; i < h->nofNames; i++)
	h->active[i] = 0;

    h->lastIndexActive = 0;
}

/*---------------------------------------------------------------------------*/

void Rast3d_cache_hash_dispose(Rast3d_cache_hash * h)
{
    if (h == NULL)
	return;

    if (h->index != NULL)
	Rast3d_free(h->index);
    if (h->active != NULL)
	Rast3d_free(h->active);
    Rast3d_free(h);
}

/*---------------------------------------------------------------------------*/

void *Rast3d_cache_hash_new(int nofNames)
{
    Rast3d_cache_hash *tmp;

    tmp = (Rast3d_cache_hash *)Rast3d_malloc(sizeof(Rast3d_cache_hash));
    if (tmp == NULL) {
	Rast3d_error("Rast3d_cache_hash_new: error in Rast3d_malloc");
	return (void *)NULL;
    }
    
    tmp->nofNames = nofNames;
    tmp->index = (int*) Rast3d_malloc(tmp->nofNames * sizeof(int));
    tmp->active = (char*) Rast3d_malloc(tmp->nofNames * sizeof(char));
    if ((tmp->index == NULL) || (tmp->active == NULL)) {
	Rast3d_cache_hash_dispose(tmp);
	Rast3d_error("Rast3d_cache_hash_new: error in Rast3d_malloc");
	return (void *)NULL;
    }

    Rast3d_cache_hash_reset(tmp);

    return tmp;
}

/*---------------------------------------------------------------------------*/

void Rast3d_cache_hash_remove_name(Rast3d_cache_hash * h, int name)
{
    if (name >= h->nofNames)
	Rast3d_fatal_error("Rast3d_cache_hash_remove_name: name %i out of range", name);

    if (h->active[name] == 0)
	Rast3d_fatal_error("Rast3d_cache_hash_remove_name: name %i not in hashtable", name);

    h->active[name] = 0;
    if (name == h->lastName)
	h->lastIndexActive = 0;
}

/*---------------------------------------------------------------------------*/

void Rast3d_cache_hash_load_name(Rast3d_cache_hash * h, int name, int index)
{
    if (name >= h->nofNames)
	Rast3d_fatal_error("Rast3d_cache_hash_load_name: name out of range");

    if (h->active[name] != 0)
	Rast3d_fatal_error("Rast3d_cache_hash_load_name: name already in hashtable");

    h->index[name] = index;
    h->active[name] = 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_hash_name2index(Rast3d_cache_hash * h, int name)
{
    int index;

    if (h->lastIndexActive)
	if (h->lastName == name)
	    return h->lastIndex;

    if (!h->active[name])
	return -1;

    index = h->index[name];

    h->lastName = name;
    h->lastIndex = index;
    h->lastIndexActive = 1;

    return index;
}
