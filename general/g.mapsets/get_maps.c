#include <stdio.h>
#include <grass/gis.h>
#include "local_proto.h"

int get_available_mapsets(void)
{
    char **ms;
    int i;

    ms = G_available_mapsets();
    for (nmapsets = 0; ms[nmapsets]; nmapsets++);

    mapset_name = (char **)G_malloc(nmapsets*sizeof(char *));
    for(i = 0; i < nmapsets; i++)
	mapset_name[i] = G_store(ms[i]);

    return 0;
}
