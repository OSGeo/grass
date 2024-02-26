/****************************************************************************
 *
 * MODULE:       r.smooth
 * AUTHOR(S):    Maris Nartiss maris.gis gmail.com
 * PURPOSE:      Provides smoothing with anisotropic diffusion according to:
 *               Perona P. and Malik J. 1990. Scale-space and edge detection
 *               using anisotropic diffusion. IEEE transactions on pattern
 *               analysis and machine intelligence, 12(7).
 *               Tukey's conductance function according to:
 *               Black M.J., Sapiro G., Marimont D.H. and Heeger D. 1998.
 *               Robust anisotropic diffusion. IEEE transactions on image
 *               processing, 7(3).
 *
 * COPYRIGHT:    (C) 2024 by Maris Nartiss and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#if defined(_OPENMP)
#include <omp.h>
#endif

#include <grass/gis.h>
#include <grass/rowio.h>
#include <grass/raster.h>
#include <grass/manage.h>
#include <grass/glocale.h>
#include "local_proto.h"

static struct Row_cache {
    ROWIO rowio_cache;
    bool use_rowio;
    int tmp_fd;
    char *tmp_name;
    int nrows;
    int ncols;
    size_t len;
    DCELL **matrix;
    void (*fill)(DCELL *, int);
    DCELL *(*get)(int);
    void (*put)(DCELL *, int);
} row_cache;

int rowio_get_row(int fd, void *buffer, int row, int buffer_length);
int rowio_put_row(int fd, const void *buffer, int row, int buffer_length);
DCELL *get_rowio(int row);
void put_rowio(DCELL *buf, int row);
DCELL *get_ram(int row);
void put_ram(DCELL *buf, int row);
void setup_row_cache(int nrows, int ncols, double max_ram);
void teardown_row_cache();
void fill_ram(DCELL *buf, int row);
void fill_rowio(DCELL *buf, int row);

/* For rowio */
int rowio_get_row(int fd, void *buf, int row, int buf_len)
{
    lseek(fd, ((off_t)row) * buf_len, SEEK_SET);
    errno = 0;
    ssize_t reads = read(fd, buf, buf_len);
    if (reads == -1)
        G_fatal_error(
            _("There was an error reading data from a temporary file. %d: %s"),
            errno, strerror(errno));
    return (reads == buf_len);
}

/* For rowio */
int rowio_put_row(int fd, const void *buf, int row, int buf_len)
{
    // can't use G_fseek, as rowio operates on file descriptors
    lseek(fd, ((off_t)row) * buf_len, SEEK_SET);
    errno = 0;
    ssize_t writes = write(fd, buf, buf_len);
    if (writes == -1)
        G_fatal_error(
            _("There was an error writing data from a temporary file. %d: %s"),
            errno, strerror(errno));
    return (writes == buf_len);
}

/* Function to use if cache is disk based */
DCELL *get_rowio(int row)
{
    /* Rowio_get returned buffer is owned by the Rowio and might get
     * a new content on subsequent get call.
     */
    DCELL *buf = Rowio_get(&(row_cache.rowio_cache), row);
    if (buf == NULL)
        G_fatal_error(_("Error fetching data from a disk cache"));
    DCELL *target = G_malloc(row_cache.len);
    memcpy(target, buf, row_cache.len);
    return target;
}

void put_rowio(DCELL *buf, int row)
{
    /* Rowio_put will memcpy buffer to a different one, thus it is safe to reuse
     */
    Rowio_put(&(row_cache.rowio_cache), buf, row);
    G_free(buf);
}

/* Function to use if cache is RAM based */
DCELL *get_ram(int row)
{
    return row_cache.matrix[row];
}

void put_ram(DCELL *buf, int row)
{
    row_cache.matrix[row] = buf;
}

/* For filling with initial data */
void fill_ram(DCELL *buf, int row)
{
    memcpy(row_cache.matrix[row], buf, row_cache.len);
}

void fill_rowio(DCELL *buf, int row)
{
    /* rowio.put calls memcpy thus buf reuse is safe */
    Rowio_put(&(row_cache.rowio_cache), buf, row);
}

/* Set up temporary storage for intermediate step data */
void setup_row_cache(int nrows, int ncols, double max_ram)
{
    /* 1 cell padding on each side */
    row_cache.nrows = nrows + 2;
    row_cache.ncols = ncols + 2;
    row_cache.len = (row_cache.ncols) * sizeof(DCELL);
    /* Try to keep in RAM as much as possible */
    double max_rows = max_ram / (row_cache.len / (1024.0 * 1024));
    /* 22 rows are used for computation */
    row_cache.use_rowio = max_rows - 22 > nrows ? false : true;

    if (row_cache.use_rowio) {
        if (max_rows < 24)
            G_fatal_error(_("Insufficient memory to hold processed data. "
                            "Either increase available memory with the "
                            "\"memory\" parameter "
                            "or reduce size of the computational region."));
        /* Store data for looping over in a temporary file */
        row_cache.tmp_name = G_tempfile();
        errno = 0;
        /* Here could be upgraded to use O_TMPFILE */
        row_cache.tmp_fd = open(row_cache.tmp_name, O_RDWR | O_CREAT | O_EXCL,
                                S_IRUSR | S_IWUSR);
        if (row_cache.tmp_fd == -1)
            G_fatal_error(_("Error creating row cache. %d: %s"), errno,
                          strerror(errno));
        int cache_nrows = (int)max_rows - 22;
        if (Rowio_setup(&(row_cache.rowio_cache), row_cache.tmp_fd, cache_nrows,
                        row_cache.len, &rowio_get_row, &rowio_put_row) != 1)
            G_fatal_error(_("Error creating row cache"));
        row_cache.get = &get_rowio;
        row_cache.put = &put_rowio;
        row_cache.fill = &fill_rowio;
    }
    else {
        /* Everything fits into RAM, no need for disk access */
        row_cache.matrix =
            (DCELL **)G_malloc(row_cache.nrows * sizeof(DCELL *));
#pragma GCC ivdep
        for (int row = 0; row < row_cache.nrows; row++) {
            row_cache.matrix[row] = (DCELL *)G_malloc(row_cache.len);
        }
        row_cache.get = &get_ram;
        row_cache.put = &put_ram;
        row_cache.fill = &fill_ram;
    }
}

void teardown_row_cache()
{
    if (row_cache.use_rowio) {
        Rowio_release(&(row_cache.rowio_cache));
        errno = 0;
        if (close(row_cache.tmp_fd) == -1 || unlink(row_cache.tmp_name) == -1)
            G_warning(_("Error cleaning up row cache. %d: %s"), errno,
                      strerror(errno));
        G_free(row_cache.tmp_name);
    }
    else {
#pragma GCC ivdep
        for (int row = 0; row < row_cache.nrows; row++) {
            G_free(row_cache.matrix[row]);
        }
        G_free(row_cache.matrix);
    }
}

int main(int argc, char *argv[])
{
    int nrows, ncols;
    int out_fd;
    RASTER_MAP_TYPE data_type;

    struct GModule *module;

    struct {
        struct Option *input, *output, *K, *l, *t, *met, *mem;
        struct Flag *old;
    } opt;

    G_gisinit(argv[0]);

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("smoothing"));
    G_add_keyword(_("edge detection"));
    module->description = _("Smoothing with anisotropic diffusion");

    opt.input = G_define_standard_option(G_OPT_R_INPUT);

    opt.output = G_define_standard_option(G_OPT_R_OUTPUT);

    opt.K = G_define_option();
    opt.K->key = "threshold";
    opt.K->type = TYPE_DOUBLE;
    opt.K->required = YES;
    opt.K->description = _("Gradient magnitude threshold (in map units)");
    opt.K->guisection = _("Diffusion");
    opt.K->answer = "5";
    opt.K->options = "0.000000001-";

    opt.l = G_define_option();
    opt.l->key = "lambda";
    opt.l->type = TYPE_DOUBLE;
    opt.l->required = YES;
    opt.l->description = _("Rate of diffusion (0,1]");
    opt.l->guisection = _("Diffusion");
    opt.l->answer = "0.1";
    opt.l->options = "0-1";

    opt.t = G_define_option();
    opt.t->key = "steps";
    opt.t->type = TYPE_INTEGER;
    opt.t->required = YES;
    opt.t->description = _("Number of diffusion steps");
    opt.t->guisection = _("Diffusion");
    opt.t->answer = "10";
    opt.t->options = "1-";

    opt.met = G_define_option();
    opt.met->key = "conditional";
    opt.met->type = TYPE_STRING;
    opt.met->required = YES;
    opt.met->description = _("Conductance function");
    opt.met->options = "exponental,quadratic,tukey";
    opt.met->answer = "tukey";

    opt.mem = G_define_standard_option(G_OPT_MEMORYMB);

    /* Temporary development option */
    opt.old = G_define_flag();
    opt.old->key = 'o';
    opt.old->label = "Use old code";

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    const char *in_map, *out_map, *in_mapset;
    in_map = opt.input->answer;
    out_map = opt.output->answer;
    /* K and lambda could be determined from the data. See formula #21
     * and following discussion in the Black et al. 1998
     */
    int conditional, steps;
    double dt, lambda, threshold;
    threshold = atof(opt.K->answer);
    lambda = atof(opt.l->answer);
    steps = atoi(opt.t->answer);

    double contrast2 = threshold * threshold;
    double scale = threshold * sqrt(2);

    /* Silence compiler warnings */
    conditional = 3;
    dt = 1;

    if (strncmp(opt.met->answer, "tuk", 3) == 0) {
        conditional = 3;
        dt = lambda;
    }
    else if (strncmp(opt.met->answer, "exp", 3) == 0) {
        conditional = 1;
        /* Equations are stable only if 0<= lambda <= 0.25
         * according to Perona P. and Malik J. 1990 */
        dt = lambda > 0.25 ? 0.25 : lambda;
    }
    else if (strncmp(opt.met->answer, "qua", 3) == 0) {
        conditional = 2;
        dt = lambda > 0.25 ? 0.25 : lambda;
    }
    /* Lambda needs to be devided by number of neighbours
     * according to formula 5 in Black et al. 1998 */
    dt = dt / 8.0;

    in_mapset = (char *)G_find_raster2(in_map, "");
    if (in_mapset == NULL)
        G_fatal_error(_("Raster map <%s> not found"), in_map);

    struct Cell_head window;
    Rast_get_window(&window);
    if (window.ew_res < GRASS_EPSILON) {
        G_fatal_error(_("Wrong computational region"));
    }
    nrows = window.rows;
    ncols = window.cols;
    if (nrows < 3 || ncols < 3) {
        G_fatal_error(_("Computational region is too small!"));
    }

    /* Static adjustment for non-square cells.
     * Should be replaced with much more expensice true per-row distance
     * calculation for a ll location. */
    double vert_cor, diag_cor;
    vert_cor = window.ns_res / window.ew_res;
    diag_cor =
        sqrt(window.ns_res * window.ns_res + window.ew_res * window.ew_res) /
        window.ew_res;

    if (opt.old->answer) {
        double **cellst, **cellst1, **tmp_ptr;
        /* determine the inputmap type (CELL/FCELL/DCELL) to match output type
         */
        data_type = Rast_map_type(in_map, in_mapset);
        int in_fd = Rast_open_old(in_map, in_mapset);
        out_fd = Rast_open_new(out_map, data_type);
        void *in_buf, *out_buf;
        int t;
        double n, e, s, w;
        in_buf = Rast_allocate_buf(data_type);
        switch (data_type) {
        case CELL_TYPE:
            out_buf = (CELL *)Rast_allocate_buf(data_type);
            break;
        case FCELL_TYPE:
            out_buf = (FCELL *)Rast_allocate_buf(data_type);
            break;
        case DCELL_TYPE:
            out_buf = (DCELL *)Rast_allocate_buf(data_type);
            break;
        }
        /* Assign memory to hold whole raster */
        cellst = (double **)G_malloc(nrows * sizeof(double *));
        cellst1 = (double **)G_malloc(nrows * sizeof(double *));

        /* Initialization run */
        /* for each row */
        for (int row = 0; row < nrows; row++) {
            CELL c;
            FCELL f;
            DCELL d;

            /* read input map */
            Rast_get_row(in_fd, in_buf, row, data_type);

            cellst[row] = (double *)G_malloc(ncols * sizeof(double));
            cellst1[row] = (double *)G_malloc(ncols * sizeof(double));
            if (cellst[row] == NULL || cellst1[row] == NULL) {
                G_free(cellst[row]);
                G_free(cellst1[row]);
                G_fatal_error("Row %d out of memory", row);
            }

            /* process the data */
            for (int col = 0; col < ncols; col++) {
                /* use different function for each data type */
                switch (data_type) {
                case CELL_TYPE:
                    c = ((CELL *)in_buf)[col];
                    cellst[row][col] = c;
                    break;
                case FCELL_TYPE:
                    f = ((FCELL *)in_buf)[col];
                    cellst[row][col] = f;
                    break;
                case DCELL_TYPE:
                    d = ((DCELL *)in_buf)[col];
                    cellst[row][col] = d;
                    break;
                }
            }
        } /* Finish first initialization run */

        double nw, ne, se, sw;
        /* Loop t times */
        for (t = 0; t < steps; t++) {
            // G_percent(t, steps, 2);
            for (int row = 0; row < nrows; row++) {
                for (int col = 0; col < ncols; col++) {
                    /* We are cloning cell values over computational region
                     * edges */
                    if (row - 1 < 0) {
                        if (col - 1 < 0) {
                            n = cellst[row][col];
                            e = cellst[row][col + 1];
                            s = cellst[row + 1][col];
                            w = cellst[row][col];
                            nw = sw = ne = cellst[row][col];
                            se = cellst[row + 1][col + 1];
                        }
                        else if (col + 1 >= ncols) {
                            n = cellst[row][col];
                            e = cellst[row][col];
                            s = cellst[row + 1][col];
                            w = cellst[row][col - 1];
                            nw = se = ne = cellst[row][col];
                            sw = cellst[row + 1][col - 1];
                        }
                        else {
                            n = cellst[row][col];
                            e = cellst[row][col + 1];
                            s = cellst[row + 1][col];
                            w = cellst[row][col - 1];
                            nw = ne = cellst[row][col];
                            se = cellst[row + 1][col + 1];
                            sw = cellst[row + 1][col - 1];
                        }
                    }
                    else if (row + 1 >= nrows) {
                        if (col - 1 < 0) {
                            n = cellst[row - 1][col];
                            e = cellst[row][col + 1];
                            s = cellst[row][col];
                            w = cellst[row][col];
                            nw = sw = se = cellst[row][col];
                            ne = cellst[row - 1][col + 1];
                        }
                        else if (col + 1 >= ncols) {
                            n = cellst[row - 1][col];
                            e = cellst[row][col];
                            s = cellst[row][col];
                            w = cellst[row][col - 1];
                            nw = se = ne = cellst[row][col];
                            sw = cellst[row - 1][col - 1];
                        }
                        else {
                            n = cellst[row - 1][col];
                            e = cellst[row][col + 1];
                            s = cellst[row][col];
                            w = cellst[row][col - 1];
                            sw = se = cellst[row][col];
                            ne = cellst[row - 1][col + 1];
                            nw = cellst[row - 1][col - 1];
                        }
                    }
                    else {
                        if (col - 1 < 0) {
                            n = cellst[row - 1][col];
                            e = cellst[row][col + 1];
                            s = cellst[row + 1][col];
                            w = cellst[row][col];
                            sw = nw = cellst[row][col];
                            ne = cellst[row - 1][col + 1];
                            se = cellst[row + 1][col + 1];
                        }
                        else if (col + 1 >= ncols) {
                            n = cellst[row - 1][col];
                            e = cellst[row][col];
                            s = cellst[row + 1][col];
                            w = cellst[row][col - 1];
                            se = ne = cellst[row][col];
                            nw = cellst[row - 1][col - 1];
                            sw = cellst[row + 1][col - 1];
                        }
                        else {
                            n = cellst[row - 1][col];
                            e = cellst[row][col + 1];
                            s = cellst[row + 1][col];
                            w = cellst[row][col - 1];
                            nw = cellst[row - 1][col - 1];
                            sw = cellst[row + 1][col - 1];
                            ne = cellst[row - 1][col + 1];
                            se = cellst[row + 1][col + 1];
                        }
                    }
                    cellst1[row][col] =
                        diffuse(conditional, threshold, dt, cellst[row][col], n,
                                e, s, w, nw, ne, se, sw);
                }
            }
            /* Swap arrays for reuse at t+1 */
            tmp_ptr = cellst;
            cellst = cellst1;
            cellst1 = tmp_ptr;
        }

        /* Write out results */
        switch (data_type) {
        case CELL_TYPE:
            for (int row = 0; row < nrows; row++) {
                for (int col = 0; col < ncols; col++) {
                    ((CELL *)out_buf)[col] = cellst[row][col];
                }
                Rast_put_row(out_fd, out_buf, data_type);
            }
            break;
        case FCELL_TYPE:
            for (int row = 0; row < nrows; row++) {
                for (int col = 0; col < ncols; col++) {
                    ((FCELL *)out_buf)[col] = cellst[row][col];
                }
                Rast_put_row(out_fd, out_buf, data_type);
            }
            break;
        case DCELL_TYPE:
            for (int row = 0; row < nrows; row++) {
                for (int col = 0; col < ncols; col++) {
                    ((DCELL *)out_buf)[col] = cellst[row][col];
                }
                Rast_put_row(out_fd, out_buf, data_type);
            }
            break;
        }
        /* memory cleanup */
        G_free(in_buf);
        G_free(out_buf);
        Rast_close(in_fd);
    }
    else {
        setup_row_cache(nrows, ncols, atof(opt.mem->answer));
        /* Copy initial data to the tmp file */
        int in_fd = Rast_open_old(in_map, in_mapset);
        data_type = Rast_map_type(in_map, in_mapset);

        /* Sliding rows (above, current, below + modified) */
        DCELL *out = G_malloc((ncols + 2) * sizeof(DCELL));
        DCELL *ra = G_malloc((ncols + 2) * sizeof(DCELL));
        DCELL *rc;
        DCELL *rb;
        /* We'll use nrows + 2 pad rows at each end + 2 pad cols */
        Rast_get_d_row(in_fd, &out[1], 0);
        out[0] = out[1];
        out[ncols + 1] = out[ncols];
        row_cache.fill(out, 0);
        for (unsigned int row = 0; row < nrows; row++) {
            Rast_get_d_row(in_fd, &out[1], row);
            out[0] = out[1];
            out[ncols + 1] = out[ncols];
            row_cache.fill(out, row + 1);
        }
        row_cache.fill(out, nrows + 1);
        Rast_close(in_fd);

        /* No padding for gradients and divs */
        DCELL **gradients, **divs, *di;
        di = (DCELL *)G_malloc(ncols * sizeof(DCELL));
        gradients = (DCELL **)G_malloc(8 * sizeof(DCELL *));
        divs = (DCELL **)G_malloc(8 * sizeof(DCELL *));
#pragma GCC unroll 8
        for (unsigned int i = 0; i < 8; i++) {
            gradients[i] = (DCELL *)G_malloc(ncols * sizeof(DCELL));
            divs[i] = (DCELL *)G_malloc(ncols * sizeof(DCELL));
        }

        /* A single step */
        for (int step = 0; step < steps; step++) {
            /* Prefill rows */
            rc = row_cache.get(0);
            rb = row_cache.get(1);
            row_cache.put(out, 0);

            /* Loop over padded data */
            for (unsigned int prow = 1; prow < nrows + 1; prow++) {
                /* Slide down by a single row */
                out = ra;
                ra = rc;
                rc = rb;
                rb = row_cache.get(prow + 1);

#pragma omp parallel for
                for (unsigned int pcol = 1; pcol <= ncols; pcol++) {
                    gradients[0][pcol - 1] = (ra[pcol] - rc[pcol]) * vert_cor;
                    gradients[1][pcol - 1] = (rb[pcol] - rc[pcol]) * vert_cor;
                    gradients[2][pcol - 1] =
                        (ra[pcol - 1] - rc[pcol]) * diag_cor;
                    gradients[3][pcol - 1] =
                        (ra[pcol + 1] - rc[pcol]) * diag_cor;
                    gradients[4][pcol - 1] = rc[pcol + 1] - rc[pcol];
                    gradients[5][pcol - 1] = rc[pcol - 1] - rc[pcol];
                    gradients[6][pcol - 1] =
                        (rb[pcol - 1] - rc[pcol]) * diag_cor;
                    gradients[7][pcol - 1] =
                        (rb[pcol + 1] - rc[pcol]) * diag_cor;
                }

                /* Calculate conductance coefficient */
                if (conditional == 3) {
                    /* Black et al. 1998 Tukey's biweight function */
#pragma omp parallel for
                    for (unsigned int col = 0; col < ncols; col++) {
                        if (scale < fabs(gradients[0][col]) ||
                            scale < fabs(gradients[1][col]) ||
                            scale < fabs(gradients[2][col]) ||
                            scale < fabs(gradients[3][col]) ||
                            scale < fabs(gradients[4][col]) ||
                            scale < fabs(gradients[5][col]) ||
                            scale < fabs(gradients[6][col]) ||
                            scale < fabs(gradients[7][col])) {
                            gradients[0][col] = 0;
                            gradients[1][col] = 0;
                            gradients[2][col] = 0;
                            gradients[3][col] = 0;
                            gradients[4][col] = 0;
                            gradients[5][col] = 0;
                            gradients[6][col] = 0;
                            gradients[7][col] = 0;
                        }
#pragma GCC unroll 8
                        for (unsigned int i = 0; i < 8; i++) {
                            divs[i][col] =
                                gradients[i][col] * 0.5 *
                                ((1 - ((gradients[i][col] * gradients[i][col]) /
                                       (scale * scale))) *
                                 (1 - ((gradients[i][col] * gradients[i][col]) /
                                       (scale * scale))));
                        }
                    }
                }
                else if (conditional == 1) {
/* Perona & Malik 1st conductance function = exponential */
#pragma omp parallel for
                    for (unsigned int col = 0; col < ncols; col++) {
#pragma GCC unroll 8
                        for (unsigned int i = 1; i < 8; i++) {
                            divs[i][col] =
                                gradients[i][col] *
                                exp(-1.0 *
                                    ((gradients[i][col] * gradients[i][col]) /
                                     contrast2));
                        }
                    }
                }
                else if (conditional == 2) {
/* Perona & Malik 2nd conductance function = quadratic */
#pragma omp parallel for
                    for (unsigned int col = 0; col < ncols; col++) {
#pragma GCC unroll 8
                        for (unsigned int i = 1; i < 8; i++) {
                            divs[i][col] =
                                gradients[i][col] * 1 /
                                (1 + ((gradients[i][col] * gradients[i][col]) /
                                      contrast2));
                        }
                    }
                }

/* Calculate new values and add padding */
#pragma omp parallel for
                for (unsigned int col = 0; col < ncols; col++) {
                    di[col] = divs[0][col] + divs[1][col] + divs[2][col] +
                              divs[3][col] + divs[4][col] + divs[5][col] +
                              divs[6][col] + divs[7][col];
                    out[col + 1] = di[col] * dt + rc[col + 1];
                }
                out[0] = out[1];
                out[ncols + 1] = out[ncols];

                /* Write out values to tmp file
                 * out can not be reused as it might point to a row in row_cache
                 */
                row_cache.put(out, prow);
            }
            out = ra;
            ra = rc;
            /* Clone new values to padding rows */
            DCELL *tmp;
            tmp = row_cache.get(0);
            memcpy(tmp, row_cache.get(1), row_cache.len);
            row_cache.put(tmp, 0);
            tmp = row_cache.get(nrows + 1);
            memcpy(tmp, row_cache.get(nrows), row_cache.len);
            row_cache.put(tmp, nrows + 1);
        } // step
        /* Let out and ra leak, as they might be freed by caching functions */
        G_free(di);

#pragma GCC ivdep
        for (unsigned int i = 0; i < 8; i++) {
            G_free(gradients[i]);
            G_free(divs[i]);
        }
        G_free(gradients);
        G_free(divs);

        /* Write out final data */
        out_fd = Rast_open_new(out_map, data_type);
        switch (data_type) {
        case DCELL_TYPE:
            for (unsigned int row = 0; row < nrows; row++) {
                double *dbuf = row_cache.get(row + 1);
                Rast_put_d_row(out_fd, &(dbuf[1]));
            }
            break;
        case FCELL_TYPE:
            FCELL *fbuf = (FCELL *)G_malloc(ncols * sizeof(FCELL));
            for (unsigned int row = 0; row < nrows; row++) {
                double const *dbuf = row_cache.get(row + 1);
/* Get rid of padding and cast to output type */
#pragma GCC ivdep
                for (unsigned int col = 0; col < ncols; col++) {
                    fbuf[col] = (FCELL)dbuf[col + 1];
                }
                Rast_put_f_row(out_fd, fbuf);
            }
            G_free(fbuf);
            break;
        case CELL_TYPE:
            CELL *cbuf = G_malloc(ncols * sizeof(CELL));
            for (unsigned int row = 0; row < nrows; row++) {
                double const *dbuf = row_cache.get(row + 1);
/* Get rid of padding and cast to output type */
#pragma GCC ivdep
                for (unsigned int col = 0; col < ncols; col++) {
                    cbuf[col] = (CELL)round(dbuf[col + 1]);
                }
                Rast_put_c_row(out_fd, cbuf);
            }
            G_free(cbuf);
            break;
        }
        teardown_row_cache();
    }

    Rast_close(out_fd);

    struct Colors colors;
    const char *out_mapset = G_mapset();
    if (Rast_read_colors(in_map, in_mapset, &colors) < 0) {
        Rast_init_colors(&colors);
        if (Rast_map_type(in_map, in_mapset) == CELL_TYPE) {
            struct Range range;
            CELL min, max;
            Rast_read_range(out_map, out_mapset, &range);
            Rast_get_range_min_max(&range, &min, &max);
            Rast_make_grey_scale_colors(&colors, min, max);
            Rast_write_colors(out_map, out_mapset, &colors);
        }
        else {
            struct FPRange range;
            DCELL min, max;
            Rast_read_fp_range(in_map, in_mapset, &range);
            Rast_get_fp_range_min_max(&range, &min, &max);
            Rast_make_grey_scale_fp_colors(&colors, min, max);
            Rast_write_colors(out_map, out_mapset, &colors);
        }
        Rast_free_colors(&colors);
    }
    else {
        Rast_write_colors(out_map, out_mapset, &colors);
        Rast_free_colors(&colors);
    }

    struct History history;
    Rast_put_cell_title(out_map, _("Smoothed map"));
    Rast_short_history(out_map, "raster", &history);
    Rast_set_history(&history, HIST_DATSRC_1, in_map);
    Rast_command_history(&history);
    Rast_write_history(out_map, &history);

    exit(EXIT_SUCCESS);
}
