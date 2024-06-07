/****************************************************************************
 *
 * MODULE:       r.texture
 * AUTHOR(S):    Carmine Basco - basco@unisannio.it
 *               with hints from:
 *                         prof. Giulio Antoniol - antoniol@ieee.org
 *                         prof. Michele Ceccarelli - ceccarelli@unisannio.it
 *               Markus Metz (optimization and bug fixes)
 *
 * PURPOSE:      Create map raster with textural features.
 *
 * COPYRIGHT:    (C) 2003 by University of Sannio (BN), Benevento, Italy
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted. This
 * software is provided "as is" without express or implied warranty.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "h_measure.h"

/* modify this table to add new measures */
static struct menu measure_menu[] = {
    {"asm", "Angular Second Moment", "_ASM", 0, 1},
    {"contrast", "Contrast", "_Contr", 0, 2},
    {"corr", "Correlation", "_Corr", 0, 3},
    {"var", "Variance", "_Var", 0, 4},
    {"idm", "Inverse Diff Moment", "_IDM", 0, 5},
    {"sa", "Sum Average", "_SA", 0, 6},
    {"sv", "Sum Variance", "_SV", 0, 7},
    {"se", "Sum Entropy", "_SE", 0, 8},
    {"entr", "Entropy", "_Entr", 0, 9},
    {"dv", "Difference Variance", "_DV", 0, 10},
    {"de", "Difference Entropy", "_DE", 0, 11},
    {"moc1", "Measure of Correlation-1", "_MOC-1", 0, 12},
    {"moc2", "Measure of Correlation-2", "_MOC-2", 0, 13},
    {NULL, NULL, NULL, 0, -1}};

static int find_measure(const char *measure_name)
{
    int i;

    for (i = 0; measure_menu[i].name; i++)
        if (strcmp(measure_menu[i].name, measure_name) == 0)
            return i;

    G_fatal_error(_("Unknown measure <%s>"), measure_name);

    return -1;
}

int main(int argc, char *argv[])
{
    struct Cell_head cellhd;
    char *name, *result;
    char **mapname;
    int n_measures, n_outputs, *measure_idx, overwrite;
    int nrows, ncols;
    int row, col, first_row, last_row, first_col, last_col;
    int i, j;
    CELL **data; /* Data structure containing image */
    DCELL *dcell_row;
    struct FPRange range;
    DCELL min, max, inscale;
    int dist, size; /* dist = value of distance, size = s. of moving window */
    int offset;
    int have_px, have_py, have_pxpys, have_pxpyd;
    int infd, *outfd;
    int threads;
    RASTER_MAP_TYPE out_data_type;
    struct GModule *module;
    struct Option *opt_input, *opt_output, *opt_size, *opt_dist, *opt_measure,
        *opt_threads;
    struct Flag *flag_ind, *flag_all, *flag_null;
    struct History history;
    struct dimensions *dim;
    struct output_setting *out_set;
    char p[1024];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("algebra"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("texture"));
    module->description =
        _("Generate images with textural features from a raster map.");

    /* Define the different options */

    opt_input = G_define_standard_option(G_OPT_R_INPUT);

    opt_output = G_define_standard_option(G_OPT_R_BASENAME_OUTPUT);

    opt_threads = G_define_standard_option(G_OPT_M_NPROCS);

    opt_size = G_define_option();
    opt_size->key = "size";
    opt_size->key_desc = "value";
    opt_size->type = TYPE_INTEGER;
    opt_size->required = NO;
    opt_size->description = _("The size of moving window (odd and >= 3)");
    opt_size->answer = "3";

    /* Textural character is in direct relation of the spatial size of the
     * texture primitives. */

    opt_dist = G_define_option();
    opt_dist->key = "distance";
    opt_dist->key_desc = "value";
    opt_dist->type = TYPE_INTEGER;
    opt_dist->required = NO;
    opt_dist->label = _("The distance between two samples (>= 1)");
    opt_dist->description =
        _("The distance must be smaller than the size of the moving window");
    opt_dist->answer = "1";

    for (i = 0; measure_menu[i].name; i++) {
        if (i)
            strcat(p, ",");
        else
            *p = 0;
        strcat(p, measure_menu[i].name);
    }
    opt_measure = G_define_option();
    opt_measure->key = "method";
    opt_measure->type = TYPE_STRING;
    opt_measure->required = NO;
    opt_measure->multiple = YES;
    opt_measure->options = p;
    opt_measure->description = _("Textural measurement method");

    flag_ind = G_define_flag();
    flag_ind->key = 's';
    flag_ind->label = _("Separate output for each angle (0, 45, 90, 135)");
    flag_ind->description =
        _("Angles are counterclockwise from east: "
          "0 is East to West, 45 is North-East to South-West");

    flag_all = G_define_flag();
    flag_all->key = 'a';
    flag_all->description = _("Calculate all textural measurements");

    flag_null = G_define_flag();
    flag_null->key = 'n';
    flag_null->label = _("Allow NULL cells in a moving window");
    flag_null->description =
        _("This will also avoid cropping along edges of the current region");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    name = opt_input->answer;
    result = opt_output->answer;
    size = atoi(opt_size->answer);
    if (size <= 0)
        G_fatal_error(_("Size of the moving window must be > 0"));
    if (size % 2 != 1)
        G_fatal_error(_("Size of the moving window must be odd"));
    dist = atoi(opt_dist->answer);
    if (dist <= 0)
        G_fatal_error(_("The distance between two samples must be > 0"));
    if (dist >= size)
        G_fatal_error(_("The distance between two samples must be smaller than "
                        "the size of the moving window"));

    n_measures = 0;
    if (flag_all->answer) {
        for (i = 0; measure_menu[i].name; i++) {
            measure_menu[i].useme = 1;
        }
        n_measures = i;
    }
    else {
        for (i = 0; opt_measure->answers[i]; i++) {
            if (opt_measure->answers[i]) {
                const char *measure_name = opt_measure->answers[i];
                int n = find_measure(measure_name);

                measure_menu[n].useme = 1;
                n_measures++;
            }
        }
    }
    if (!n_measures)
        G_fatal_error(
            _("Nothing to compute. Use at least one textural measure."));

    measure_idx = G_malloc(n_measures * sizeof(int));
    j = 0;
    for (i = 0; measure_menu[i].name; i++) {
        if (measure_menu[i].useme == 1) {
            measure_idx[j] = i;
            j++;
        }
    }

    infd = Rast_open_old(name, "");

    Rast_get_cellhd(name, "", &cellhd);

    out_data_type = FCELL_TYPE;
    /* Allocate output buffers, use FCELL data_type */
    n_outputs = n_measures;
    if (flag_ind->answer) {
        n_outputs = n_measures * 4;
    }

    mapname = G_malloc(n_outputs * sizeof(char *));
    for (i = 0; i < n_outputs; i++)
        mapname[i] = G_malloc(GNAME_MAX * sizeof(char));

    overwrite = G_check_overwrite(argc, argv);

    /* open output maps */
    outfd = G_malloc(n_outputs * sizeof(int));
    for (i = 0; i < n_measures; i++) {
        if (flag_ind->answer) {
            for (j = 0; j < 4; j++) {
                sprintf(mapname[i * 4 + j], "%s%s_%d", result,
                        measure_menu[measure_idx[i]].suffix, j * 45);
                if (!G_find_raster(mapname[i * 4 + j], G_mapset()) ||
                    overwrite) {
                    outfd[i * 4 + j] =
                        Rast_open_new(mapname[i * 4 + j], out_data_type);
                }
                else {
                    G_fatal_error(_("At least one of the requested output maps "
                                    "exists. Use --o to overwrite."));
                }
            }
        }
        else {
            sprintf(mapname[i], "%s%s", result,
                    measure_menu[measure_idx[i]].suffix);
            if (!G_find_raster(mapname[i], G_mapset()) || overwrite) {
                outfd[i] = Rast_open_new(mapname[i], out_data_type);
            }
            else {
                G_fatal_error(_("At least one of the requested output maps "
                                "exists. Use --o to overwrite."));
            }
        }
    }
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* Load raster map. */

    /* allocate the space for one row of cell map data *A* */
    dcell_row = Rast_allocate_d_buf();

    /* Allocate appropriate memory for the structure containing the image */
    data = (int **)G_malloc(nrows * sizeof(int *));
    for (i = 0; i < nrows; i++) {
        data[i] = (int *)G_malloc(ncols * sizeof(int));
    }

    /* read input range */
    Rast_init_fp_range(&range);
    Rast_read_fp_range(name, "", &range);
    Rast_get_fp_range_min_max(&range, &min, &max);
    inscale = 0;
    if (min < 0 || max > 255) {
        inscale = 255. / (max - min);
    }
    /* input has 0 - 1 range */
    else if (max <= 1.) {
        inscale = 255. / (max - min);
    }

    /* Read in cell map values */
    /* TODO: use r.proj cache */
    G_important_message(_("Reading raster map..."));
    for (j = 0; j < nrows; j++) {
        Rast_get_row(infd, dcell_row, j, DCELL_TYPE);
        for (i = 0; i < ncols; i++) {
            if (Rast_is_d_null_value(&(dcell_row[i])))
                data[j][i] = -1;
            else if (inscale) {
                data[j][i] = (CELL)((dcell_row[i] - min) * inscale);
            }
            else
                data[j][i] = (CELL)dcell_row[i];
        }
    }

    /* close input cell map and release the row buffer */
    Rast_close(infd);
    G_free(dcell_row);

    /* variables needed */
    dim = G_malloc(sizeof(struct dimensions));
    out_set = G_malloc(sizeof(struct output_setting));

    dim->size = size;
    dim->dist = dist;
    dim->nrows = nrows;
    dim->ncols = ncols;
    dim->n_outputs = n_outputs;
    dim->n_measures = n_measures;

    out_set->outfd = outfd;
    out_set->out_data_type = out_data_type;
    out_set->flag_null = flag_null;
    out_set->flag_ind = flag_ind;

    execute_texture(data, dim, measure_menu, measure_idx, out_set);

    for (i = 0; i < n_outputs; i++) {
        Rast_close(outfd[i]);
        Rast_short_history(mapname[i], "raster", &history);
        Rast_command_history(&history);
        Rast_write_history(mapname[i], &history);
    }

    /* Free allocated memory */
    for (i = 0; i < n_outputs; i++)
        G_free(mapname[i]);
    for (i = 0; i < nrows; i++)
        G_free(data[i]);

    G_free(dim);
    G_free(out_set);
    G_free(measure_idx);
    G_free(mapname);
    G_free(data);

    exit(EXIT_SUCCESS);
}
