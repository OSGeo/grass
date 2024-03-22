
/****************************************************************************
 *
 * MODULE:       i.svm.predict
 * AUTHOR(S):    Maris Nartiss - maris.gis gmail.com
 * PURPOSE:      Predicts values with Support Vector Machine classifier
 *
 * COPYRIGHT:    (C) 2023 by Maris Nartiss and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *               Development of this module was supported from
 *               science funding of University of Latvia (2020-2023).
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <grass/config.h>
#if HAVE_SVM_H
#include <svm.h>
#elif HAVE_LIBSVM_SVM_H
#include <libsvm/svm.h>
#endif

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

/* LIBSVM message wrapper */
void print_func(const char *s)
{
    G_verbose_message("%s", s);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *opt_group, *opt_subgroup, *opt_sigfile, *opt_values;
    struct Option *opt_svm_cache_size;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("svm"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("prediction"));
    G_add_keyword(_("regression"));
    module->label = _("Predict with a SVM");
    module->description = _("Predict with a Support Vector Machine");

    opt_group = G_define_standard_option(G_OPT_I_GROUP);
    /* GTC: SVM prediction input */
    opt_group->description = _("Maps with feature values (attributes)");

    opt_subgroup = G_define_standard_option(G_OPT_I_SUBGROUP);
    opt_subgroup->required = NO;

    opt_sigfile = G_define_option();
    opt_sigfile->key = "signaturefile";
    opt_sigfile->type = TYPE_STRING;
    opt_sigfile->key_desc = "name";
    opt_sigfile->required = YES;
    opt_sigfile->gisprompt = "old,signatures/libsvm,sigfile";
    opt_sigfile->description = _("Name of input file containing signatures");

    opt_values = G_define_standard_option(G_OPT_R_OUTPUT);
    opt_values->required = YES;
    opt_values->description =
        _("Output map with predicted class or calculated value");

    opt_svm_cache_size = G_define_option();
    opt_svm_cache_size->key = "cache";
    opt_svm_cache_size->type = TYPE_INTEGER;
    opt_svm_cache_size->key_desc = "cache size";
    opt_svm_cache_size->required = NO;
    opt_svm_cache_size->options = "1-";
    opt_svm_cache_size->answer = "512";
    opt_svm_cache_size->description = _("LIBSVM kernel cache size in MB");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* Input validation */
    /* Input maps */
    char name_values[GNAME_MAX], name_sigfile[GNAME_MAX];
    char name_group[GNAME_MAX], name_subgroup[GNAME_MAX];
    char mapset_values[GMAPSET_MAX], mapset_sigfile[GMAPSET_MAX];
    char mapset_group[GMAPSET_MAX], mapset_subgroup[GMAPSET_MAX];
    char sigfile_dir[GPATH_MAX], model_file[GPATH_MAX];
    if (G_unqualified_name(opt_group->answer, NULL, name_group, mapset_group) ==
        0)
        strcpy(mapset_group, G_mapset());
    if (opt_subgroup->answer &&
        G_unqualified_name(opt_subgroup->answer, NULL, name_subgroup,
                           mapset_subgroup) != 0 &&
        strcmp(mapset_subgroup, mapset_group) != 0)
        G_fatal_error(_("Invalid subgroup <%s> provided"),
                      opt_subgroup->answer);
    if (!I_find_group2(name_group, mapset_group)) {
        G_fatal_error(_("Group <%s> not found in mapset <%s>"), name_group,
                      mapset_group);
    }
    if (opt_subgroup->answer &&
        !I_find_subgroup2(name_group, name_subgroup, mapset_group)) {
        G_fatal_error(_("Subgroup <%s> in group <%s@%s> not found"),
                      name_subgroup, name_group, mapset_group);
    }

    if (G_unqualified_name(opt_sigfile->answer, NULL, name_sigfile,
                           mapset_sigfile) == 0)
        strcpy(mapset_sigfile, G_mapset());
    if (!I_find_signature2(I_SIGFILE_TYPE_LIBSVM, name_sigfile, mapset_sigfile))
        G_fatal_error(_("Signature file <%s@%s> not found"), name_sigfile,
                      mapset_sigfile);

    if (G_unqualified_name(opt_values->answer, G_mapset(), name_values,
                           mapset_values) < 0)
        G_fatal_error(_("<%s> does not match the current mapset"),
                      mapset_values);
    if (G_legal_filename(name_values) < 0)
        G_fatal_error(_("<%s> is an illegal file name"), name_values);

    /* Get bands */
    struct Ref group_ref;
    if (opt_subgroup->answer) {
        if (!I_get_subgroup_ref2(name_group, opt_subgroup->answer, mapset_group,
                                 &group_ref)) {
            G_fatal_error(
                _("There was an error reading subgroup <%s> in group <%s@%s>"),
                opt_subgroup->answer, name_group, mapset_group);
        }
    }
    else {
        if (!I_get_group_ref2(name_group, mapset_group, &group_ref)) {
            G_fatal_error(_("There was an error reading group <%s@%s>"),
                          name_group, mapset_group);
        }
    }
    if (group_ref.nfiles <= 0) {
        if (opt_subgroup->answer)
            G_fatal_error(
                _("Subgroup <%s> in group <%s@%s> contains no raster maps."),
                opt_subgroup->answer, name_group, mapset_group);
        else
            G_fatal_error(_("Group <%s@%s> contains no raster maps."),
                          name_group, mapset_group);
    }
    const char **semantic_labels_group =
        G_malloc(group_ref.nfiles * sizeof(char *));
    for (int n = 0; n < group_ref.nfiles; n++) {
        semantic_labels_group[n] = Rast_get_semantic_label_or_name(
            group_ref.file[n].name, group_ref.file[n].mapset);
    }

    I_get_signatures_dir(sigfile_dir, I_SIGFILE_TYPE_LIBSVM);
    /* Read signature file version */
    int sigfile_version;
    FILE *misc_file =
        G_fopen_old_misc(sigfile_dir, "version", name_sigfile, mapset_sigfile);
    if (fscanf(misc_file, "%d", &sigfile_version) != 1) {
        G_fatal_error(_("Invalid signature file"));
    }
    fclose(misc_file);
    /* Current version number is 1 */
    if (sigfile_version != 1) {
        G_fatal_error(_("Invalid signature file version"));
    }

    /* Reorder group items to match order from the signature file (=training
     * order) */
    misc_file = G_fopen_old_misc(sigfile_dir, "semantic_label", name_sigfile,
                                 mapset_sigfile);
    if (!misc_file)
        G_fatal_error(_("Unable to read signature file '%s'."), name_sigfile);
    char **names_ordered = G_malloc(group_ref.nfiles * sizeof(char *));
    char **mapsets_ordered = G_malloc(group_ref.nfiles * sizeof(char *));

    char frmt[10];
    snprintf(frmt, sizeof(frmt), "%%%ds", GNAME_MAX - 1);
    char semantic_label[GNAME_MAX];
    int semantic_label_count = 0, semantic_label_match_count = 0;
    while (fscanf(misc_file, frmt, semantic_label) == 1) {
        semantic_label_count++;
        bool found = false;

        for (int n = 0; n < group_ref.nfiles; n++) {
            if (semantic_label[0] != '\n' &&
                strcmp(semantic_label, semantic_labels_group[n]) == 0) {
                semantic_label_match_count++;
                found = true;
                names_ordered[n] = group_ref.file[n].name;
                mapsets_ordered[n] = group_ref.file[n].mapset;
                break;
            }
        }
        if (!found)
            G_fatal_error(_("Imagery group does not contain a raster with a "
                            "semantic label '%s'"),
                          semantic_label);
    }
    fclose(misc_file);
    if (semantic_label_match_count != semantic_label_count ||
        semantic_label_match_count != group_ref.nfiles) {
        G_fatal_error(_("Unable to match all signature file bands to imagery "
                        "group bands. "
                        "Signature band count: %d, imagery group band count: "
                        "%d, band match count: %d."),
                      semantic_label_count, group_ref.nfiles,
                      semantic_label_match_count);
    }

    /* Read rescaling parameters */
    DCELL *means, *ranges, mean, range;
    int scale_count = 0;
    misc_file =
        G_fopen_old_misc(sigfile_dir, "scale", name_sigfile, mapset_sigfile);
    if (!misc_file)
        G_fatal_error(_("Unable to read signature file '%s'."), name_sigfile);
    means = G_malloc(group_ref.nfiles * sizeof(DCELL));
    ranges = G_malloc(group_ref.nfiles * sizeof(DCELL));
    while (fscanf(misc_file, "%lf %lf", &mean, &range) == 2) {
        if (scale_count >= group_ref.nfiles)
            G_fatal_error(_("Unable to read signature file '%s'."),
                          name_sigfile);
        means[scale_count] = mean;
        ranges[scale_count] = range;
        if (range == 0)
            G_fatal_error(_("Unable to read signature file '%s'."),
                          name_sigfile);
        scale_count++;
    }
    fclose(misc_file);
    if (scale_count != group_ref.nfiles)
        G_fatal_error(_("Unable to read signature file '%s'."), name_sigfile);

    /* Pass LIBSVM messages through GRASS */
    svm_set_print_string_function(&print_func);

    /* Load trained model from a file */
    struct svm_model *model;
    G_verbose_message("Reading in trained SVM");
    G_file_name_misc(model_file, sigfile_dir, "sig", name_sigfile,
                     mapset_sigfile);
    model = svm_load_model(model_file);
    if (model == NULL)
        G_fatal_error(_("Unable to open trained model file <%s>"),
                      name_sigfile);

    G_message(_("Starting value prediction process"));
    /* For row, cell: svm_predict */
    int row, col, band;
    int nrows, ncols;
    int svm_type;
    int fd_values = 0;
    RASTER_MAP_TYPE out_type;

    int *fd_bands;
    DCELL **buf_bands;
    struct svm_node *nodes;

    svm_type = svm_get_svm_type(model);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    buf_bands = (DCELL **)G_malloc(group_ref.nfiles * sizeof(DCELL *));
    fd_bands = (int *)G_calloc(group_ref.nfiles, sizeof(int));
    for (band = 0; band < group_ref.nfiles; band++) {
        buf_bands[band] = Rast_allocate_d_buf();
        fd_bands[band] =
            Rast_open_old(names_ordered[band], mapsets_ordered[band]);
    }
    nodes = (struct svm_node *)G_malloc(((size_t)group_ref.nfiles + 1) *
                                        sizeof(struct svm_node));

    /* Predict a class (C_SVC, NU_SVC, ONE_CLASS)
     * Other SVM types calculate a value */
    if (svm_type == C_SVC || svm_type == NU_SVC || svm_type == ONE_CLASS) {
        CELL *out_row;
        DCELL val;

        out_row = Rast_allocate_c_buf();
        out_type = CELL_TYPE;
        fd_values = Rast_open_c_new(name_values);

        for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);
            for (band = 0; band < group_ref.nfiles; band++)
                Rast_get_d_row(fd_bands[band], &buf_bands[band][0], row);
            for (col = 0; col < ncols; col++) {
                nodes[0].index = -1;
                for (band = 0; band < group_ref.nfiles; band++) {
                    if (Rast_is_d_null_value(&buf_bands[band][col]))
                        continue;
                    nodes[band].index = band;
                    nodes[band].value =
                        (buf_bands[band][col] - means[band]) / ranges[band];
                }

                /* All values where NULLs */
                if (nodes[0].index == -1) {
                    Rast_set_c_null_value(&out_row[col], 1);
                    continue;
                }
                /* Mark the end of values in nodes */
                nodes[group_ref.nfiles].index = -1;

                val = svm_predict(model, nodes);
                out_row[col] = (CELL)val;
            }
            Rast_put_row(fd_values, out_row, out_type);
        }
        G_percent(1, 1, 1);
        G_free(out_row);
    }
    else {
        DCELL *out_row;
        DCELL val;

        out_row = Rast_allocate_d_buf();
        out_type = DCELL_TYPE;
        fd_values = Rast_open_fp_new(name_values);

        for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);
            for (band = 0; band < group_ref.nfiles; band++)
                Rast_get_d_row(fd_bands[band], &buf_bands[band][0], row);
            for (col = 0; col < ncols; col++) {
                nodes[0].index = -1;
                for (band = 0; band < group_ref.nfiles; band++) {
                    if (Rast_is_d_null_value(&buf_bands[band][col]))
                        continue;
                    nodes[band].index = band;
                    nodes[band].value =
                        (buf_bands[band][col] - means[band]) / ranges[band];
                }

                /* All values where NULLs */
                if (nodes[0].index == -1) {
                    Rast_set_d_null_value(&out_row[col], 1);
                    continue;
                }
                /* Mark the end of values in nodes */
                nodes[group_ref.nfiles].index = -1;

                val = svm_predict(model, nodes);
                out_row[col] = val;
            }
            Rast_put_row(fd_values, out_row, out_type);
        }
        G_percent(1, 1, 1);
        G_free(out_row);
    }

    /* Clean up */
    Rast_close(fd_values);
    for (band = 0; band < group_ref.nfiles; band++) {
        Rast_close(fd_bands[band]);
        G_free(buf_bands[band]);
    }
    G_free(nodes);
    G_free(means);
    G_free(ranges);

    /* Try to give full history */
    struct History history;
    G_verbose_message("Writing out history");
    Rast_short_history(name_values, "raster", &history);
    misc_file =
        G_fopen_old_misc(sigfile_dir, "history", name_sigfile, mapset_sigfile);
    if (misc_file != NULL) {
        char hist_line[4096]; /* history lines are limited to 4096 */

        while (G_getl(hist_line, sizeof(hist_line), misc_file) == 1) {
            Rast_append_history(&history, hist_line);
        }
        fclose(misc_file);
    }
    Rast_command_history(&history);
    if (opt_subgroup->answer)
        Rast_format_history(&history, HIST_DATSRC_1, "Group/subgroup: %s@%s/%s",
                            name_group, mapset_group, opt_subgroup->answer);
    else
        Rast_format_history(&history, HIST_DATSRC_1, "Group: %s@%s", name_group,
                            mapset_group);
    Rast_format_history(&history, HIST_DATSRC_2, "Signature file: %s@%s",
                        name_sigfile, mapset_sigfile);
    Rast_write_history(name_values, &history);

    if (svm_type != ONE_CLASS) {
        char in_path[GPATH_MAX], out_path[GPATH_MAX];

        /* Copy CATs file from the original training map */
        G_verbose_message("Copying category information");
        G_file_name_misc(in_path, sigfile_dir, "cats", name_sigfile,
                         mapset_sigfile);
        /* Avoid warnings if file does not exist */
        if (access(in_path, 0) == 0) {
            G_file_name(out_path, "cats", name_values, G_mapset());
            G_copy_file(in_path, out_path);
        }

        /* Copy color file from the original training map */
        G_verbose_message("Copying color information");
        G_file_name_misc(in_path, sigfile_dir, "colr", name_sigfile,
                         mapset_sigfile);
        if (access(in_path, 0) == 0) {
            G_file_name(out_path, "colr", name_values, G_mapset());
            G_copy_file(in_path, out_path);
        }
    }
    Rast_put_cell_title(name_values,
                        /* GTC: A map title */
                        _("Values predicted with a Support Vector Machine"));

    exit(EXIT_SUCCESS);
}
