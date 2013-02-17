#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"

static int cmp(const void *, const void *);

char** get_available_mapsets(int *nmapsets)
{
    char **ms, **mapset_name;
    int i, n;

    ms = G_available_mapsets();
    for (n = 0; ms[n]; n++);

    mapset_name = (char **)G_malloc(n * sizeof(char *));
    for(i = 0; i < n; i++)
        mapset_name[i] = G_store(ms[i]);

    /* sort mapsets */
    qsort(mapset_name, n, sizeof(char *), cmp);

    *nmapsets = n;
    
    return mapset_name;
}

int cmp(const void *a, const void *b) 
{
    return (strcmp(*(char **)a, *(char **)b));
}

const char *substitute_mapset(const char *mapset)
{
    if (strcmp(mapset, ".") == 0)
        return G_mapset();

    return mapset;
}
