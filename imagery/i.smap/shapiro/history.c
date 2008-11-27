#include <grass/gis.h>

void make_history(const char *name, const char *group, const char *subgroup, const char *sigfile)
{
    struct History hist;

    if (G_read_history(name, G_mapset(), &hist) >= 0) {
	sprintf(hist.datsrc_1, "Group/subgroup: %s/%s", group, subgroup);
	sprintf(hist.datsrc_2, "Sigset file: %s", sigfile);
	G_write_history(name, &hist);
    }
}
