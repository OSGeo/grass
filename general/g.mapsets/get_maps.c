#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"

static int cmp(const void *, const void *);

int get_available_mapsets(void)
{
    char **ms;
    int i;

    ms = G_available_mapsets();
    for (nmapsets = 0; ms[nmapsets]; nmapsets++);

    mapset_name = (char **)G_malloc(nmapsets*sizeof(char *));
    for(i = 0; i < nmapsets; i++)
	mapset_name[i] = G_store(ms[i]);

    /* sort mapsets */
    qsort(mapset_name, nmapsets, sizeof(char *), cmp);

    return 0;
}

static int cmp(const void *a, const void *b) 
{
    return (strcmp(*(char **)a, *(char **)b));
}
