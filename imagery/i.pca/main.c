/****************************************************************************
 *
 * MODULE:       i.pca
 *
 * AUTHOR(S):    Original author Center for Space Research (Uni. of TX)
 *               Rewritten by Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:      Principal Component Analysis transform of satellite data.
 *
 * COPYRIGHT:    (C) 2004-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>
#include "local_proto.h"


#undef PCA_DEBUG


/* function prototypes */
static CELL round_c (double);
static int set_output_scale (struct Option *, int *, int *, int *);
static int calc_mu (int *, double *, int);
static int calc_covariance (int *, double **, double *, int);
static int write_pca (double **, int *, char *, int, int, int, int);
#ifdef PCA_DEBUG
static int dump_eigen (int, double **, double *);
#endif


int main (int argc, char *argv[])
{
    int    i, j;        /* Loop control variables */
    int    bands;       /* Number of image bands */
    double *mu;         /* Mean vector for image bands */
    double **covar;     /* Covariance Matrix */
    double *eigval;
    double **eigmat;
    int    *inp_fd;
    int scale, scale_max, scale_min;

    struct GModule *module;
    struct Option *opt_in, *opt_out, *opt_scale;

    /* initialize GIS engine */
    G_gisinit (argv[0]);

    module = G_define_module();
    module->keywords = _("imagery");
    module->description = _("Principal components analysis (pca) program "
			    "for image processing.");

    /* Define options */
    opt_in  = G_define_standard_option (G_OPT_R_INPUTS);
    opt_out = G_define_standard_option (G_OPT_R_OUTPUT);

    opt_scale = G_define_option() ;
    opt_scale->key        = "rescale";
    opt_scale->type       = TYPE_INTEGER;
    opt_scale->key_desc   = "min,max";
    opt_scale->required   = NO;
    opt_scale->answer     = "0,255"; 
    opt_scale->description= _("Rescaling range output (For no rescaling use 0,0)");

    if (G_parser (argc, argv) < 0)
        exit (EXIT_FAILURE);

    /* determine number of bands passed in */
    for (bands = 0; opt_in->answers[bands] != NULL; bands++);

    if (bands < 2)
        G_fatal_error (_("Sorry, at least 2 input bands must be provided"));

    /* default values */
    scale     = 1;
    scale_min = 0;
    scale_max = 255;

    /* get scale parameters */
    set_output_scale (opt_scale, &scale, &scale_min, &scale_max);

    /* allocate memory */
    covar          = (double **) G_calloc(bands, sizeof(double *));
    mu             = (double *)  G_malloc(bands * sizeof(double));
    inp_fd         = (int *)     G_malloc (bands * sizeof(int));
    eigmat         = (double **) G_calloc(bands, sizeof(double *));
    eigval         = (double *)  G_calloc(bands, sizeof(double));

    /* allocate memory for matrices */
    for (i = 0; i < bands; i++)
    {
        covar[i]  = (double *)G_malloc (bands * sizeof(double));
        eigmat[i] = (double *)G_calloc (bands, sizeof(double)); 

        /* initialize covariance matrix */
        for (j = 0; j < bands; j++)
            covar[i][j] = 0.;
    }

    /* open and check input/output files */
    for (i = 0; i < bands; i++)
    {
        char tmpbuf[128];
        char *mapset;

        sprintf (tmpbuf, "%s.%d", opt_out->answer, i + 1);
        G_check_input_output_name (opt_in->answers[i], tmpbuf, GR_FATAL_EXIT);

        if ((mapset = G_find_cell (opt_in->answers[i], "")) == NULL)
            G_fatal_error (_("Raster map <%s> not found"), opt_in->answers[i]);

        if ((inp_fd[i] = G_open_cell_old (opt_in->answers[i], mapset)) < 0)
            G_fatal_error (_("Unable to open raster map <%s>"), opt_in->answers[i]);
    }

    G_message (_("Calculating covariance matrix:"));
    calc_mu (inp_fd, mu, bands);

    calc_covariance (inp_fd, covar, mu, bands);

    for (i = 0; i < bands; i++)
    {
        for (j = 0; j < bands; j++)
        {
            covar[i][j] = covar[i][j] / ((double) ((G_window_rows () * G_window_cols ()) - 1));
            G_debug (3, "covar[%d][%d] = %f", i, j, covar[i][j]);
        }
    }

    G_debug (1, _("Calculating eigenvalues and eigenvectors..."));
    eigen (covar, eigmat, eigval, bands);

#ifdef PCA_DEBUG
    /* dump eigen matrix and eigen values */
    dump_eigen (bands, eigmat, eigval);
#endif

    G_debug (1, _("Ordering eigenvalues in descending order..."));
    egvorder2 (eigval, eigmat, bands);

    G_debug (1, _("Transposing eigen matrix..."));
    transpose2 (eigmat, bands);

    /* write output images */
    write_pca (eigmat, inp_fd, opt_out->answer, bands, scale, scale_min, scale_max); 

    /* write colors and history to output */
    for (i = 0; i < bands; i++)
    {
        char outname[80];

        sprintf (outname, "%s.%d", opt_out->answer, i + 1);

        /* write colors and history to file */
        write_support (bands, outname, eigmat);

        /* close output file */
        G_unopen_cell (inp_fd[i]);
    }

    exit (EXIT_SUCCESS);
}


static CELL round_c (double x)
{
    CELL n;

    if (x >= 0.0)
        n = x + .5;
    else {
        n = -x + .5;
        n = -n;
    }

    return n;
}


static int
set_output_scale (struct Option *scale_opt, int *scale, int *scale_min, int *scale_max)
{
    if (scale_opt->answer)
    {
        sscanf (scale_opt->answers[0], "%d", (int *)scale_min);
        sscanf (scale_opt->answers[1], "%d", (int *)scale_max);

        if (*scale_min == *scale_max)
        {
            if (*scale_min == 0)
                *scale = 0;
            else
            {
                G_warning (_("Scale range length should be > 0. Using default values: 0,255"));
                *scale_min = 0;
                *scale_max = 255;
            }
        }

        if (*scale_max < *scale_min)
        {
            int tmp_scale = *scale_max;

            *scale_max = *scale_min;
            *scale_min = tmp_scale;
        }
    }

    return 0;
}


static int
calc_mu (int *fds, double *mu, int bands)
{
    int i;
    int rows = G_window_rows ();
    int cols = G_window_cols ();
    void *rowbuf = NULL;

    for (i = 0; i < bands; i++) 
    {
        RASTER_MAP_TYPE maptype;
        int row, col;
        double sum = 0.;

        maptype = G_get_raster_map_type (fds[i]);

        /* don't assume each image is of the same type */
        if (rowbuf) G_free (rowbuf);
        if ((rowbuf = G_allocate_raster_buf (maptype)) == NULL)
            G_fatal_error (_("Cannot allocate memory for row buffer"));

        G_message (_("Computing Means for band %d:"), i+1);
        for (row = 0; row < rows; row++)
        {
            void *ptr = rowbuf;

            G_percent (row, rows - 1, 2);

            if (G_get_raster_row (fds[i], rowbuf, row, maptype) < 0)
                G_fatal_error (_("Cannot read raster row [%d]"), row);

            for (col = 0; col < cols; col++)
            {
                /* skip null cells */
                if (G_is_null_value (rowbuf, maptype)) {
                    ptr = G_incr_void_ptr (ptr, G_raster_size (maptype));
                    continue;
                }

                sum += G_get_raster_value_d (rowbuf, maptype);
                ptr = G_incr_void_ptr (ptr, G_raster_size (maptype));
            }
        }

        mu[i] = sum / (double) (rows * cols);
    }

    if (rowbuf) G_free (rowbuf);

    return 0;
}


static int
calc_covariance (int *fds, double **covar, double *mu, int bands)
{
    int j, k;
    int rows = G_window_rows ();
    int cols = G_window_cols ();
    int row, col;

    for (j = 0; j < bands; j++)
    {
        RASTER_MAP_TYPE maptype = G_get_raster_map_type (fds[j]);
        void *rowbuf1 = NULL;
        void *rowbuf2 = NULL;

        /* don't assume each image is of the same type */
        if (rowbuf1) G_free (rowbuf1);
        if ((rowbuf1 = G_allocate_raster_buf (maptype)) == NULL)
            G_fatal_error (_("Cannot allocate memory for row buffer"));

        G_message (_("Computing row %d of covariance matrix:"), j+1);
        for (row = 0; row < rows; row++)
        {
            void *ptr1, *ptr2;

            G_percent (row, rows - 1, 2);

            if (G_get_raster_row (fds[j], rowbuf1, row, maptype) < 0)
                G_fatal_error (_("Cannot read raster row [%d]"), row);

            for (k = j; k < bands; k++)
            {
                RASTER_MAP_TYPE maptype2 = G_get_raster_map_type (fds[k]);

                /* don't assume each image is of the same type */
                if (rowbuf2) G_free (rowbuf2);
                if ((rowbuf2 = G_allocate_raster_buf (maptype2)) == NULL)
                    G_fatal_error (_("Cannot allocate memory for row buffer"));

                if (G_get_raster_row (fds[k], rowbuf2, row, maptype2) < 0)
                    G_fatal_error (_("Cannot read raster row [%d]"), row);

                ptr1 = rowbuf1;
                ptr2 = rowbuf2;

                for (col = 0; col < cols; col++)
                {
                    /* skip null cells */
                    if (G_is_null_value (ptr1, maptype) || G_is_null_value (ptr2, maptype2)) {
                        ptr1 = G_incr_void_ptr (ptr1, G_raster_size (maptype));
                        ptr2 = G_incr_void_ptr (ptr2, G_raster_size (maptype2));
                        continue;
                    }

                    covar[j][k] += ((double)G_get_raster_value_d (ptr1, maptype) - mu[j]) *
                                   ((double)G_get_raster_value_d (ptr2, maptype2) - mu[k]);

                    ptr1 = G_incr_void_ptr (ptr1, G_raster_size (maptype));
                    ptr2 = G_incr_void_ptr (ptr2, G_raster_size (maptype2));
                }

                covar[k][j] = covar[j][k];
            }
        }
    }

    return 0;
}


static int
write_pca (double **eigmat, int *inp_fd, char *out_basename,
            int bands, int scale, int scale_min, int scale_max)
{
    int i, j;
    void *outbuf, *outptr;
    double min       = 0.;
    double max       = 0.;
    double old_range = 0.;
    double new_range = 0.;
    int rows         = G_window_rows ();
    int cols         = G_window_cols ();
    int cell_mapsiz  = G_raster_size (CELL_TYPE);
    int dcell_mapsiz = G_raster_size (DCELL_TYPE);
    DCELL *d_buf;

    /* 2 passes for rescale.  1 pass for no rescale */
    int PASSES = (scale) ? 2 : 1;

    /* temporary row storage */
    d_buf  = (DCELL *)G_malloc (cols * sizeof(double));

    /* allocate memory for output row buffer */
    outbuf = (scale) ? G_allocate_raster_buf (CELL_TYPE) :
                       G_allocate_raster_buf (DCELL_TYPE);

    if (!outbuf)
        G_fatal_error (_("Cannot allocate memory for raster row"));

    for (i = 0; i < bands; i++) 
    {
        char name[100];
        int out_fd;
        int pass;

        sprintf (name, "%s.%d", out_basename, i + 1);
 
        G_message ("%s: Transforming:", name);

        /* open a new file for output */
        if (scale)
            out_fd = G_open_cell_new (name);
        else {
            out_fd = G_open_fp_cell_new (name);
            G_set_fp_type (DCELL_TYPE);
        }

        if (out_fd < 0)
            G_fatal_error (_("Unable to create raster map <%s>"),
			   G_fully_qualified_name (name, G_mapset ()));

        for (pass = 1; pass <= PASSES; pass++)
        {
            void *rowbuf = NULL;
            int row, col;

            if (scale && (pass == PASSES))
            {
                G_message (_("%s: Rescaling the data to [%d,%d] range:"), 
                            name, scale_min, scale_max);

                old_range = max - min;
                new_range = (double) (scale_max - scale_min);
            }

            for (row = 0; row < rows; row++)
            {
                void *rowptr;

                G_percent (row, rows, 2);

                /* reset d_buf */
                for (col = 0; col < cols; col++)
                    d_buf[col] = 0.;

	        for (j = 0; j < bands; j++)
	        {
                    RASTER_MAP_TYPE maptype = G_get_raster_map_type (inp_fd[j]);

                    /* don't assume each image is of the same type */
                    if (rowbuf) G_free (rowbuf);
                    if (!(rowbuf = G_allocate_raster_buf (maptype)))
                        G_fatal_error (_("Cannot allocate memory for row buffer"));

                    if (G_get_raster_row (inp_fd[j], rowbuf, row, maptype) < 0)
                        G_fatal_error (_("Cannot read raster row [%d]"), row);

                    rowptr = rowbuf;
                    outptr = outbuf;

                    /* add into the output cell eigmat[i][j] * corresp cell 
                     * of j-th band for current j */
                    for (col = 0; col < cols; col++)
                    {
                        /* handle null cells */
                        if (G_is_null_value (rowptr, maptype))
                        {
                            if (scale) {
                                G_set_null_value (outptr, 1, CELL_TYPE);
                                outptr = G_incr_void_ptr (outptr, cell_mapsiz);
                            } else {
                                G_set_null_value (outptr, 1, DCELL_TYPE);
                                outptr = G_incr_void_ptr (outptr, dcell_mapsiz);
                            }

                            rowptr = G_incr_void_ptr (rowptr, G_raster_size (maptype));
                            continue;
                        }

                        /* corresp. cell of j-th band */
                        d_buf[col] += eigmat[i][j] * G_get_raster_value_d (rowptr, maptype);

                        /* the cell entry is complete */
                        if (j == (bands - 1))
                        {
                            if (scale && (pass == 1))
                            {
                                if ((row == 0) && (col == 0))
                                    min = max = d_buf[0];

                                if (d_buf[col] < min)
                                    min = d_buf[col];

                                if (d_buf[col] > max)
                                    max = d_buf[col];
                            } else if (scale) {

                                if (min == max) {
                                    G_set_raster_value_c (outptr, 1, CELL_TYPE);
                                } else {
                                    /* map data to 0, (new_range-1) and then adding new_min */
                                    CELL tmpcell = round_c ((new_range * (d_buf[col] - min) / old_range) + scale_min);

                                    G_set_raster_value_c (outptr, tmpcell, CELL_TYPE);
                                }
                            } else /* (!scale) */ {
                                G_set_raster_value_d (outptr, d_buf[col], DCELL_TYPE);
                            }
                        }

                        outptr = (scale) ?
                                G_incr_void_ptr (outptr, cell_mapsiz) :
                                G_incr_void_ptr (outptr, dcell_mapsiz);

                        rowptr = G_incr_void_ptr (rowptr, G_raster_size (maptype));
                    }
                } /* for j = 0 to bands */

                if (pass == PASSES)
                {
	            if (scale)
                        G_put_raster_row (out_fd, outbuf, CELL_TYPE);
                    else
                        G_put_raster_row (out_fd, outbuf, DCELL_TYPE);
                }
            }

            G_percent (row, rows, 2);

            /* close output file */
            if (pass == PASSES)
                G_close_cell (out_fd);
        }
    }

    if (d_buf)  G_free (d_buf);
    if (outbuf) G_free (outbuf);

    return 0;
}


#ifdef PCA_DEBUG
static int
dump_eigen (int bands, double **eigmat, double *eigval)
{
    int i, j;

    for (i = 0; i < bands; i++)
    {
        for (j = 0; j < bands; j++)
            fprintf (stderr, "%f  ", eigmat[i][j]);

        fprintf (stderr, "\n");
    }

    for (i = 0; i < bands; i++)
        fprintf (stderr, "%f  ", eigval[i]);

    fprintf (stderr, "\n");

    return 0;
}
#endif
