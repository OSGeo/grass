/* input.c (simlib), 20.nov.2002, JH */

#include "simlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/linkm.h>
#include <grass/gmath.h>
#include <grass/simlib.h>
#include <grass/waterglobs.h>

/* Local prototypes for raster map reading and array allocation */
static float **read_float_raster_map(int rows, int cols, char *name,
                                     float unitconv);
static double **read_double_raster_map(int rows, int cols, char *name,
                                       double unitconv);
static float **create_float_matrix(int rows, int cols, float fill_value);
static double **create_double_matrix(int rows, int cols, double fill_value);
static void copy_matrix_undef_double_to_float_values(int rows, int cols,
                                                     double **source,
                                                     float **target);
static void copy_matrix_undef_float_values(int rows, int cols, float **source,
                                           float **target);

/* we do the allocation inside because we anyway need to set the variables */

void alloc_grids_water(const Geometry *geometry, const Outputs *outputs)
{
    /* memory allocation for output grids */
    G_debug(1, "beginning memory allocation for output grids");

    gama = G_alloc_matrix(geometry->my, geometry->mx);
    if (outputs->err != NULL)
        gammas = G_alloc_matrix(geometry->my, geometry->mx);
    dif = G_alloc_fmatrix(geometry->my, geometry->mx);
}

void alloc_grids_sediment(const Geometry *geometry, const Outputs *outputs)
{
    /* mandatory for si,sigma */

    si = G_alloc_matrix(geometry->my, geometry->mx);
    sigma = G_alloc_matrix(geometry->my, geometry->mx);

    /* memory allocation for output grids */

    dif = G_alloc_fmatrix(geometry->my, geometry->mx);
    if (outputs->erdep != NULL || outputs->et != NULL)
        er = G_alloc_fmatrix(geometry->my, geometry->mx);
}

void init_grids_sediment(const Setup *setup, const Geometry *geometry,
                         const Outputs *outputs)
{
    /* this should be fulfilled for sediment but not water */
    if (outputs->et != NULL)
        erod(si, setup, geometry);
}

void alloc_walkers(int max_walkers, Simulation *sim, const Outputs *outputs)
{
    G_debug(1, "beginning memory allocation for walkers");

    w = (struct point3D *)G_calloc(max_walkers, sizeof(struct point3D));
    vavg = (struct point2D *)G_calloc(max_walkers, sizeof(struct point2D));
    if (outputs->outwalk != NULL)
        sim->stack =
            (struct point3D *)G_calloc(max_walkers, sizeof(struct point3D));
}

/* ************************************************************** */
/*                         GRASS input procedures, allocations    */
/* *************************************************************** */

/*!
 * \brief allocate memory, read input rasters, assign UNDEF to NODATA
 *
 *  \return int
 */

/* ************************************************************************* */
/* Read all input maps and input values into memory ************************ */
int input_data(int rows, int cols, Simulation *sim, const Inputs *inputs,
               const Outputs *outputs)
{
    int max_walkers;
    double unitconv = 0.000000278; /* mm/hr to m/s */

    G_debug(1, "Running MAR 2011 version, started modifications on 20080211");
    G_debug(1, "Reading input data");

    /* Elevation and gradients are mandatory */
    zz = read_float_raster_map(rows, cols, inputs->elevin, 1.0);
    v1 = read_double_raster_map(rows, cols, inputs->dxin, 1.0);
    v2 = read_double_raster_map(rows, cols, inputs->dyin, 1.0);

    /* Update elevation map */
    copy_matrix_undef_double_to_float_values(rows, cols, v1, zz);
    copy_matrix_undef_double_to_float_values(rows, cols, v2, zz);

    /* Manning surface roughnes: read map or use a single value */
    if (inputs->manin != NULL) {
        cchez = read_float_raster_map(rows, cols, inputs->manin, 1.0);
    }
    else if (inputs->manin_val >=
             0.0) { /* If no value set its set to -999.99 */
        cchez = create_float_matrix(rows, cols, inputs->manin_val);
    }
    else {
        G_fatal_error(_("Manning's n raster map not found and manin_val "
                        "undefined, choose one to be allowed to process"));
    }

    /* Rain: read rain map or use a single value for all cells */
    if (inputs->rain != NULL) {
        si = read_double_raster_map(rows, cols, inputs->rain, unitconv);
    }
    else if (inputs->rain_val >= 0.0) { /* If no value set its set to -999.99 */
        si = create_double_matrix(rows, cols, inputs->rain_val * unitconv);
    }
    else {
        si = create_double_matrix(rows, cols, (double)UNDEF);
    }

    /* Update elevation map */
    copy_matrix_undef_double_to_float_values(rows, cols, si, zz);

    /* Infiltration: read map or use a single value */
    if (inputs->infil != NULL) {
        inf = read_double_raster_map(rows, cols, inputs->infil, unitconv);
    }
    else if (inputs->infil_val >=
             0.0) { /* If no value set its set to -999.99 */
        inf = create_double_matrix(rows, cols, inputs->infil_val * unitconv);
    }
    else {
        inf = create_double_matrix(rows, cols, (double)UNDEF);
    }

    /* Traps */
    if (inputs->traps != NULL)
        trap = read_float_raster_map(rows, cols, inputs->traps, 1.0);
    else
        trap = create_float_matrix(rows, cols, (double)UNDEF);

    if (inputs->detin != NULL) {
        dc = read_float_raster_map(rows, cols, inputs->detin, 1.0);
        copy_matrix_undef_float_values(rows, cols, dc, zz);
    }

    if (inputs->tranin != NULL) {
        ct = read_float_raster_map(rows, cols, inputs->tranin, 1.0);
        copy_matrix_undef_float_values(rows, cols, ct, zz);
    }

    if (inputs->tauin != NULL) {
        tau = read_float_raster_map(rows, cols, inputs->tauin, 1.0);
        copy_matrix_undef_float_values(rows, cols, tau, zz);
    }

    if (inputs->wdepth != NULL) {
        gama = read_double_raster_map(rows, cols, inputs->wdepth, 1.0);
        copy_matrix_undef_double_to_float_values(rows, cols, gama, zz);
    }
    /* allocate walkers */
    max_walkers = sim->maxwa + cols * rows;
    alloc_walkers(max_walkers, sim, outputs);

    /* Array for gradient checking */
    slope = create_double_matrix(rows, cols, 0.0);

    return 1;
}

/* ************************************************************************* */

/* data preparations, sigma, shear, etc. */
int grad_check(Setup *setup, const Geometry *geometry, const Settings *settings,
               const Inputs *inputs, const Outputs *outputs)
{
    int k, l;
    double zx, zy, zd2, zd4, sinsl;
    double cc, cmul2;
    double sheer;
    double vsum = 0.;
    double vmax = 0.;
    double chsum = 0.;
    double zmin = 1.e12;
    double zmax = -1.e12;
    double zd2min = 1.e12;
    double zd2max = -1.e12;
    double smin = 1.e12;
    double smax = -1.e12;
    double infmin = 1.e12;
    double infmax = -1.e12;
    double sigmax = -1.e12;
    double cchezmax = -1.e12;
    double rhow = 1000.;
    double gacc = 9.81;
    double hh = 1.;
    double deltaw = 1.e12;

    setup->sisum = 0.;
    double infsum = 0.;
    cmul2 = rhow * gacc;

    for (k = 0; k < geometry->my; k++) {
        for (l = 0; l < geometry->mx; l++) {
            if (zz[k][l] != UNDEF) {
                zx = v1[k][l];
                zy = v2[k][l];
                zd2 = zx * zx + zy * zy;
                sinsl = sqrt(zd2) / sqrt(zd2 + 1); /* sin(terrain slope) */
                /* Computing MIN */
                zd2 = sqrt(zd2);
                zd2min = amin1(zd2min, zd2);
                /* Computing MAX */
                zd2max = amax1(zd2max, zd2);
                zd4 = sqrt(zd2); /* ^.25 */
                if (cchez[k][l] != 0.) {
                    cchez[k][l] = 1. / cchez[k][l]; /* 1/n */
                }
                else {
                    G_fatal_error(_("Zero value in Mannings n"));
                }
                if (zd2 == 0.) {
                    v1[k][l] = 0.;
                    v2[k][l] = 0.;
                    slope[k][l] = 0.;
                }
                else {
                    if (inputs->wdepth)
                        hh = pow(gama[k][l], 2. / 3.);
                    /* hh = 1 if there is no water depth input */
                    v1[k][l] = (double)hh * cchez[k][l] * zx / zd4;
                    v2[k][l] = (double)hh * cchez[k][l] * zy / zd4;

                    slope[k][l] =
                        sqrt(v1[k][l] * v1[k][l] + v2[k][l] * v2[k][l]);
                }
                if (inputs->wdepth) {
                    sheer =
                        (double)(cmul2 * gama[k][l] * sinsl); /* shear stress */
                    /* if critical shear stress >= shear then all zero */
                    if ((sheer <= tau[k][l]) || (ct[k][l] == 0.)) {
                        si[k][l] = 0.;
                        sigma[k][l] = 0.;
                    }
                    else {
                        si[k][l] = (double)(dc[k][l] * (sheer - tau[k][l]));
                        sigma[k][l] =
                            (double)(dc[k][l] / ct[k][l]) *
                            (sheer - tau[k][l]) /
                            (pow(sheer,
                                 1.5)); /* rill erosion=1.5, sheet = 1.1 */
                    }
                }
                setup->sisum += si[k][l];
                smin = amin1(smin, si[k][l]);
                smax = amax1(smax, si[k][l]);
                if (inf) {
                    infsum += inf[k][l];
                    infmin = amin1(infmin, inf[k][l]);
                    infmax = amax1(infmax, inf[k][l]);
                }
                vmax = amax1(vmax, slope[k][l]);
                vsum += slope[k][l];
                chsum += cchez[k][l];
                zmin = amin1(zmin, (double)zz[k][l]);
                zmax =
                    amax1(zmax, (double)zz[k][l]); /* not clear were needed */
                if (inputs->wdepth)
                    sigmax = amax1(sigmax, sigma[k][l]);
                cchezmax = amax1(cchezmax, cchez[k][l]);
                /* saved sqrt(sinsl)*cchez to cchez array for output */
                cchez[k][l] *= sqrt(sinsl);
            } /* DEFined area */
        }
    }
    if (inf != NULL && smax < infmax)
        G_warning(_("Infiltration exceeds the rainfall rate everywhere! No "
                    "overland flow."));

    cc = (double)geometry->mx * geometry->my;

    setup->si0 = setup->sisum / cc;
    setup->vmean = vsum / cc;
    double chmean = chsum / cc;

    if (inf)
        setup->infmean = infsum / cc;

    if (inputs->wdepth)
        deltaw = 0.8 / (sigmax * vmax); /*time step for sediment */
    setup->deltap =
        0.25 * sqrt(geometry->stepx * geometry->stepy) /
        (setup->vmean > EPS ? setup->vmean : EPS); /*time step for water */

    if (setup->deltap < settings->mintimestep)
        setup->deltap = settings->mintimestep;

    if (deltaw > setup->deltap)
        setup->timec = 4.;
    else
        setup->timec = 1.25;

    setup->miter =
        (int)(settings->timesec /
              (setup->deltap * setup->timec)); /* number of iterations = number
                                                  of cells to pass */
    setup->iterout =
        (int)(settings->iterout /
              (setup->deltap * setup->timec)); /* number of cells to pass for
                                                  time series output */

    fprintf(stderr, "\n");
    G_message(_("Min elevation \t= %.2f m\nMax elevation \t= %.2f m\n"), zmin,
              zmax);
    G_message(_("Mean Source Rate (rainf. excess or sediment) \t= %f m/s or "
                "kg/m2s \n"),
              setup->si0);
    G_message(_("Mean flow velocity \t= %f m/s\n"), setup->vmean);
    G_message(_("Mean Mannings \t= %f\n"), 1.0 / chmean);

    setup->deltap = amin1(setup->deltap, deltaw);

    G_message(n_("Number of iterations \t= %d cell\n",
                 "Number of iterations \t= %d cells\n", setup->miter),
              setup->miter);
    G_message(_("Time step \t= %.2f s\n"), setup->deltap);
    if (inputs->wdepth) {
        G_message(_("Sigmax \t= %f\nMax velocity \t= %f m/s\n"), sigmax, vmax);
        G_message(_("Time step used \t= %.2f s\n"), deltaw);
    }
    /*    if (wdepth) deltap = 0.1;
     *    deltap for sediment is ar. average deltap and deltaw */
    /*    if (wdepth) deltap = (deltaw+deltap)/2.;
     *    deltap for sediment is ar. average deltap and deltaw */

    /*! For each cell (k,l) compute the length s=(v1,v2) of the path
     *  that the particle will travel per one time step
     *  \f$ s(k,l)=v(k,l)*dt \f$, [m]=[m/s]*[s]
     *  give warning if there is a cell that will lead to path longer than 2
     * cells
     *
     *  if running erosion, compute sediment transport capacity for each cell
     * si(k,l) \f$ T({\bf r})=K_t({\bf r}) \bigl[\tau({\bf r})\bigr]^p =K_t({\bf
     * r}) \bigl[\rho_w\, g h({\bf r}) \sin \beta ({\bf r}) \bigr]^p \f$
     * [kg/ms]=...
     */
    for (k = 0; k < geometry->my; k++) {
        for (l = 0; l < geometry->mx; l++) {
            if (zz[k][l] != UNDEF) {
                v1[k][l] *= setup->deltap;
                v2[k][l] *= setup->deltap;
                /*if(v1[k][l]*v1[k][l]+v2[k][l]*v2[k][l] > cellsize, warning,
                 *napocitaj ak viac ako 10%a*/
                /* THIS IS CORRECT SOLUTION currently commented out */
                if (inf)
                    inf[k][l] *= settings->timesec;
                if (inputs->wdepth)
                    gama[k][l] = 0.;
                if (outputs->et) {
                    if (sigma[k][l] == 0. || slope[k][l] == 0.)
                        si[k][l] = 0.;
                    else
                        /* temp for transp. cap. erod */
                        si[k][l] = si[k][l] / (slope[k][l] * sigma[k][l]);
                }
            } /* DEFined area */
        }
    }

    /*! compute transport capacity limited erosion/deposition et
     *   as a divergence of sediment transport capacity
     *   \f$
     D_T({\bf r})= \nabla\cdot {\bf T}({\bf r})
     *   \f$
     */
    if (outputs->et) {
        erod(si, setup, geometry); /* compute divergence of t.capc */
        if (output_et(geometry, outputs) != 1)
            G_fatal_error(_("Unable to write et file"));
    }

    /*! compute the inversion operator and store it in sigma - note that after
     * this sigma does not store the first order reaction coefficient but the
     * operator WRITE the equation here
     */
    if (inputs->wdepth) {
        for (k = 0; k < geometry->my; k++) {
            for (l = 0; l < geometry->mx; l++) {
                if (zz[k][l] != UNDEF) {
                    /* get back from temp */
                    if (outputs->et)
                        si[k][l] = si[k][l] * slope[k][l] * sigma[k][l];
                    if (sigma[k][l] != 0.)
                        /* rate of weight loss - w=w*sigma ,
                         * vaha prechadzky po n-krokoch je sigma^n */

                        /*!!!!! not clear what's here :-\ !!!!! */

                        sigma[k][l] =
                            exp(-sigma[k][l] * setup->deltap * slope[k][l]);
                    /* if(sigma[k][l]<0.5) warning, napocitaj,
                     * ak vacsie ako 50% skonci, zmensi deltap)*/
                }
            } /*DEFined area */
        }
    }
    return 1;
}

/* ************************************************************************* */

void copy_matrix_undef_double_to_float_values(int rows, int cols,
                                              double **source, float **target)
{
    int col = 0, row = 0;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            if (source[row][col] == UNDEF)
                target[row][col] = UNDEF;
        }
    }
}

/* ************************************************************************* */

void copy_matrix_undef_float_values(int rows, int cols, float **source,
                                    float **target)
{
    int col = 0, row = 0;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            if (source[row][col] == UNDEF)
                target[row][col] = UNDEF;
        }
    }
}

/* ************************************************************************* */

float **create_float_matrix(int rows, int cols, float fill_value)
{
    int col = 0, row = 0;
    float **matrix = NULL;

    G_verbose_message("Creating float matrix with value %g", fill_value);

    /* Allocate the float marix */
    matrix = G_alloc_fmatrix(rows, cols);

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            matrix[row][col] = fill_value;
        }
    }

    return matrix;
}

/* ************************************************************************* */

double **create_double_matrix(int rows, int cols, double fill_value)
{
    int col = 0, row = 0;
    double **matrix = NULL;

    G_verbose_message("Creating double matrix with value %g", fill_value);

    /* Allocate the float marix */
    matrix = G_alloc_matrix(rows, cols);

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            matrix[row][col] = fill_value;
        }
    }

    return matrix;
}

/* ************************************************************************* */

float **read_float_raster_map(int rows, int cols, char *name, float unitconv)
{
    FCELL *row_buff = NULL;
    int fd;
    int col = 0, row = 0, row_rev = 0;
    float **matrix = NULL;

    G_verbose_message("Reading float map %s into memory", name);

    /* Open raster map */
    fd = Rast_open_old(name, "");

    /* Allocate the row buffer */
    row_buff = Rast_allocate_f_buf();

    /* Allocate the float marix */
    matrix = G_alloc_fmatrix(rows, cols);

    for (row = 0; row < rows; row++) {
        Rast_get_f_row(fd, row_buff, row);

        for (col = 0; col < cols; col++) {
            /* we fill the arrays from south to north */
            row_rev = rows - row - 1;
            /* Check for null values */
            if (!Rast_is_f_null_value(row_buff + col))
                matrix[row_rev][col] = (float)(unitconv * row_buff[col]);
            else
                matrix[row_rev][col] = UNDEF;
        }
    }

    /* Free the row buffer */
    if (row_buff)
        G_free(row_buff);

    Rast_close(fd);

    return matrix;
}

/* ************************************************************************* */

double **read_double_raster_map(int rows, int cols, char *name, double unitconv)
{
    DCELL *row_buff = NULL;
    int fd;
    int col = 0, row = 0, row_rev;
    double **matrix = NULL;

    G_verbose_message("Reading double map %s into memory", name);

    /* Open raster map */
    fd = Rast_open_old(name, "");

    /* Allocate the row buffer */
    row_buff = Rast_allocate_d_buf();

    /* Allocate the double marix */
    matrix = G_alloc_matrix(rows, cols);

    for (row = 0; row < rows; row++) {
        Rast_get_d_row(fd, row_buff, row);

        for (col = 0; col < cols; col++) {
            /* we fill the arrays from south to north */
            row_rev = rows - row - 1;
            /* Check for null values */
            if (!Rast_is_d_null_value(row_buff + col))
                matrix[row_rev][col] = (double)(unitconv * row_buff[col]);
            else
                matrix[row_rev][col] = UNDEF;
        }
    }

    /* Free the row buffer */
    if (row_buff)
        G_free(row_buff);

    Rast_close(fd);

    return matrix;
}
