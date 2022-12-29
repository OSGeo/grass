#include <grass/gis.h>
#include <grass/raster.h>
int make_history(char *name, char *group, char *subgroup, char *sigfile)
{
    struct History hist;

    if (Rast_read_history(name, G_mapset(), &hist) >= 0) {
	Rast_format_history(&hist, HIST_DATSRC_1, "Group/subgroup: %s/%s", group, subgroup);
	Rast_format_history(&hist, HIST_DATSRC_2, "Signature file: %s", sigfile);
	Rast_write_history(name, &hist);
    }

    return 0;
}
