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
#include <grass/Vect.h>
#include <grass/glocale.h>

static int nsew(const char *,const char *,const char *,const char *);
static void die(struct Option *);
static char *llinfo(const char *,const char *,int);


int main (int argc, char *argv[])
{
	int i;
	double x;
	struct Cell_head cellhd, window;
	char *value;
	char *name;
	char *mapset;
	char *err;
	int projection;

	struct GModule *module;
	struct {
		struct Flag
		*dflt, *cur;
	} flag;
	struct {
		struct Option
		*map,
		*north,*south,*east,*west,
		*raster, *vect, *region, *view,
		*align;
	} parm;

	G_gisinit (argv[0]);

	module = G_define_module();
	module->keywords = _("raster");
    module->description =
		_("Sets the boundary definitions for a raster map.");

	G_get_window(&window);

	projection = window.proj;

	/* flags */

	flag.cur = G_define_flag();
	flag.cur->key		= 'c';
	flag.cur->description	= _("Set from current region");

	flag.dflt = G_define_flag();
	flag.dflt->key		= 'd';
	flag.dflt->description	= _("Set from default region");

	/* parameters */

	parm.map = G_define_option();
	parm.map->key		= "map";
	parm.map->key_desc	= "name";
	parm.map->required	= YES;
	parm.map->multiple	= NO;
	parm.map->type		= TYPE_STRING;
	parm.map->gisprompt	= "old,cell,raster";
	parm.map->description	= _("Raster map to change");

	parm.region = G_define_option();
	parm.region->key	= "region";
	parm.region->key_desc	= "name";
	parm.region->required	= NO;
	parm.region->multiple	= NO;
	parm.region->type	= TYPE_STRING;
	parm.region->description= _("Set region from named region");

	parm.raster = G_define_option();
	parm.raster->key	= "raster";
	parm.raster->key_desc	= "name";
	parm.raster->required	= NO;
	parm.raster->multiple	= NO;
	parm.raster->type	= TYPE_STRING;
	parm.raster->description= _("Set region to match this raster map");

	parm.vect = G_define_option();
	parm.vect->key		= "vector";
	parm.vect->key_desc	= "name";
	parm.vect->required	= NO;
	parm.vect->multiple	= NO;
	parm.vect->type		= TYPE_STRING;
	parm.vect->description	= _("Set region to match this vector map");

	parm.view = G_define_option();
	parm.view->key		= "3dview";
	parm.view->key_desc	= "name";
	parm.view->required	= NO;
	parm.view->multiple	= NO;
	parm.view->type		= TYPE_STRING;
	parm.view->description	= _("Set region to match this 3dview file");

	parm.north = G_define_option();
	parm.north->key		= "n";
	parm.north->key_desc	= "value";
	parm.north->required	= NO;
	parm.north->multiple	= NO;
	parm.north->type	= TYPE_STRING;
	parm.north->description = llinfo(_("Value for the northern edge"), G_lat_format_string(), window.proj);

	parm.south = G_define_option();
	parm.south->key		= "s";
	parm.south->key_desc	= "value";
	parm.south->required	= NO;
	parm.south->multiple	= NO;
	parm.south->type	= TYPE_STRING;
	parm.south->description = llinfo(_("Value for the southern edge"), G_lat_format_string(), window.proj);

	parm.east = G_define_option();
	parm.east->key		= "e";
	parm.east->key_desc	= "value";
	parm.east->required	= NO;
	parm.east->multiple	= NO;
	parm.east->type		= TYPE_STRING;
	parm.east->description	= llinfo(_("Value for the eastern edge"), G_lon_format_string(), window.proj);

	parm.west = G_define_option();
	parm.west->key		= "w";
	parm.west->key_desc	= "value";
	parm.west->required	= NO;
	parm.west->multiple	= NO;
	parm.west->type		= TYPE_STRING;
	parm.west->description	= llinfo(_("Value for the western edge"), G_lon_format_string(), window.proj);

	parm.align = G_define_option();
	parm.align->key		= "align";
	parm.align->key_desc	= "name";
	parm.align->required	= NO;
	parm.align->multiple	= NO;
	parm.align->type	= TYPE_STRING;
	parm.align->description = _("Raster map to align to");

	if (G_parser(argc,argv))
		exit (EXIT_FAILURE);

	name = parm.map->answer;

	mapset = G_find_cell2(name, "");
	if (!mapset)
		G_fatal_error(_("Raster map <%s> not found"), name);
	if (G_get_cellhd(name, mapset, &cellhd) < 0)
		G_fatal_error(_("Unable to read header of raster map <%s@%s>"), name, mapset);

	G_copy(&window, &cellhd, sizeof(window));

	if (flag.dflt->answer)
		G_get_default_window(&window);

	if (flag.cur->answer)
		G_get_window(&window);

	if ((name = parm.region->answer))	/* region= */
	{
		mapset = G_find_file("windows", name, "");
		if (!mapset)
			G_fatal_error(_("Region <%s> not found"), name);
		if (G__get_window(&window, "windows", name, mapset) != NULL)
			G_fatal_error(_("Unable to read region <%s> in <%s>"), name, mapset);
	}

	if ((name = parm.view->answer))	/* 3dview= */
	{
		struct G_3dview v;
		FILE *fp;
		int ret;
		
		mapset = G_find_file2("3d.view", name, "");
		if (!mapset)
			G_fatal_error(_("3dview file <%s> not found"), name);

		G_3dview_warning(0); /* suppress boundary mismatch warning */

		fp = G_fopen_old("3d.view",name,mapset);
		if (!fp)
			G_fatal_error(_("Unable to open 3dview file <%s> in <%s>"), name, mapset);

		ret = G_get_3dview(name, mapset, &v);
		if (ret < 0)
			G_fatal_error(_("Unable to read 3dview file <%s> in <%s>"), name, mapset);
		if (ret == 0)
			G_fatal_error(_("Old 3dview file. Region <%s> not found in <%s>"), name, mapset);

		 
		window.north	= v.vwin.north;
		window.south	= v.vwin.south;
		window.west	= v.vwin.west;
		window.east	= v.vwin.east;

		fclose(fp);

	}

	if ((name = parm.raster->answer))	/* raster= */
	{
		mapset = G_find_cell2(name, "");
		if (!mapset)
			G_fatal_error(_("Raster map <%s> not found"), name);
		if (G_get_cellhd(name, mapset, &window) < 0)
			G_fatal_error(_("Unable to read header of raster map <%s@%s>"), name, mapset);
	}

	if ((name = parm.vect->answer))	/* vect= */
	{
		struct Map_info Map;
		BOUND_BOX box;
		
		mapset = G_find_vector2(name, "");
		if (!mapset)
			G_fatal_error(_("Vector map <%s> not found"), name);

		Vect_set_open_level(1);
		if (Vect_open_old(&Map, name, mapset) != 1)
			G_fatal_error(_("Unable to open vector map <%s> in <%s>"), name, mapset);

		Vect_get_map_box (&Map, &box );
		window.north	= box.N;
		window.south	= box.S;
		window.west	= box.W;
		window.east	= box.E;

		G_align_window(&window, &cellhd);

		Vect_close(&Map);
	}

	if ((value = parm.north->answer))	/* n= */
	{
		if ((i = nsew(value, "n+", "n-", "s+")))
		{
			if (!G_scan_resolution(value+2, &x, window.proj))
				die(parm.north);
			switch(i)
			{
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

	if ((value = parm.south->answer))	/* s= */
	{
		if ((i = nsew(value, "s+", "s-", "n-")))
		{
			if (!G_scan_resolution(value+2, &x, window.proj))
				die(parm.south);
			switch(i)
			{
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

	if ((value = parm.east->answer))	/* e= */
	{
		if ((i = nsew(value, "e+", "e-", "w+")))
		{
			if (!G_scan_resolution(value+2, &x, window.proj))
				die(parm.east);
			switch(i)
			{
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

	if ((value = parm.west->answer))	/* w= */
	{
		if ((i = nsew(value, "w+", "w-", "e-")))
		{
			if (!G_scan_resolution(value+2, &x, window.proj))
				die(parm.west);
			switch(i)
			{
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

	if ((name = parm.align->answer))	/* align= */
	{
		struct Cell_head temp_window;

		mapset = G_find_cell2(name, "");
		if (!mapset)
			G_fatal_error(_("Raster map <%s> not found"), name);
		if (G_get_cellhd(name, mapset, &temp_window) < 0)
			G_fatal_error(_("Unable to read header of raster map <%s@%s>"), name, mapset);
		if ((err = G_align_window(&window, &temp_window)))
			G_fatal_error("%s in %s: %s", name, mapset, err);
	}

	window.rows = cellhd.rows;
	window.cols = cellhd.cols;

	if ((err = G_adjust_Cell_head(&window, 1, 1)))
		G_fatal_error(_("Invalid region: %s"), err);

	cellhd.north	= window.north;
	cellhd.south	= window.south;
	cellhd.east	= window.east;
	cellhd.west	= window.west;

	if (G_put_cellhd(parm.map->answer, &cellhd) < 0)
		G_fatal_error(_("Unable to update boundaries"));

	G_done_msg(" ");

	return 0;
}

static void die(struct Option *parm)
{
	G_fatal_error("<%s=%s> ** illegal value **", parm->key, parm->answer);
}

static int nsew(const char *value, const char *a, const char *b, const char *c)
{
	if (strncmp(value, a, strlen(a)) == 0) return 1;
	if (strncmp(value, b, strlen(b)) == 0) return 2;
	if (strncmp(value, c, strlen(c)) == 0) return 3;
	return 0;
}

static char *llinfo(const char *msg, const char *llformat, int proj)
{
	char buf[256];
	if (proj != PROJECTION_LL)
		return (char *) msg;

	sprintf(buf, "%s (format %s)", msg, llformat);
	return G_store(buf);
}

