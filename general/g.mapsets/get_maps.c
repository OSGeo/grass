#include <stdio.h>
#include "externs.h"
#include <grass/gis.h>

int get_available_mapsets(void)
{
    char **ms;

    ms = G_available_mapsets();

    for (nmapsets = 0; ms[nmapsets]; nmapsets++)
	mapset_name[nmapsets] = G_store(ms[nmapsets]);

    return 0;
}
