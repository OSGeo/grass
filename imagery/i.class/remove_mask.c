#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


int remove_mask(void)
{
    static char *elements[] = { "cell", "cellhd", "cats", "colr",
	"colr2", "hist", "cell_misc", ""
    };
    int i = 0;

    while (strcmp(elements[i], "") != 0)
	if (G_remove(elements[i++], "MASK") < 0)
	    G_fatal_error(_("Error while removing the old MASK cell map."));

    return 0;
}
