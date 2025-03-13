/*
 * i.svm.train Functions filling svm_problem struct
 *
 *   Copyright 2023 by Maris Nartiss, and the GRASS Development Team
 *   Author: Maris Nartiss
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */
#include <grass/raster.h>
#include <grass/glocale.h>

#include "fill.h"

void fill_problem(const char *name_labels, const char *mapset_labels,
                  struct Ref band_refs, const DCELL *means, const DCELL *ranges,
                  struct svm_problem *problem)
{
    /* Keep track of used svm_problem node head */
    int label_num = 0;
    int label_max = 0;
    problem->l = 0;
    problem->x = NULL;
    problem->y = NULL;

    int nrows = Rast_window_rows();
    int ncols = Rast_window_cols();

    int fd_labels = Rast_open_old(name_labels, mapset_labels);
    /* svm_problem always stores labels as doubles */
    DCELL *buf_labels = Rast_allocate_d_buf();

    DCELL **buf_bands = (DCELL **)G_malloc(band_refs.nfiles * sizeof(DCELL *));
    int *fd_bands = (int *)G_calloc(band_refs.nfiles, sizeof(int));
    int band;
    for (band = 0; band < band_refs.nfiles; band++) {
        buf_bands[band] = Rast_allocate_d_buf();
        fd_bands[band] = Rast_open_old(band_refs.file[band].name,
                                       band_refs.file[band].mapset);
    }

    for (int row = 0; row < nrows; row++) {
        G_percent(row, nrows, 10);
        Rast_get_d_row(fd_labels, buf_labels, row);
        for (band = 0; band < band_refs.nfiles; band++)
            Rast_get_d_row(fd_bands[band], &buf_bands[band][0], row);
        for (int col = 0; col < ncols; col++) {
            if (Rast_is_d_null_value(&buf_labels[col]))
                continue;
            if (label_num >= label_max) {
                label_max += SIZE_INCREMENT;
                problem->y =
                    G_realloc(problem->y, (size_t)label_max * sizeof(double));
                problem->x = G_realloc(
                    problem->x, (size_t)label_max * sizeof(struct svm_node *));
            }
            problem->l = label_num;
            problem->y[label_num] = buf_labels[col];
            problem->x[label_num] = NULL;
            int value_num = 0;
            int value_max = 0;
            for (band = 0; band < band_refs.nfiles; band++) {
                if (Rast_is_d_null_value(&buf_bands[band][col]))
                    continue;
                if (value_num >= value_max) {
                    /* Three bands are typical, thus we start with 4 nodes */
                    value_max += 4;
                    problem->x[label_num] = G_realloc(
                        problem->x[label_num],
                        ((size_t)value_max + 1) * sizeof(struct svm_node));
                }
                problem->x[label_num][value_num].index = band;
                problem->x[label_num][value_num].value =
                    (buf_bands[band][col] - means[band]) / ranges[band];
                value_num++;
            }
            /* If label has no data */
            if (value_num == 0) {
                continue;
            }
            problem->x[label_num][value_num].index = -1;
            label_num++;
        }
    }

    /* Although there could be more memory allocated, not all might be filled */
    problem->l = label_num;

    /* Clean up */
    Rast_close(fd_labels);
    G_free(buf_labels);

    for (band = 0; band < band_refs.nfiles; band++) {
        Rast_close(fd_bands[band]);
        G_free(buf_bands[band]);
    }
    G_free(fd_bands);
    G_free(buf_bands);
    G_percent(1, 1, 1);
    G_percent_reset();
}
