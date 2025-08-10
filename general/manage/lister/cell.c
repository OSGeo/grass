#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>

static int lister(const char *, const char *, char *);

int main(int argc, char *argv[])
{
    G_gisinit(argv[0]);

    if (argc == 1)
        G_list_element("cell", "raster", "", lister);
    else {
        int i;

        for (i = 1; i < argc; ++i)
            G_list_element("cell", "raster", argv[i], lister);
    }

    exit(0);
}

static int lister(const char *name, const char *mapset, char *title)
{
    *title = 0;
    if (*name)
        strcpy(title, Rast_get_cell_title(name, mapset));

    return 0;
}
