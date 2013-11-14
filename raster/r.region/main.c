
/***************************************************************************
 *
 * MODULE:	r.region (commandline)
 * AUTHOR(S):	Glynn Clements
 *		based upon g.region
 * PURPOSE:	Set the boundary definitions for a raster map.
 * 
 * COPYRIGHT:	(C) 2002 by the GRASS Development Team
 *
 *		This program is free software under the GPL (>=v2)
 *		Read the file COPYING that comes with GRASS for details.
 ****************************************************************************
 */

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static int nsew(const char *, const char *, const char *, const char *);
static void die(struct Option *);


int main(int argc, char *argv[])
{
    int i;
    double x;
    struct Cell_head cellhd, window;
    const char *value;
    const char *name;

    struct GModule *module;
    struct
    {
	struct Flag *dflt, *cur;
    } flag;
    struct
    {
	struct Option
	    *map,
	    *north, *south, *east, *west,
	    *raster, *vect, *region, *view, *align;
    } parm;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("metadata"));
    module->description =
	_("Sets the boundary definitions for a raster map.");

    /* flags */

    flag.cur = G_define_flag();
    flag.cur->key = 'c';
    flag.cur->description = _("Set from current region");
    flag.cur->guisection = _("Existing");

    flag.dflt = G_define_flag();
    flag.dflt->key = 'd';
    flag.dflt->description = _("Set from default region");
    flag.dflt->guisection = _("Existing");

    /* parameters */

    parm.map = G_define_standard_option(G_OPT_R_MAP);
    parm.map->description = _("Name of raster map to change");

    parm.region = G_define_option();
    parm.region->key = "region";
    parm.region->key_desc = "name";
    parm.region->required = NO;
    parm.region->multiple = NO;
    parm.region->type = TYPE_STRING;
    parm.region->description = _("Set region from named region");
    parm.region->gisprompt = "old,windows,region";
    parm.region->guisection = _("Existing");
    
    parm.raster = G_define_standard_option(G_OPT_R_MAP);
    parm.raster->key = "raster";
    parm.raster->required = NO;
    parm.raster->multiple = NO;
    parm.raster->description = _("Set region to match this raster map");
    parm.raster->guisection = _("Existing");

    parm.vect = G_define_standard_option(G_OPT_V_MAP);
    parm.vect->key = "vector";
    parm.vect->required = NO;
    parm.vect->multiple = NO;
    parm.vect->description = _("Set region to match this vector map");
    parm.vect->guisection = _("Existing");

    parm.view = G_define_option();
    parm.view->key = "3dview";
    parm.view->key_desc = "name";
    parm.view->required = NO;
    parm.view->multiple = NO;
    parm.view->type = TYPE_STRING;
    parm.view->description = _("Set region to match this 3dview file");
    parm.view->gisprompt = "old,3d.view,3d view";
    parm.view->guisection = _("Existing");

    parm.north = G_define_option();
    parm.north->key = "n";
    parm.north->key_desc = "value";
    parm.north->required = NO;
    parm.north->multiple = NO;
    parm.north->type = TYPE_STRING;
    parm.north->description = _("Value for the northern edge");
    parm.north->guisection = _("Bounds");

    parm.south = G_define_option();
    parm.south->key = "s";
    parm.south->key_desc = "value";
    parm.south->required = NO;
    parm.south->multiple = NO;
    parm.south->type = TYPE_STRING;
    parm.south->description = _("Value for the southern edge");
    parm.south->guisection = _("Bounds");

    parm.east = G_define_option();
    parm.east->key = "e";
    parm.east->key_desc = "value";
    parm.east->required = NO;
    parm.east->multiple = NO;
    parm.east->type = TYPE_STRING;
    parm.east->description = _("Value for the eastern edge");
    parm.east->guisection = _("Bounds");

    parm.west = G_define_option();
    parm.west->key = "w";
    parm.west->key_desc = "value";
    parm.west->required = NO;
    parm.west->multiple = NO;
    parm.west->type = TYPE_STRING;
    parm.west->description = _("Value for the western edge");
    parm.west->guisection = _("Bounds");
    
    parm.align = G_define_standard_option(G_OPT_R_MAP);
    parm.align->key = "align";
    parm.align->required = NO;
    parm.align->multiple = NO;
    parm.align->description = _("Raster map to align to");
    parm.align->guisection = _("Existing");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_window(&window);

    name = parm.map->answer;

    Rast_get_cellhd(name, G_mapset(), &cellhd);

    window = cellhd;

    if (flag.dflt->answer)
	G_get_default_window(&window);

    if (flag.cur->answer)
	G_get_window(&window);

    if ((name = parm.region->answer))	/* region= */
	G__get_window(&window, "windows", name, "");

    if ((name = parm.view->answer)) {	/* 3dview= */
	struct G_3dview v;
	FILE *fp;
	int ret;

	G_3dview_warning(0);	/* suppress boundary mismatch warning */

	fp = G_fopen_old("3d.view", name, "");
	if (!fp)
	    G_fatal_error(_("Unable to open 3dview file <%s>"), name);

	ret = G_get_3dview(name, "", &v);
	if (ret < 0)
	    G_fatal_error(_("Unable to read 3dview file <%s>"), name);
	if (ret == 0)
	    G_fatal_error(_("Old 3dview file. Region <%s> not found"), name);


	window.north = v.vwin.north;
	window.south = v.vwin.south;
	window.west = v.vwin.west;
	window.east = v.vwin.east;

	fclose(fp);

    }

    if ((name = parm.raster->answer)) {	/* raster= */
	Rast_get_cellhd(name, "", &window);
    }

    if ((name = parm.vect->answer)) {	/* vect= */
	struct Map_info Map;
	struct bound_box box;

	Vect_set_open_level(1);
	if (Vect_open_old(&Map, name, "") != 1)
	    G_fatal_error(_("Unable to open vector map <%s>"), name);

	Vect_get_map_box(&Map, &box);
	window.north = box.N;
	window.south = box.S;
	window.west = box.W;
	window.east = box.E;

	Rast_align_window(&window, &cellhd);

	Vect_close(&Map);
    }

    if ((value = parm.north->answer)) {	/* n= */
	if ((i = nsew(value, "n+", "n-", "s+"))) {
	    if (!G_scan_resolution(value + 2, &x, window.proj))
		die(parm.north);
	    switch (i) {
	    case 1:
		window.north += x;
		break;
	    case 2:
		window.north -= x;
		break;
	    case 3:
		window.north = window.south + x;
		break;
	    }
	}
	else if (G_scan_northing(value, &x, window.proj))
	    window.north = x;
	else
	    die(parm.north);
    }

    if ((value = parm.south->answer)) {	/* s= */
	if ((i = nsew(value, "s+", "s-", "n-"))) {
	    if (!G_scan_resolution(value + 2, &x, window.proj))
		die(parm.south);
	    switch (i) {
	    case 1:
		window.south += x;
		break;
	    case 2:
		window.south -= x;
		break;
	    case 3:
		window.south = window.north - x;
		break;
	    }
	}
	else if (G_scan_northing(value, &x, window.proj))
	    window.south = x;
	else
	    die(parm.south);
    }

    if ((value = parm.east->answer)) {	/* e= */
	if ((i = nsew(value, "e+", "e-", "w+"))) {
	    if (!G_scan_resolution(value + 2, &x, window.proj))
		die(parm.east);
	    switch (i) {
	    case 1:
		window.east += x;
		break;
	    case 2:
		window.east -= x;
		break;
	    case 3:
		window.east = window.west + x;
		break;
	    }
	}
	else if (G_scan_easting(value, &x, window.proj))
	    window.east = x;
	else
	    die(parm.east);
    }

    if ((value = parm.west->answer)) {	/* w= */
	if ((i = nsew(value, "w+", "w-", "e-"))) {
	    if (!G_scan_resolution(value + 2, &x, window.proj))
		die(parm.west);
	    switch (i) {
	    case 1:
		window.west += x;
		break;
	    case 2:
		window.west -= x;
		break;
	    case 3:
		window.west = window.east - x;
		break;
	    }
	}
	else if (G_scan_easting(value, &x, window.proj))
	    window.west = x;
	else
	    die(parm.west);
    }

    if ((name = parm.align->answer)) {	/* align= */
	struct Cell_head temp_window;

	Rast_get_cellhd(name, "", &temp_window);

	Rast_align_window(&window, &temp_window);
    }

    window.rows = cellhd.rows;
    window.cols = cellhd.cols;

    G_adjust_Cell_head(&window, 1, 1);

    cellhd.north = window.north;
    cellhd.south = window.south;
    cellhd.east = window.east;
    cellhd.west = window.west;

    Rast_put_cellhd(parm.map->answer, &cellhd);

    G_done_msg(" ");

    return 0;
}

static void die(struct Option *parm)
{
    G_fatal_error("<%s=%s> ** illegal value **", parm->key, parm->answer);
}

static int nsew(const char *value, const char *a, const char *b,
		const char *c)
{
    if (strncmp(value, a, strlen(a)) == 0)
	return 1;
    if (strncmp(value, b, strlen(b)) == 0)
	return 2;
    if (strncmp(value, c, strlen(c)) == 0)
	return 3;
    return 0;
}

