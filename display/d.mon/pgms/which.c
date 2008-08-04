/* which_mon - show name of currently selected monitor */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>

int main(int argc, char *argv[])
{
    char *name;

    G_gisinit(argv[0]);
    if ((name = G__getenv("MONITOR")) == NULL)
	fprintf(stdout, "No monitor currently selected for output\n");
    else
	fprintf(stdout, "Currently selected monitor: %s\n", name);
    exit(EXIT_SUCCESS);
}
