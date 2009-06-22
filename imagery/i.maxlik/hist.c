#include <grass/gis.h>
#include <grass/raster.h>
int make_history(char *name, char *group, char *subgroup, char *sigfile)
{
    struct History hist;

    if (Rast_read_history(name, G_mapset(), &hist) >= 0) {
	sprintf(hist.datsrc_1, "Group/subgroup: %s/%s", group, subgroup);
	sprintf(hist.datsrc_2, "Signature file: %s", sigfile);
	Rast_write_history(name, &hist);
    }

    return 0;
}
