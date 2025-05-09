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
 * COPYRIGHT:    (C) 2001-2007, 2011, 2013 by the GRASS Development Team
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
static void print_subgroups(const char *group, const char *mapset, int simple);

int main(int argc, char *argv[])
{
    char group[GNAME_MAX], mapset[GMAPSET_MAX];
    char xgroup[GNAME_MAX];
    char **rasters = NULL;
    int m, k = 0;
    int can_edit;

    struct Option *grp, *rast, *rastf, *sgrp;
    struct Flag *r, *l, *s, *simple_flag;
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

    sgrp = G_define_standard_option(G_OPT_I_SUBGROUP);
    sgrp->required = NO;
    sgrp->description = _("Name of imagery subgroup");

    rast = G_define_standard_option(G_OPT_R_INPUTS);
    rast->required = NO; /* -l flag */
    rast->description = _("Name of raster map(s) to include in group");
    rast->guisection = _("Maps");

    rastf = G_define_standard_option(G_OPT_F_INPUT);
    rastf->key = "file";
    rastf->description = _("Input file with one raster map name per line");
    rastf->required = NO;

    r = G_define_flag();
    r->key = 'r';
    r->description =
        _("Remove selected files from specified group or subgroup");
    r->guisection = _("Maps");

    l = G_define_flag();
    l->key = 'l';
    l->description = _("List files from specified (sub)group");
    l->guisection = _("Print");

    s = G_define_flag();
    s->key = 's';
    s->description = _("List subgroups from specified group");
    s->guisection = _("Print");

    simple_flag = G_define_flag();
    simple_flag->key = 'g';
    simple_flag->description = _("Print in shell script style");
    simple_flag->guisection = _("Print");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* backward compatibility -> simple list implied l flag list, if there was
       only l flag (with s flag added it is not clear, simple_flag is linked to
       both) */
    if ((simple_flag->answer && !s->answer) && !l->answer)
        l->answer = TRUE;

    /* Determine number of raster maps to include */
    if (rast->answers) {
        for (m = 0; rast->answers[m]; m++) {
            k = m;
        }
        k++;
        rasters = rast->answers;
    }
    /* process the input maps from the file */
    else if (rastf->answer) {
        FILE *in;

        m = 10;
        rasters = G_malloc(m * sizeof(char *));
        in = fopen(rastf->answer, "r");
        if (!in)
            G_fatal_error(_("Unable to open input file <%s>"), rastf->answer);

        for (;;) {
            char buf[GNAME_MAX];
            char *name;

            if (!G_getl2(buf, sizeof(buf), in))
                break;

            name = G_chop(buf);

            /* Ignore empty lines */
            if (!*name)
                continue;

            if (m <= k) {
                m += 10;
                rasters = G_realloc(rasters, m * sizeof(char *));
            }
            rasters[k] = G_store(name);
            k++;
        }
        fclose(in);
    }

    if (k < 1 && !(l->answer || s->answer)) /* remove if input is requirement */
        G_fatal_error(_("No input raster map(s) specified"));

    /* Get groups mapset. Remove @mapset if group contains
     */
    strcpy(xgroup, grp->answer);
    can_edit = G_unqualified_name(xgroup, G_mapset(), group, mapset) != -1;

    if (r->answer && can_edit) {
        /* Remove files from Group */

        if (I_find_group(group) == 0) {
            G_fatal_error(
                _("Specified group does not exist in current mapset"));
        }

        if (sgrp->answer) {
            G_verbose_message(_("Removing raster maps from subgroup <%s>..."),
                              sgrp->answer);
            remove_subgroup_files(group, sgrp->answer, rasters, k);
        }
        else {
            G_verbose_message(_("Removing raster maps from group <%s>..."),
                              group);
            remove_group_files(group, rasters, k);
        }
    }
    else {
        if (l->answer || s->answer) {
            /* List raster maps in group */
            if (!I_find_group2(group, mapset))
                G_fatal_error(_("Group <%s> not found"), group);

            struct Ref ref;

            if (sgrp->answer) {
                /* list subgroup files */
                I_get_subgroup_ref2(group, sgrp->answer, mapset, &ref);
                if (simple_flag->answer) {
                    G_message(_("Subgroup <%s> of group <%s> references the "
                                "following raster maps:"),
                              sgrp->answer, group);
                    I_list_subgroup_simple(&ref, stdout);
                }
                else
                    I_list_subgroup(group, sgrp->answer, &ref, stdout);
            }
            else if (s->answer) {
                print_subgroups(group, mapset, simple_flag->answer);
            }
            else {
                /* list group files */
                I_get_group_ref2(group, mapset, &ref);
                if (simple_flag->answer) {
                    G_message(
                        _("Group <%s> references the following raster maps:"),
                        group);
                    I_list_group_simple(&ref, stdout);
                }
                else
                    I_list_group(group, &ref, stdout);
            }
        }
        else {
            if (!can_edit) {
                /* GTC Group refers to an image group */
                G_fatal_error(
                    _("Only groups from the current mapset can be edited"));
            }
            /* Create or update Group REF */
            if (I_find_group(group) == 0)
                G_verbose_message(
                    _("Group <%s> does not yet exist. Creating..."), group);

            if (sgrp->answer) {
                G_verbose_message(_("Adding raster maps to group <%s>..."),
                                  group);
                add_or_update_group(group, rasters, k);

                G_verbose_message(_("Adding raster maps to subgroup <%s>..."),
                                  sgrp->answer);
                add_or_update_subgroup(group, sgrp->answer, rasters, k);
            }
            else {
                G_verbose_message(_("Adding raster maps to group <%s>..."),
                                  group);
                add_or_update_group(group, rasters, k);
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
        if (!rasters[m]) {
            G_warning(_("No input raster maps defined"));
            return 0;
        }
        if ((mapset = G_find_raster(rasters[m], "")) == NULL) {
            G_warning(_("Raster map <%s> not found. Skipped."), rasters[m]);
            skip = 1;
            continue;
        }
        char *rname = G_fully_qualified_name(rasters[m], mapset);

        G_message(_("Adding raster map <%s> to group"), rname);

        /* Go through existing files to check for duplicates */
        for (n = 0; n < ref.nfiles; n++) {
            if (strcmp(rasters[m], ref.file[n].name) == 0) {
                G_message(_("Raster map <%s> exists in group. Skipped."),
                          rname);
                skip = 1;
                continue;
            }
        }

        if (skip == 0)
            I_add_file_to_group_ref(rasters[m], mapset, &ref);
        G_free(rname);
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
        if ((mapset = G_find_raster(rasters[m], "")) == NULL) {
            G_warning(_("Raster map <%s> not found. Skipped."), rasters[m]);
            skip = 1;
            continue;
        }
        char *rname = G_fully_qualified_name(rasters[m], mapset);

        G_message(_("Adding raster map <%s> to subgroup"), rname);

        /* Go through existing files to check for duplicates */
        for (n = 0; n < ref.nfiles; n++) {
            if (strcmp(rasters[m], ref.file[n].name) == 0) {
                G_message(_("Raster map <%s> exists in subgroup. Skipping..."),
                          rname);
                skip = 1;
                continue;
            }
        }
        if (skip == 0)
            I_add_file_to_group_ref(rasters[m], mapset, &ref);
        G_free(rname);
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

static void print_subgroups(const char *group, const char *mapset, int simple)
{
    int subgs_num, i;
    int len, tot_len;
    int max;
    char **subgs;

    subgs = I_list_subgroups2(group, mapset, &subgs_num);
    if (simple)
        for (i = 0; i < subgs_num; i++)
            fprintf(stdout, "%s\n", subgs[i]);
    else {
        if (subgs_num <= 0) {
            fprintf(stdout, _("Group <%s> does not contain any subgroup.\n"),
                    group);
            G_free(subgs);
            return;
        }
        max = 0;
        for (i = 0; i < subgs_num; i++) {
            len = strlen(subgs[i]) + 4;
            if (len > max)
                max = len;
        }
        fprintf(stdout, _("group <%s> references the following subgroups\n"),
                group);
        fprintf(stdout, "-------------\n");
        tot_len = 0;
        for (i = 0; i < subgs_num; i++) {
            tot_len += max;
            if (tot_len > 78) {
                fprintf(stdout, "\n");
                tot_len = max;
            }
            fprintf(stdout, "%-*s", max, subgs[i]);
        }
        if (tot_len)
            fprintf(stdout, "\n");
        fprintf(stdout, "-------------\n");
    }
    G_free(subgs);
    return;
}
