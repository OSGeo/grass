
/****************************************************************************
 *
 * MODULE:       g.mlist
 *
 * AUTHOR(S):    Huidae Cho
 * 		 Based on general/manage/cmd/list.c by Michael Shapiro.
 *
 * PURPOSE:      Lists available GRASS data base files of the
 *               user-specified data type to standard output
 *
 * COPYRIGHT:    (C) 1999-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/vector.h>
#include <grass/manage.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

enum {
    TYPE_RAST,
    TYPE_RAST3D,
    TYPE_VECT,
    TYPE_3DVIEW,
    TYPE_OTHERS
};

static int any = 0;

static void make_list(FILE *, const struct list *, const char *,
		      const char *, int, int, struct Cell_head *);
static int region_overlaps(struct Cell_head *, const char *, const char *, int);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *type;
	struct Option *pattern;
	struct Option *exclude;
	struct Option *separator;
	struct Option *mapset;
	struct Option *region;
	struct Option *output;
    } opt;
    struct
    {
	struct Flag *regex;
	struct Flag *extended;
	struct Flag *type;
	struct Flag *mapset;
	struct Flag *pretty;
	struct Flag *full;
    } flag;
    int i, j, n, all, num_types, nlist;
    void *filter, *exclude;
    struct Popen pager;
    FILE *fp;
    const char *mapset;
    char *separator;
    int use_region;
    struct Cell_head window;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("list"));
    module->description =
	_("Lists available GRASS data base files of "
	  "the user-specified data type optionally using the search pattern.");

    M_read_list(FALSE, &nlist);

    opt.type = G_define_standard_option(G_OPT_M_DATATYPE);
    opt.type->multiple = YES;
    opt.type->options = M_get_options(TRUE);
    opt.type->descriptions = M_get_option_desc(TRUE);

    opt.pattern = G_define_option();
    opt.pattern->key = "pattern";
    opt.pattern->type = TYPE_STRING;
    opt.pattern->required = NO;
    opt.pattern->multiple = NO;
    opt.pattern->description = _("Map name search pattern (default: all)");
    opt.pattern->guisection = _("Pattern");

    opt.exclude = G_define_option();
    opt.exclude->key = "exclude";
    opt.exclude->type = TYPE_STRING;
    opt.exclude->required = NO;
    opt.exclude->multiple = NO;
    opt.exclude->description = _("Map name exclusion pattern (default: none)");
    opt.exclude->guisection = _("Pattern");

    opt.mapset = G_define_standard_option(G_OPT_M_MAPSET);
    opt.mapset->multiple = YES;
    opt.mapset->label =
	_("Name of mapset to list (default: current search path)");
    opt.mapset->description =
	_("'.' for current mapset; '*' for all mapsets in location");
    opt.separator = G_define_standard_option(G_OPT_F_SEP);
    opt.separator->answer = "newline";

    opt.region = G_define_standard_option(G_OPT_M_REGION);
    opt.region->label = _("Name of saved region for map search (default: not restricted)");
    opt.region->description =
	_("'.' for current region; '*' for default region");

    opt.output = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.output->required = NO;
    opt.output->label = _("Name for output file");
    opt.output->description = _("If not given or '-' then standard output");

    flag.regex = G_define_flag();
    flag.regex->key = 'r';
    flag.regex->description =
	_("Use basic regular expressions instead of wildcards");
    flag.regex->guisection = _("Pattern");

    flag.extended = G_define_flag();
    flag.extended->key = 'e';
    flag.extended->description =
	_("Use extended regular expressions instead of wildcards");
    flag.extended->guisection = _("Pattern");

    flag.type = G_define_flag();
    flag.type->key = 't';
    flag.type->description = _("Print data types");
    flag.type->guisection = _("Print");

    flag.mapset = G_define_flag();
    flag.mapset->key = 'm';
    flag.mapset->description = _("Print fully-qualified map names (including mapsets)");
    flag.mapset->guisection = _("Print");

    flag.pretty = G_define_flag();
    flag.pretty->key = 'p';
    flag.pretty->description = _("Pretty printing in human readable format");
    flag.pretty->guisection = _("Print");

    flag.full = G_define_flag();
    flag.full->key = 'f';
    flag.full->description = _("Verbose listing (also list map titles)");
    flag.full->guisection = _("Print");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if ((flag.pretty->answer || flag.full->answer) && opt.output->answer)
        G_fatal_error(_("-%c/-%c and %s= are mutually exclusive"),
		      flag.pretty->key, flag.full->key, opt.output->key);

    if ((flag.pretty->answer || flag.full->answer) && opt.region->answer)
        G_fatal_error(_("-%c/-%c and %s= are mutually exclusive"),
		      flag.pretty->key, flag.full->key, opt.region->key);

    if ((flag.pretty->answer || flag.full->answer) &&
	(flag.mapset->answer || flag.type->answer))
	G_fatal_error(_("-%c/-%c and -%c/-%c are mutually exclusive"),
		      flag.pretty->key, flag.full->key,
		      flag.mapset->key, flag.type->key);

    if (flag.pretty->answer && flag.full->answer)
	G_fatal_error(_("-%c and -%c are mutually exclusive"),
		      flag.pretty->key, flag.full->key);

    if (flag.regex->answer && flag.extended->answer)
	G_fatal_error(_("-%c and -%c are mutually exclusive"),
		      flag.regex->key, flag.extended->key);

    if (opt.pattern->answer) {
	if (flag.regex->answer || flag.extended->answer)
	    filter = G_ls_regex_filter(opt.pattern->answer, 0,
			    	       (int)flag.extended->answer);
	else {
	    /* handle individual map names */
	    if (strchr(opt.pattern->answer, ',')) {
		char *pattern;

		pattern = (char *)G_malloc(strlen(opt.pattern->answer) + 3);
		sprintf(pattern, "{%s}", opt.pattern->answer);

		filter = G_ls_glob_filter(pattern, 0);
	    }
	    else
		filter = G_ls_glob_filter(opt.pattern->answer, 0);
	}
	if (!filter)
	    G_fatal_error(_("Unable to compile pattern <%s>"),
			  opt.pattern->answer);
    }
    else
	filter = NULL;

    if (opt.exclude->answer) {
	if (flag.regex->answer || flag.extended->answer)
	    exclude = G_ls_regex_filter(opt.exclude->answer, 1,
			    		(int)flag.extended->answer);
	else {
	    /* handle individual map names */
	    if (strchr(opt.exclude->answer, ',')) {
		char *pattern;

		pattern = (char *)G_malloc(strlen(opt.exclude->answer) + 3);
		sprintf(pattern, "{%s}", opt.exclude->answer);

		exclude = G_ls_glob_filter(pattern, 1);
	    }
	    else
		exclude = G_ls_glob_filter(opt.exclude->answer, 1);
	}
	if (!exclude)
	    G_fatal_error(_("Unable to compile pattern <%s>"),
			  opt.exclude->answer);
    }
    else
	exclude = NULL;

    separator = G_option_to_separator(opt.separator);

    if (opt.region->answer) {
	use_region = 1;

	if (strcmp(opt.region->answer, "*") == 0)
	    G_get_default_window(&window);
	else if (strcmp(opt.region->answer, ".") == 0)
	    G_get_window(&window);
	else {
	    char name[GNAME_MAX], mapset[GMAPSET_MAX];

	    if (G_name_is_fully_qualified(opt.region->answer, name, mapset))
		G__get_window(&window, "windows", name, mapset);
	    else
		G__get_window(&window, "windows", opt.region->answer, "");
	}
    }
    else
	use_region = 0;

    for (i = 0; opt.type->answers[i]; i++) {
	if (strcmp(opt.type->answers[i], "all") == 0)
	    break;
    }
    if (opt.type->answers[i]) {
	all = 1;
	num_types = nlist;
    }
    else {
	all = 0;
	num_types = i;
    }

    if (opt.mapset->answers && opt.mapset->answers[0]) {
	G_create_alt_search_path();
	for (i = 0; (mapset = opt.mapset->answers[i]); i++) {
	    if (strcmp(mapset, "*") == 0) {
		/* all mapsets from current location */
		char **ms;

		ms = G_get_available_mapsets();
		for (j = 0; (mapset = ms[j]); j++)
		    G_add_mapset_to_search_path(mapset);
		continue;
	    }
	    else if (strcmp(mapset, ".") == 0)
		mapset = G_mapset();
	    else if (G__mapset_permissions(mapset) == -1)
		G_fatal_error(_("Mapset <%s> does not exist"), mapset);
	    G_add_mapset_to_search_path(mapset);
	}
    }

    if (flag.pretty->answer || flag.full->answer) {
	fp = G_open_pager(&pager);
	dup2(fileno(fp), STDOUT_FILENO);
    }
    else
	fp = G_open_option_file(opt.output);

    for (i = 0; i < num_types; i++) {
	const struct list *elem;

	n = all ? i : M_get_element(opt.type->answers[i]);
	elem = M_get_list(n);

	if (flag.full->answer) {
	    char lister[GPATH_MAX];

	    sprintf(lister, "%s/etc/lister/%s", G_gisbase(), elem->element[0]);

	    G_debug(3, "lister CMD: %s", lister);

	    if (access(lister, X_OK) == 0)	/* execute permission? */
		G_spawn(lister, lister, mapset, NULL);
	    else
		M_do_list(n, "");
	}
	else if (flag.pretty->answer)
	    M_do_list(n, "");
	else {
	    for (j = 0; (mapset = G_get_mapset_name(j)); j++)
		make_list(fp, elem, mapset, separator, flag.type->answer,
			  flag.mapset->answer, use_region ? &window : NULL);
	}
    }

    if (flag.pretty->answer || flag.full->answer) {
	fclose(stdout);
	G_close_pager(&pager);
    }
    else {
	if (any)
	    fprintf(fp, "\n");
	G_close_option_file(fp);
    }

    if (filter)
	G_free_ls_filter(filter);

    if (exclude)
	G_free_ls_filter(exclude);

    exit(EXIT_SUCCESS);
}

static void make_list(FILE *fp, const struct list *elem, const char *mapset,
		      const char *separator, int add_type, int add_mapset,
		      struct Cell_head *window)
{
    static int first_mapset = 1;
    char path[GPATH_MAX];
    const char *element, *alias;
    char **list;
    int count, first, i;
    int type;

    element = elem->element[0];
    alias = elem->alias;

    G_file_name(path, element, "", mapset);
    if (access(path, 0) != 0)
	return;

    if ((list = G__ls(path, &count)) == NULL)
	return;

    if (strcmp(alias, "rast") == 0)
	type = TYPE_RAST;
    else if (strcmp(alias, "rast3d") == 0)
	type = TYPE_RAST3D;
    else if (strcmp(alias, "vect") == 0)
	type = TYPE_VECT;
    else if (strcmp(alias, "3dview") == 0)
	type = TYPE_3DVIEW;
    else
	type = TYPE_OTHERS;

    /* Suppress "... found in more mapsets" warnings from G_find_file2. */
    G_suppress_warnings(1);

    first = 1;
    for (i = 0; i < count; i++) {
	char *name = list[i];
	int need_mapset = 0;

	/* If region= is used, read the map region. */
	if (window) {
	    /* If the map region doesn't overlap with the input region, don't
	     * print the map. */
	    if (!region_overlaps(window, name, mapset, type))
		continue;
	}

        if (first) {
	    first = 0;

	    if (any)
		fprintf(fp, "%s", separator);
	    if (fp == stdout && isatty(STDOUT_FILENO))
		G_message(_("%s available in mapset <%s>:"),
			  elem->text, mapset);
	}

	if (any && i != 0)
	    fprintf(fp, "%s", separator);

	if (add_type)
	    fprintf(fp, "%s/", alias);

	fprintf(fp, "%s", name);

	if (!add_mapset && !first_mapset) {
	    const char *mapset2 = G_find_file2(element, name, "");
            if (mapset2)
                need_mapset = strcmp(mapset, mapset2) != 0;
	}
	if (add_mapset || need_mapset)
	    fprintf(fp, "@%s", mapset);

	G_free(name);

	any++;
    }

    G_suppress_warnings(0);
    fflush(fp);

    G_free(list);

    first_mapset = 0;
}

static int region_overlaps(struct Cell_head *window, const char *name,
			   const char *mapset, int type)
{
    int has_region;
    struct Cell_head map_window;
    RASTER3D_Region region3d;
    struct Map_info Map;
    struct bound_box box;
    int ret;
    struct G_3dview view3d;

    switch (type) {
    case TYPE_RAST:
	Rast_get_cellhd(name, mapset, &map_window);
	has_region = 1;
	break;
    case TYPE_RAST3D:
	if (Rast3d_read_region_map(name, mapset, &region3d) < 0)
	    G_fatal_error(_("Unable to read header of 3D raster map <%s@%s>"),
			  name, mapset);
	Rast3d_region_to_cell_head(&region3d, &map_window);
	has_region = 1;
	break;
    case TYPE_VECT:
	Vect_set_open_level(2);
	if (Vect_open_old_head(&Map, name, mapset) < 2)
	    G_fatal_error(_("Unable to open vector map <%s@%s> on topological level"),
			  name, mapset);
	Vect_get_map_box(&Map, &box);
	Vect_close(&Map);

	map_window.north = box.N;
	map_window.south = box.S;
	map_window.west = box.W;
	map_window.east = box.E;
	has_region = 1;
	break;
    case TYPE_3DVIEW:
	if ((ret = G_get_3dview(name, mapset, &view3d)) < 0)
	    G_fatal_error(_("Unable to read 3dview file <%s@%s>"),
			  name, mapset);
	if (ret == 0) {
	    G_warning(_("No region support in an old 3dview file <%s@%s>. Listing anyway"),
		      name, mapset);
	    has_region = 0;
	}
	else {
	    map_window.north = view3d.vwin.north;
	    map_window.south = view3d.vwin.south;
	    map_window.west = view3d.vwin.west;
	    map_window.east = view3d.vwin.east;
	    has_region = 1;
	}
	break;
    default:
	has_region = 0;
	break;
    }

    /* If an element doesn't have a concept of region at all, return 1 so we
     * can always print it. */
    if (!has_region)
	return 1;

    /* If the map region is outside the input region, return 0. Otherwise
     * return 1 */
    return !(window->north <= map_window.south ||
	     window->south >= map_window.north ||
	     window->west >= map_window.east ||
	     window->east <= map_window.west);
}
