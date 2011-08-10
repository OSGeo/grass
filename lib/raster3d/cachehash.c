#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/G3d.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/
#ifndef GRASS_G3D_H
typedef struct
{

    int nofNames;
    int *index;
    char *active;
    int lastName;
    int lastIndex;
    int lastIndexActive;

} G3d_cache_hash;
#endif

/*---------------------------------------------------------------------------*/

void G3d_cache_hash_reset(G3d_cache_hash * h)
{
    int i;

    for (i = 0; i < h->nofNames; i++)
	h->active[i] = 0;

    h->lastIndexActive = 0;
}

/*---------------------------------------------------------------------------*/

void G3d_cache_hash_dispose(G3d_cache_hash * h)
{
    if (h == NULL)
	return;

    if (h->index != NULL)
	G3d_free(h->index);
    if (h->active != NULL)
	G3d_free(h->active);
    G3d_free(h);
}

/*---------------------------------------------------------------------------*/

void *G3d_cache_hash_new(int nofNames)
{
    G3d_cache_hash *tmp;

    tmp = G3d_malloc(sizeof(G3d_cache_hash));
    if (tmp == NULL) {
	G3d_error("G3d_cache_hash_new: error in G3d_malloc");
	return (void *)NULL;
    }

    tmp->nofNames = nofNames;
    tmp->index = G3d_malloc(sizeof(int) * tmp->nofNames);
    tmp->active = G3d_malloc(sizeof(char) * tmp->nofNames);
    if ((tmp->index == NULL) || (tmp->active == NULL)) {
	G3d_cache_hash_dispose(tmp);
	G3d_error("G3d_cache_hash_new: error in G3d_malloc");
	return (void *)NULL;
    }

    G3d_cache_hash_reset(tmp);

    return tmp;
}

/*---------------------------------------------------------------------------*/

void G3d_cache_hash_remove_name(G3d_cache_hash * h, int name)
{
    if (name >= h->nofNames)
	G3d_fatalError("G3d_cache_hash_remove_name: name out of range");

    if (h->active[name] == 0)
	G3d_fatalError("G3d_cache_hash_remove_name: name not in hashtable");

    h->active[name] = 0;
    if (name == h->lastName)
	h->lastIndexActive = 0;
}

/*---------------------------------------------------------------------------*/

void G3d_cache_hash_load_name(G3d_cache_hash * h, int name, int index)
{
    if (name >= h->nofNames)
	G3d_fatalError("G3d_cache_hash_load_name: name out of range");

    if (h->active[name] != 0)
	G3d_fatalError("G3d_cache_hash_load_name: name already in hashtable");

    h->index[name] = index;
    h->active[name] = 1;
}

/*---------------------------------------------------------------------------*/

int G3d_cache_hash_name2index(G3d_cache_hash * h, int name)
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
