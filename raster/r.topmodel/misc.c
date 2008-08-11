#include "global.h"


int run(char *cmd)
{
    int retval;


    if (G_system(cmd)) {
	G_warning("Failed");
	retval = 1;
    }
    else {
	G_message("OK");
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
	sprintf(buf, "%s/bin/g.region rast=%s > /dev/null", gisbase, hdmap);
	G_message("g.region rast=%s ... ", hdmap);

	if (run(buf))
	    exit(1);
    }
}


void depressionless(void)
{
    char buf[GPATH_MAX];
    sprintf(buf, "%s/bin/r.fill.dir "
	    "input=%s elev=%s dir=%s type=grass > /dev/null",
	    gisbase, map.elev, map.fill, map.dir);
    G_message("r.fill.dir input=%s elev=%s dir=%s type=grass ... ",
	      map.elev, map.fill, map.dir);

    if (run(buf))
	exit(1);

    map.elev = map.fill;


    return;
}


void basin_elevation(void)
{
    char buf[GPATH_MAX];
    sprintf(buf, "%s/bin/r.mapcalc "
	    "'%s = if(%s == 0 || isnull(%s), null(), %s)' > /dev/null",
	    gisbase, map.belev, map.basin, map.basin, map.elev);
    G_message("r.mapcalc '%s = if(%s == 0 || isnull(%s), null(), %s)'"
	      " ... ", map.belev, map.basin, map.basin, map.elev);

    if (run(buf))
	exit(1);


    return;
}


void top_index(void)
{
    char buf[GPATH_MAX];
    if (map.belev) {
	sprintf(buf, "%s/bin/r.topidx "
		"input=%s output=%s > /dev/null",
		gisbase, map.belev, map.topidx);
	G_message("r.topidx input=%s output=%s ... ", map.belev, map.topidx);

	if (run(buf))
	    exit(1);
    }

    if (map.topidx) {
	sprintf(buf, "%s/bin/r.stats -Anc "
		"input=%s nsteps=%d > %s",
		gisbase, map.topidx, misc.nidxclass, file.idxstats);
	G_message("r.stats -Anc input=%s nsteps=%d > %s ... ",
		  map.topidx, misc.nidxclass, file.idxstats);

	if (run(buf))
	    exit(1);
    }


    return;
}
