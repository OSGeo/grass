/****************************************************************************
 *
 * MODULE:       r.patch
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor),
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Markus Neteler <neteler itc.it>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>,
 *               Jan-Oliver Wagner <jan intevation.de>,
 *               Huidae Cho <grass4u gmail.com>,
 *               Aaron Saw Min Sern (OpenMP parallelization)
 * PURPOSE:
 * COPYRIGHT:    (C) 1999-2022 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#if defined(_OPENMP)
#include <omp.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    int **infd;
    struct Categories cats;
    struct Cell_stats **thread_statf;
    struct Colors colr;
    int cats_ok;
    int colr_ok;
    int outfd;
    RASTER_MAP_TYPE out_type, map_type;
    size_t out_cell_size, in_buf_size, out_buf_size;
    struct History history;
    void **presult, **patch;
    void *outbuf;
    int bufrows;
    int nfiles;
    char *rname;
    int i, t;
    int nprocs;
    int row, nrows, ncols;
    int use_zero, no_support;
    char *new_name;
    char **names;
    char **ptr;
    struct Cell_head window;
    struct Cell_head *cellhd;

    struct GModule *module;
    struct Flag *zeroflag, *nosupportflag;
    struct Option *opt1, *opt2, *threads, *memory;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("mosaicking"));
    G_add_keyword(_("merge"));
    G_add_keyword(_("patching"));
    G_add_keyword(_("aggregation"));
    G_add_keyword(_("series"));
    G_add_keyword(_("parallel"));
    module->description =
        _("Creates a composite raster map layer by using "
          "known category values from one (or more) map layer(s) "
          "to fill in areas of \"no data\" in another map layer.");

    /* Define the different options */

    opt1 = G_define_standard_option(G_OPT_R_INPUTS);
    opt1->description = _("Name of raster maps to be patched together");

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt2->description = _("Name for resultant raster map");

    threads = G_define_standard_option(G_OPT_M_NPROCS);
    memory = G_define_standard_option(G_OPT_MEMORYMB);

    /* Define the different flags */

    zeroflag = G_define_flag();
    zeroflag->key = 'z';
    zeroflag->description = _("Use zero (0) for transparency instead of NULL");

    nosupportflag = G_define_flag();
    nosupportflag->key = 's';
    nosupportflag->description = _("Do not create color and category files");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    nprocs = G_set_omp_num_threads(threads);
    nprocs = Rast_disable_omp_on_mask(nprocs);
    if (nprocs < 1)
        G_fatal_error(_("<%d> is not valid number of nprocs."), nprocs);

    use_zero = (zeroflag->answer);
    no_support = (nosupportflag->answer);

    names = opt1->answers;

    out_type = CELL_TYPE;

    for (ptr = names, nfiles = 0; *ptr != NULL; ptr++, nfiles++)
        ;

    if (nfiles < 2)
        G_fatal_error(_("The minimum number of input raster maps is two"));

    infd = G_malloc(nprocs * sizeof(int *));
    for (t = 0; t < nprocs; t++)
        infd[t] = G_malloc(nfiles * sizeof(int));
    thread_statf = G_malloc(nprocs * (nfiles * sizeof(struct Cell_stats)));
    for (t = 0; t < nprocs; t++) {
        thread_statf[t] = G_malloc(nfiles * sizeof(struct Cell_stats));
    }
    cellhd = G_malloc(nfiles * sizeof(struct Cell_head));

    for (i = 0; i < nfiles; i++) {
        const char *name = names[i];
        int fd;

        for (t = 0; t < nprocs; t++) {
            infd[t][i] = Rast_open_old(name, "");
        }

        fd = infd[0][i];

        map_type = Rast_get_map_type(fd);
        if (map_type == FCELL_TYPE && out_type == CELL_TYPE)
            out_type = FCELL_TYPE;
        else if (map_type == DCELL_TYPE)
            out_type = DCELL_TYPE;

        for (t = 0; t < nprocs; t++) {
            Rast_init_cell_stats(&thread_statf[t][i]);
        }

        Rast_get_cellhd(name, "", &cellhd[i]);
    }
    out_cell_size = Rast_cell_size(out_type);

    rname = opt2->answer;
    outfd = Rast_open_new(new_name = rname, out_type);

    presult = G_malloc(nprocs * sizeof *presult);
    patch = G_malloc(nprocs * sizeof *patch);
    for (t = 0; t < nprocs; t++) {
        presult[t] = Rast_allocate_buf(out_type);
    }
    for (t = 0; t < nprocs; t++) {
        patch[t] = Rast_allocate_buf(out_type);
    }

    Rast_get_window(&window);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* memory reserved for presult and patch */
    in_buf_size = out_cell_size * ncols * nprocs * 2;
    /* memory available for output buffer */
    out_buf_size = (size_t)atoi(memory->answer) * (1 << 20);
    /* size_t is unsigned, check if any memory is left for output buffer */
    if (out_buf_size <= in_buf_size)
        out_buf_size = 0;
    else
        out_buf_size -= in_buf_size;
    /* number of buffered output rows */
    bufrows = out_buf_size / (out_cell_size * ncols);
    /* set the output buffer rows to be at most covering the entire map */
    if (bufrows > nrows) {
        bufrows = nrows;
    }
    /* but at least the number of threads */
    if (bufrows < nprocs) {
        bufrows = nprocs;
    }

    outbuf = G_malloc(out_cell_size * ncols * bufrows);

    G_verbose_message(_("Percent complete..."));

    int computed = 0;
    int written = 0;

    while (written < nrows) {
        int start = written;
        int end = start + bufrows;

        if (end > nrows)
            end = nrows;

#pragma omp parallel private(i) if (nprocs > 1)
        {
            int t_id = 0;

#if defined(_OPENMP)
            t_id = omp_get_thread_num();
#endif
            void *local_presult = presult[t_id];
            void *local_patch = patch[t_id];
            int *local_infd = infd[t_id];

#pragma omp for schedule(static)
            for (row = start; row < end; row++) {
                double north_edge, south_edge;

                G_percent(computed, nrows, 2);
                Rast_get_row(local_infd[0], local_presult, row, out_type);

                north_edge = Rast_row_to_northing(row, &window);
                south_edge = north_edge - window.ns_res;

                if (out_type == CELL_TYPE && !no_support) {
                    Rast_update_cell_stats((CELL *)local_presult, ncols,
                                           &thread_statf[t_id][0]);
                }
                for (i = 1; i < nfiles; i++) {
                    /* check if raster i overlaps with the current row */
                    if (south_edge >= cellhd[i].north ||
                        north_edge <= cellhd[i].south ||
                        window.west >= cellhd[i].east ||
                        window.east <= cellhd[i].west)
                        continue;

                    Rast_get_row(local_infd[i], local_patch, row, out_type);
                    if (!do_patch(local_presult, local_patch,
                                  &(thread_statf[t_id][i]), ncols, out_type,
                                  out_cell_size, use_zero, no_support))
                        break;
                }
                void *p = G_incr_void_ptr(outbuf, out_cell_size *
                                                      (row - start) * ncols);
                memcpy(p, local_presult, out_cell_size * ncols);

#pragma omp atomic update
                computed++;
            }

        } /* end parallel region */

        for (row = start; row < end; row++) {
            void *p =
                G_incr_void_ptr(outbuf, out_cell_size * (row - start) * ncols);
            Rast_put_row(outfd, p, out_type);
        }

        written = end;

    } /* end while loop */
    G_percent(nrows, nrows, 2);

    for (t = 0; t < nprocs; t++) {
        G_free(patch[t]);
        G_free(presult[t]);
    }
    G_free(patch);
    G_free(presult);

    for (t = 0; t < nprocs; t++)
        for (i = 0; i < nfiles; i++)
            Rast_close(infd[t][i]);

    if (!no_support) {
        /*
         * build the new cats and colors. do this before closing the new
         * file, in case the new file is one of the patching files as well.
         */
        G_verbose_message(_("Creating support files for raster map <%s>..."),
                          new_name);

        if (out_type == CELL_TYPE) {
            merge_threads(thread_statf, nprocs, nfiles);
        }

        support(names, thread_statf[0], nfiles, &cats, &cats_ok, &colr,
                &colr_ok, out_type);
    }

    for (t = 0; t < nprocs; t++) {
        G_free(thread_statf[t]);
    }
    G_free(thread_statf);

    /* now close (and create) the result */
    Rast_close(outfd);
    if (!no_support) {
        if (cats_ok)
            Rast_write_cats(new_name, &cats);
        if (colr_ok)
            Rast_write_colors(new_name, G_mapset(), &colr);
    }

    Rast_short_history(new_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(new_name, &history);

    exit(EXIT_SUCCESS);
}
