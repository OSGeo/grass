#include "global.h"


int run(char *cmd)
{
    int retval;


    if (G_system(cmd)) {
	G_warning("Subprocess failed");
	retval = 1;
    }
    else {
	G_verbose_message("Subprocess complete.");
	retval = 0;
    }

    return retval;
}


void gregion(void)
{
    char buf[GPATH_MAX];
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
	sprintf(buf, "g.region rast=%s --quiet", hdmap);
	G_verbose_message("%s ...", buf);

	if (run(buf))
	    exit(1);
    }
}


void depressionless(void)
{
    char buf[GPATH_MAX];

    sprintf(buf, "r.fill.dir input=%s elev=%s dir=%s type=grass --quiet",
	    map.elev, map.fill, map.dir);
    G_verbose_message("%s ...", buf);

    if (run(buf))
	exit(1);

    map.elev = map.fill;

    return;
}


void basin_elevation(void)
{
    char buf[GPATH_MAX];

    sprintf(buf, "r.mapcalc expression=\"%s = if(%s == 0 || isnull(%s), null(), %s)\" --quiet",
	    map.belev, map.basin, map.basin, map.elev);
    G_verbose_message("%s ...", buf);

    if (run(buf))
	exit(1);

    return;
}


void top_index(void)
{
    char buf[GPATH_MAX];
    if (map.belev) {
	sprintf(buf, "r.topidx input=%s output=%s --quiet",
		map.belev, map.topidx);
	G_verbose_message("%s ...", buf);

	if (run(buf))
	    exit(1);
    }

    if (map.topidx) {
	sprintf(buf, "r.stats -Anc input=%s nsteps=%d output=\"%s\"",
		map.topidx, misc.nidxclass, file.idxstats);
	G_verbose_message("%s ...", buf);

	if (run(buf))
	    exit(1);
    }

    return;
}
