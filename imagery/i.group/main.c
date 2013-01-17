
/****************************************************************************
 *
 * MODULE:       i.group
 * AUTHOR(S):    Michael Shapiro (USACERL) (original contributor)
 *               Bob Covill <bcovill tekmap.ns.ca>,
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>
 * PURPOSE:      collect raster map layers into an imagery group by assigning 
 *               them to user-named subgroups or other groups
 * COPYRIGHT:    (C) 2001-2007, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>


/* function prototypes */
static int add_or_update_group(char group[INAME_LEN], char **rasters, int k);
static int add_or_update_subgroup(char group[INAME_LEN],
				  char subgroup[INAME_LEN], char **rasters,
				  int k);
static int remove_group_files(char group[INAME_LEN], char **rasters, int k);
static int remove_subgroup_files(char group[INAME_LEN],
				 char subgroup[INAME_LEN], char **rasters,
				 int k);


int main(int argc, char *argv[])
{
    char group[GNAME_MAX], mapset[GMAPSET_MAX];
    int m, k = 0;

    struct Option *grp, *rast, *sgrp;
    struct Flag *r, *l, *simple_flag;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("map management"));
    module->description =
	_("Creates, edits, and lists groups of imagery data.");

    /* Get Args */
    grp = G_define_standard_option(G_OPT_I_GROUP);
    grp->description = _("Name of imagery group");

    sgrp = G_define_option();
    sgrp->key = "subgroup";
    sgrp->type = TYPE_STRING;
    sgrp->required = NO;
    sgrp->description = _("Name of imagery sub-group");

    rast = G_define_standard_option(G_OPT_R_INPUTS);
    rast->required = NO;	/* -l flag */
    rast->description = _("Name of raster map(s) to include in group");
    rast->guisection = _("Maps");

    r = G_define_flag();
    r->key = 'r';
    r->description = _("Remove selected files from specified group or subgroup");
    r->guisection = _("Maps");
    
    l = G_define_flag();
    l->key = 'l';
    l->description = _("List files from specified (sub)group");
    l->guisection = _("Print");

    simple_flag = G_define_flag();
    simple_flag->key = 'g';
    simple_flag->description =
	_("List files from specified (sub)group (shell script style)");
    simple_flag->guisection = _("Print");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* simple list implies list */
    if (simple_flag->answer && !l->answer)
	l->answer = TRUE;

    /* Determine number of files to include */
    if (rast->answers) {
	for (m = 0; rast->answers[m]; m++) {
	    k = m;
	}
	k++;
    }

    if (k < 1 && !l->answer)	/* remove if input is requirement */
	G_fatal_error(_("No input raster map(s) specified"));

    /* check if current mapset:  (imagery libs are very lacking in this dept)
       - abort if not,
       - remove @mapset part if it is
     */
    if (G_name_is_fully_qualified(grp->answer, group, mapset)) {
	if (strcmp(mapset, G_mapset()))
	    G_fatal_error(_("Group must exist in the current mapset"));
    }
    else {
	strcpy(group, grp->answer);	/* FIXME for buffer overflow (have the parser check that?) */
    }

    if (r->answer) {
	/* Remove files from Group */

	if (I_find_group(group) == 0) {
	    G_fatal_error(_("Specified group does not exist in current mapset"));
	}

	if (sgrp->answer) {
	    G_verbose_message(_("Removing raster maps from subgroup <%s>..."),
			      sgrp->answer);
	    remove_subgroup_files(group, sgrp->answer, rast->answers, k);
	}
	else {
	    G_verbose_message(_("Removing raster maps from group <%s>..."),
			      group);
	    remove_group_files(group, rast->answers, k);
	}
    }
    else {
	if (l->answer) {
	    /* List raster maps in group */

	    struct Ref ref;

	    if (I_find_group(group) == 0) {
		G_fatal_error(_("Specified group does not exist in current mapset"));
	    }

	    if (sgrp->answer) {
		/* list subgroup files */
		I_get_subgroup_ref(group, sgrp->answer, &ref);
		if (simple_flag->answer) {
		    G_message(_("Subgroup <%s> of group <%s> references the following raster maps:"),
			      sgrp->answer, group);
		    I_list_subgroup_simple(&ref, stdout);
		}
		else
		    I_list_subgroup(group, sgrp->answer, &ref, stdout);
	    }
	    else {
		/* list group files */
		I_get_group_ref(group, &ref);
		if (simple_flag->answer) {
		    G_message(_("Group <%s> references the following raster maps:"), group);
		    I_list_group_simple(&ref, stdout);
		}
		else
		    I_list_group(group, &ref, stdout);
	    }
	}
	else {
	    /* Create or update Group REF */
	    if (I_find_group(group) == 0)
		G_verbose_message(_("Group <%s> does not yet exist. Creating..."),
				  group);

	    if (sgrp->answer) {
		G_verbose_message(_("Adding raster maps to group <%s>..."),
				  group);
		add_or_update_group(group, rast->answers, k);

		G_verbose_message(_("Adding raster maps to subgroup <%s>..."),
				  sgrp->answer);
		add_or_update_subgroup(group, sgrp->answer, rast->answers, k);
	    }
	    else {
		G_verbose_message(_("Adding raster maps to group <%s>..."),
				  group);
		add_or_update_group(group, rast->answers, k);
	    }
	}
    }
    
    return EXIT_SUCCESS;
}


static int add_or_update_group(char group[INAME_LEN], char **rasters, int k)
{
    int m, n, skip;
    struct Ref ref;
    const char *mapset;

    I_get_group_ref(group, &ref);

    for (m = 0; m < k; m++) {
	skip = 0;
	if ((mapset = G_find_raster(rasters[m], "")) == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), rasters[m]);

	G_message(_("Adding raster map <%s> to group"),
		  G_fully_qualified_name(rasters[m], mapset));

	/* Go through existing files to check for duplicates */
	for (n = 0; n < ref.nfiles; n++) {
	    if (strcmp(rasters[m], ref.file[n].name) == 0) {
		G_message(_("Raster map <%s> exists in group. Skipping..."),
			  G_fully_qualified_name(rasters[m], mapset));
		skip = 1;
		continue;
	    }
	}

	if (skip == 0)
	    I_add_file_to_group_ref(rasters[m], mapset, &ref);
    }

    G_debug(1, "writing group REF");
    I_put_group_ref(group, &ref);

    return 0;
}


static int add_or_update_subgroup(char group[INAME_LEN],
				  char subgroup[INAME_LEN], char **rasters,
				  int k)
{
    int m, n, skip;
    struct Ref ref;
    const char *mapset;

    I_get_subgroup_ref(group, subgroup, &ref);

    for (m = 0; m < k; m++) {
	skip = 0;
	if ((mapset = G_find_raster(rasters[m], "")) == NULL)
	    G_fatal_error(_("Raster map <%s> not found"),
			  G_fully_qualified_name(rasters[m], mapset));

	G_message(_("Adding raster map <%s> to subgroup"),
		  G_fully_qualified_name(rasters[m], mapset));

	/* Go through existing files to check for duplicates */
	for (n = 0; n < ref.nfiles; n++) {
	    if (strcmp(rasters[m], ref.file[n].name) == 0) {
		G_message(_("Raster map <%s> exists in subgroup. Skipping..."),
			  G_fully_qualified_name(rasters[m], mapset));
		skip = 1;
		continue;
	    }
	}
	if (skip == 0)
	    I_add_file_to_group_ref(rasters[m], mapset, &ref);
    }

    G_debug(1, "writing subgroup REF");
    I_put_subgroup_ref(group, subgroup, &ref);

    return 0;
}


static int remove_group_files(char group[INAME_LEN], char **rasters, int k)
{
    int m, n, skip;
    struct Ref ref;
    struct Ref ref_tmp;
    const char *mapset;
    char tmp_name[INAME_LEN];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    I_get_group_ref(group, &ref_tmp);
    I_init_group_ref(&ref);


    G_debug(3, "remove_group_files: ref_tmp.nfiles %d", ref_tmp.nfiles);
    /* Go through existing files to check for duplicates */
    for (m = 0; m < ref_tmp.nfiles; m++) {
	skip = 0;
	/* Parse through supplied rasters */
	for (n = 0; n < k; n++) {
	    strcpy(tmp_name, rasters[n]);
	    mapset = G_mapset();

	    /* Parse out mapset */
	    if (G_name_is_fully_qualified(rasters[n], xname, xmapset)) {
		strcpy(tmp_name, xname);
		mapset = xmapset;
	    }

	    G_debug(3, "tmp_name %s, ref_tmp.file[%d].name: %s", tmp_name, m,
		    ref_tmp.file[m].name);
	    if ((strcmp(tmp_name, ref_tmp.file[m].name) == 0) &&
		(strcmp(mapset, ref_tmp.file[m].mapset) == 0)) {
		G_message(_("Removing raster map <%s> from group"),
			  G_fully_qualified_name(tmp_name, mapset));
		skip = 1;
		break;
	    }
	}

	if (skip == 0) {
	    I_add_file_to_group_ref(ref_tmp.file[m].name,
				    ref_tmp.file[m].mapset, &ref);
	}
    }

    G_debug(1, "writing group REF");
    I_put_group_ref(group, &ref);

    if (ref.nfiles == ref_tmp.nfiles) {
	G_warning(_("No raster map removed"));
    }

    return 0;
}


static int remove_subgroup_files(char group[INAME_LEN],
				 char subgroup[INAME_LEN], char **rasters,
				 int k)
{
    int m, n, skip;
    struct Ref ref;
    struct Ref ref_tmp;
    const char *mapset;
    char tmp_name[INAME_LEN];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    I_get_subgroup_ref(group, subgroup, &ref_tmp);
    I_init_group_ref(&ref);

    G_debug(3, "remove_subgroup_files: ref_tmp.nfiles %d", ref_tmp.nfiles);
    /* Go through existing files to check for duplicates */
    for (m = 0; m < ref_tmp.nfiles; m++) {
	skip = 0;
	/* Parse through supplied rasters */
	for (n = 0; n < k; n++) {
	    strcpy(tmp_name, rasters[n]);
	    mapset = G_mapset();

	    /* Parse out mapset */
	    if (G_name_is_fully_qualified(rasters[n], xname, xmapset)) {
		strcpy(tmp_name, xname);
		mapset = xmapset;
	    }

	    G_debug(3, "tmp_name %s, ref_tmp.file[%d].name: %s", tmp_name, m,
		    ref_tmp.file[m].name);
	    G_debug(3, "mapset %s, ref_tmp.file[%d].mapset: %s", mapset, m,
		    ref_tmp.file[m].mapset);
	    if ((strcmp(tmp_name, ref_tmp.file[m].name) == 0) &&
		(strcmp(mapset, ref_tmp.file[m].mapset) == 0)) {
		G_message(_("Removing raster map <%s> from subgroup"),
			  G_fully_qualified_name(tmp_name, mapset));
		skip = 1;
		break;
	    }
	}

	if (skip == 0) {
	    I_add_file_to_group_ref(ref_tmp.file[m].name,
				    ref_tmp.file[m].mapset, &ref);
	}
    }

    G_debug(1, "writing subgroup REF");
    I_put_subgroup_ref(group, subgroup, &ref);

    if (ref.nfiles == ref_tmp.nfiles) {
	G_warning(_("No raster map removed"));
    }

    return 0;
}
