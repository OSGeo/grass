#include <grass/gis.h>
#include <grass/raster.h>

void make_history(const char *name, const char *group, const char *subgroup, const char *sigfile)
{
    struct History hist;

    if (Rast_read_history(name, G_mapset(), &hist) >= 0) {
	Rast_format_history(&hist, HIST_DATSRC_1, "Group/subgroup: %s/%s", group, subgroup);
	Rast_format_history(&hist, HIST_DATSRC_2, "Sigset file: %s", sigfile);
	Rast_write_history(name, &hist);
    }
}
