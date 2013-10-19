#include "global.h"
#include <grass/spawn.h>

void gregion(void)
{
    char *hdmap;

    hdmap = NULL;
    if (!flg.input) {
	hdmap = map.elev;
    }
    else {
	if (map.belev)
	    hdmap = map.belev;
	else if (map.topidx)
	    hdmap = map.topidx;
    }

    if (hdmap) {
	char buf[GPATH_MAX];

	sprintf(buf, "rast=%s", hdmap);

	G_verbose_message("g.region %s ...", buf);

	if (G_spawn("g.region", "g.region", "--quiet", buf, NULL) != 0)
	    G_fatal_error("g.region failed");
    }
}


void depressionless(void)
{
    char input[GPATH_MAX];
    char elev[GPATH_MAX];
    char dir[GPATH_MAX];

    sprintf(input, "input=%s", map.elev);
    sprintf(elev, "output=%s", map.fill);
    sprintf(dir, "outdir=%s", map.dir);

    G_verbose_message("r.fill.dir %s %s %s", input, elev, dir);

    if (G_spawn("r.fill.dir", "r.fill.dir", "--quiet", input, elev, dir, NULL) != 0)
	G_fatal_error("r.fill.dir failed");

    map.elev = map.fill;
}


void basin_elevation(void)
{
    char buf[GPATH_MAX];

    sprintf(buf, "expression=%s = if(%s == 0 || isnull(%s), null(), %s)",
	    map.belev, map.basin, map.basin, map.elev);
    G_verbose_message("r.mapcalc \"%s\" ...", buf);

    if (G_spawn("r.mapcalc", "r.mapcalc", "--quiet", buf, NULL) != 0)
	G_fatal_error("r.mapcalc failed");
}


void top_index(void)
{
    char input[GPATH_MAX];
    char output[GPATH_MAX];
    char nsteps[32];

    if (map.belev) {
	sprintf(input, "input=%s", map.belev);
	sprintf(output, "output=%s", map.topidx);

	G_verbose_message("r.topidx %s %s ...", input, output);

	if (G_spawn("r.topidx", "r.topidx", "--quiet", input, output, NULL) != 0)
	    G_fatal_error("r.topidx failed");
    }

    if (map.topidx) {
	sprintf(input, "input=%s", map.topidx);
	sprintf(input, "nsteps=%d", misc.nidxclass);
	sprintf(output, "output=%s", file.idxstats);

	G_verbose_message("r.stats -Anc %s %s %s ...", input, nsteps, output);

	if (G_spawn("r.stats", "r.stats", "-Anc", input, nsteps, output, NULL) != 0)
	    G_fatal_error("r.stats failed");
    }
}

