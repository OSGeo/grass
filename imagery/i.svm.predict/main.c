
/****************************************************************************
 *
 * MODULE:       i.svm.predict
 * AUTHOR(S):    Maris Nartiss - maris.gis gmail.com
 * PURPOSE:      Predicts values with Support Vector Machine classifier
 *
 * COPYRIGHT:    (C) 2020 by Maris Nartiss and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <libsvm/svm.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include "fill.h"

/* LIBSVM message wrapper */
void print_func(const char *s)
{
    G_verbose_message("%s", s);
};


int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *opt_group, *opt_subgroup, *opt_sigfile, *opt_values;
    struct Option *opt_svm_cache_size;

    char name_values[GNAME_MAX], name_group[GNAME_MAX],
        name_subgroup[GNAME_MAX];
    const char *mapset_model;
    char mapset_values[GMAPSET_MAX], mapset_group[GMAPSET_MAX],
        mapset_subgroup[GMAPSET_MAX];
    char element[GPATH_MAX], model_file[GPATH_MAX];

    struct Ref band_ref;

    struct svm_model *model;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("svm"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("prediction"));
    G_add_keyword(_("regression"));
    module->description = _("Predict with SVM");

    opt_group = G_define_standard_option(G_OPT_I_GROUP);
    /* GTC: SVM prediction input */
    opt_group->description = _("Maps with feature values (attributes)");

    opt_subgroup = G_define_standard_option(G_OPT_I_SUBGROUP);
    opt_subgroup->required = NO;

    opt_sigfile = G_define_option();
    opt_sigfile->key = "model";
    opt_sigfile->type = TYPE_STRING;
    opt_sigfile->key_desc = "name";
    opt_sigfile->required = YES;
    opt_sigfile->gisprompt = "old,svm,sigfile";
    opt_sigfile->description = _("Name trained SVM model");

    opt_values = G_define_standard_option(G_OPT_R_OUTPUT);
    opt_values->required = YES;
    opt_values->description =
        _("Output map with predicted class / calculated value");

    opt_svm_cache_size = G_define_option();
    opt_svm_cache_size->key = "cache";
    opt_svm_cache_size->type = TYPE_INTEGER;
    opt_svm_cache_size->key_desc = "cache size";
    opt_svm_cache_size->required = NO;
    opt_svm_cache_size->options = "1-999999999";
    opt_svm_cache_size->answer = "512";
    opt_svm_cache_size->description = _("Kernel cache size in MB");
    /* opt_svm_cache_size->guisection = _("SVM options"); */


    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* Input validation */
    /* Input maps */
    if (G_unqualified_name(opt_group->answer, NULL, name_group, mapset_group)
        == 0)
        strcpy(mapset_group, G_mapset());
    if (opt_subgroup->answer &&
        G_unqualified_name(opt_subgroup->answer, NULL, name_subgroup,
                           mapset_subgroup) != 0 &&
        strcmp(mapset_subgroup, mapset_group) != 0)
        G_fatal_error(_("Invalid subgroup <%s> provided"),
                      opt_subgroup->answer);
    if (!I_find_group2(name_group, mapset_group)) {
        G_fatal_error(_("Group <%s> not found in mapset <%s>"),
                      name_group, mapset_group);
    }
    if (opt_subgroup->answer &&
        !I_find_subgroup2(name_group, name_subgroup, mapset_group)) {
        G_fatal_error(_("Subgroup <%s> in group <%s@%s> not found"),
                      name_subgroup, name_group, mapset_group);
    }

    if (opt_subgroup->answer)
        sprintf(element, "subgroup%c%s%csvm%c%s", HOST_DIRSEP, name_subgroup,
                HOST_DIRSEP, HOST_DIRSEP, opt_sigfile->answer);
    else
        sprintf(element, "svm%c%s", HOST_DIRSEP, opt_sigfile->answer);
    mapset_model = G_find_file2_misc("group", element, name_group, "");
    if (mapset_model == NULL)
        G_fatal_error(_("File <%s> with trained SVM model not found"),
                      opt_sigfile->answer);

    if (G_unqualified_name
        (opt_values->answer, G_mapset(), name_values, mapset_values) < 0)
        G_fatal_error(_("<%s> does not match the current mapset"),
                      mapset_values);
    if (G_legal_filename(name_values) < 0)
        G_fatal_error(_("<%s> is an illegal file name"), name_values);

    /* Get bands */
    if (opt_subgroup->answer) {
        if (!I_get_subgroup_ref2
            (name_group, opt_subgroup->answer, mapset_group, &band_ref)) {
            G_fatal_error(_("There was an error reading subgroup <%s> in group <%s@%s>"),
                          opt_subgroup->answer, name_group, mapset_group);
        }
    }
    else {
        if (!I_get_group_ref2(name_group, mapset_group, &band_ref)) {
            G_fatal_error(_("There was an error reading group <%s@%s>"),
                          name_group, mapset_group);
        }
    }
    if (band_ref.nfiles <= 0) {
        if (opt_subgroup->answer)
            G_fatal_error(_("Subgroup <%s> in group <%s@%s> contains no raster maps."),
                          opt_subgroup->answer, name_group, mapset_group);
        else
            G_fatal_error(_("Group <%s@%s> contains no raster maps."),
                          name_group, mapset_group);
    }

    svm_set_print_string_function(&print_func);

    /* Load trained model from a file */
    /* TODO: move to imagery lib? */
    if (opt_subgroup->answer)
        sprintf(element, "group%c%s%csubgroup%c%s%csvm", HOST_DIRSEP,
                name_group, HOST_DIRSEP, HOST_DIRSEP, opt_subgroup->answer,
                HOST_DIRSEP);
    else
        sprintf(element, "group%c%s%csvm", HOST_DIRSEP, name_group,
                HOST_DIRSEP);
    G_file_name_misc(model_file, NULL, element, opt_sigfile->answer,
                     mapset_model);
    model = svm_load_model(model_file);
    if (model == NULL)
        G_fatal_error(_("Unable to open trained model file <%s>"),
                      opt_sigfile->answer);

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

    buf_bands = (DCELL **) G_malloc(band_ref.nfiles * sizeof(DCELL *));
    fd_bands = (int *)G_calloc(band_ref.nfiles, sizeof(int));
    for (band = 0; band < band_ref.nfiles; band++) {
        buf_bands[band] = Rast_allocate_d_buf();
        fd_bands[band] =
            Rast_open_old(band_ref.file[band].name,
                          band_ref.file[band].mapset);
    }
    nodes =
        (struct svm_node *)G_malloc(((size_t)band_ref.nfiles + 1) *
                                    sizeof(struct svm_node));

    /* Predict a class or calculate a value */
    if (svm_type == C_SVC || svm_type == NU_SVC || svm_type == ONE_CLASS) {
        CELL *out_row;
        DCELL val;

        out_row = Rast_allocate_c_buf();
        out_type = CELL_TYPE;
        fd_values = Rast_open_c_new(name_values);

        for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);
            for (band = 0; band < band_ref.nfiles; band++)
                Rast_get_d_row(fd_bands[band], &buf_bands[band][0], row);
            for (col = 0; col < ncols; col++) {
                nodes[0].index = -1;
                for (band = 0; band < band_ref.nfiles; band++) {
                    if (Rast_is_d_null_value(&buf_bands[band][col]))
                        continue;
                    nodes[band].index = band;
                    nodes[band].value = buf_bands[band][col];
                }

                /* All values where NULLs */
                if (nodes[0].index == -1) {
                    Rast_set_c_null_value(&out_row[col], 1);
                    continue;
                }
                /*
                   for (band = 0; band < (band_ref.nfiles + 1); band++) {
                   if (nodes[band].index = -1)
                   continue;
                   printf("push[%d][%d][%d]=%f ", row, col, band, nodes[band].value);
                   }
                   * */
                val = svm_predict(model, nodes);
                out_row[col] = (CELL) val;
                //printf(" => %f\n", val);
            }
            Rast_put_row(fd_values, out_row, out_type);
        }
        G_percent(nrows, nrows, 2);
    }
    else {
        /*
           DCELL *out_row;
           out_row = Rast_allocate_d_buf();
           out_type = DCELL_TYPE;
           fd_values = Rast_open_d_new(name_values);
         */
    }

    /* TODO:
     *  CATs
     *  History
     */

    /* Clean up */
    Rast_close(fd_values);
    for (band = 0; band < band_ref.nfiles; band++) {
        Rast_close(fd_bands[band]);
        G_free(buf_bands[band]);
    }

    exit(EXIT_SUCCESS);
}
