/*****************************************************************************
 *
 * MODULE:       v.vol.rst: program for 3D (volume) interpolation and geometry
 *               analysis from scattered point data using regularized spline
 *               with tension
 *
 * AUTHOR(S):    Original program (1989) and various modifications:
 *               Lubos Mitas
 *
 *               GRASS 4.2, GRASS 5.0 version and modifications:
 *               H. Mitasova,  I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 * PURPOSE:      v.vol.rst interpolates the values to 3-dimensional grid from
 *               point data (climatic stations, drill holes etc.) given in a
 *               3D vector point input. Output grid3 file is elev.
 *               Regularized spline with tension is used for the
 *               interpolation.
 *
 * COPYRIGHT:    (C) 1989, 1993, 2000 L. Mitas,  H. Mitasova,
 *               I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/bitmap.h>
#include <grass/glocale.h>
#include "oct.h"
#include "surf.h"
#include "dataoct.h"
#include "userextern.h"
#include "userglobs.h"
#include "user.h"

/*
   x,y,z - input data
   npoint - number of input data
   az- interpolated values z for output grid
   adx,ady, ... - estimation of derivatives for output grid
   xmin ... - coordinates of corners of output grid

   subroutines
   INPUT - input of data x,y,z (test function or measured data)
   OUTGR - output of gridded data and derivatives/sec.parameters
 */

/*
   INPUT now reads site files using the new, multi-attribute format
   (mca 2/12/96)
 */

int INPUT(struct Map_info *In, char *column, char *scol, char *wheresql)
{
    struct quadruple *point;
    double x, y, z, w, nz = 0., sm;
    double c1, c2, c3, c4, c5, c6, nsg;
    int i, j, k = 0, a, irev, cfmask;
    int ddisk = 0;
    double deltx, delty, deltz;
    int first_time = 1;
    CELL *cellmask;
    const char *mapsetm;
    int cat, intval;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr, sarray;
    int nrec, /* nrec1, */ ctype, sctype;
    struct line_pnts *Points;
    struct line_cats *Cats;

    OUTRANGE = 0;
    NPOINT = 0;
    dmin = dmin * dmin;

    /* Read attributes */
    db_CatValArray_init(&cvarr);
    if (scol != NULL)
        db_CatValArray_init(&sarray);
    Fi = Vect_get_field(In, 1);
    if (Fi == NULL)
        G_fatal_error(_("Unable to get layer info for vector map"));

    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (Driver == NULL)
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                      Fi->database, Fi->driver);

    nrec = db_select_CatValArray(Driver, Fi->table, Fi->key, column, wheresql,
                                 &cvarr);
    ctype = cvarr.ctype;
    G_debug(3, "nrec = %d", nrec);

    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
        G_fatal_error(_("Column type of wcolumn is not supported (must be "
                        "integer or double)"));

    if (nrec < 0)
        G_fatal_error(_("Unable to select data from table"));
    G_message("%d records selected from table", nrec);

    if (scol != NULL) {

        /* nrec1 =
           db_select_CatValArray(Driver, Fi->table, Fi->key, scol, wheresql,
           &sarray); */
        sctype = cvarr.ctype;

        if (sctype == -1)
            G_fatal_error(_("Cannot read column type of smooth column"));
        if (sctype == DB_C_TYPE_DATETIME)
            G_fatal_error(
                _("Column type of smooth column (datetime) is not supported"));
        if (sctype != DB_C_TYPE_INT && sctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(_("Column type of smooth column is not supported "
                            "(must be integer or double)"));
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_rewind(In);

    while (1) {
        int ival, type, ret;

        if (-1 == (type = Vect_read_next_line(In, Points, Cats)))
            G_fatal_error(_("Unable to read vector map"));

        if (type == -2)
            break; /* EOF */

        if (!(type & GV_POINTS))
            continue;

        Vect_cat_get(Cats, 1, &cat);
        if (cat < 0) {
            G_warning(_("Point without category"));
            continue;
        }

        x = Points->x[0];
        y = Points->y[0];
        z = Points->z[0];

        if (ctype == DB_C_TYPE_INT) {
            ret = db_CatValArray_get_value_int(&cvarr, cat, &ival);
            w = ival;
        }
        else { /* DB_C_TYPE_DOUBLE */
            ret = db_CatValArray_get_value_double(&cvarr, cat, &w);
        }

        if (ret != DB_OK) {
            if (wheresql != NULL)
                /* G_message(_("Database record for cat %d not used due to SQL
                 * statement")); */
                /* do nothing in this case to not confuse user. Or implement
                 * second cat list */
                ;
            else
                G_warning(_("No record for category %d in table <%s>"), cat,
                          Fi->table);
            continue;
        }

        if (rsm == -1 && scol != NULL) {

            if (sctype == DB_C_TYPE_INT) {
                ret = db_CatValArray_get_value_int(&sarray, cat, &intval);
                sm = intval;
            }
            else { /* DB_C_TYPE_DOUBLE */
                ret = db_CatValArray_get_value_double(&sarray, cat, &sm);
            }
        }

        G_debug(3, "%f %f %f %f", x, y, z, w);

        k++;
        w = w * wmult;
        z = z * zmult;
        c1 = x - ((struct octdata *)(root->data))->x_orig;
        c2 = ((struct octdata *)(root->data))->x_orig +
             ((struct octdata *)(root->data))->n_cols * ew_res - x;
        c3 = y - ((struct octdata *)(root->data))->y_orig;
        c4 = ((struct octdata *)(root->data))->y_orig +
             ((struct octdata *)(root->data))->n_rows * ns_res - y;
        c5 = z - ((struct octdata *)(root->data))->z_orig;
        c6 = ((struct octdata *)(root->data))->z_orig +
             ((struct octdata *)(root->data))->n_levs * tb_res - z;

        if (!((c1 >= 0) && (c2 >= 0) && (c3 >= 0) && (c4 >= 0) && (c5 >= 0) &&
              (c6 >= 0))) {
            if (!OUTRANGE) {
                G_warning(_("Some points outside of region -- will ignore..."));
            }
            OUTRANGE++;
        }
        else {
            if (!(point = point_new(x, y, z, w, sm))) {
                clean();
                G_fatal_error(_("Cannot allocate memory for point"));
            }

            a = OT_insert_oct(point, root);
            if (a == 0) {
                NPOINT++;
            }
            if (a < 0) {
                G_warning(_("Can't insert %lf,%lf,%lf,%lf,%lf a=%d"), x, y, z,
                          w, sm, a);
                Vect_destroy_field_info(Fi);
                db_close_database_shutdown_driver(Driver);
                db_CatValArray_free(&cvarr);
                return -1;
            }

            if (first_time) {
                first_time = 0;
                xmin = x;
                ymin = y;
                zmin = z;
                wmin = w;
                xmax = x;
                ymax = y;
                zmax = z;
                wmax = w;
            }

            xmin = amin1(xmin, x);
            ymin = amin1(ymin, y);
            zmin = amin1(zmin, z);
            wmin = amin1(wmin, w);
            xmax = amax1(xmax, x);
            ymax = amax1(ymax, y);
            zmax = amax1(zmax, z);
            wmax = amax1(wmax, w);
        }
    } /* while */

    db_CatValArray_free(&cvarr);

    c1 = xmin - ((struct octdata *)(root->data))->x_orig;
    c2 = ((struct octdata *)(root->data))->x_orig +
         ((struct octdata *)(root->data))->n_cols * ew_res - xmax;
    c3 = ymin - ((struct octdata *)(root->data))->y_orig;
    c4 = ((struct octdata *)(root->data))->y_orig +
         ((struct octdata *)(root->data))->n_rows * ns_res - ymax;
    c5 = zmin - ((struct octdata *)(root->data))->z_orig;
    c6 = ((struct octdata *)(root->data))->z_orig +
         ((struct octdata *)(root->data))->n_levs * tb_res - zmax;

    if ((c1 > 5 * ew_res) || (c2 > 5 * ew_res) || (c3 > 5 * ns_res) ||
        (c4 > 5 * ns_res) || (c5 > 5 * tb_res) || (c6 > 5 * tb_res)) {
        static int once = 0;

        if (!once) {
            once = 1;
            G_warning(_("Strip exists with insufficient data"));
        }
    }

    nz = wmin;
    totsegm = translate_oct(root, ((struct octdata *)(root->data))->x_orig,
                            ((struct octdata *)(root->data))->y_orig,
                            ((struct octdata *)(root->data))->z_orig, nz);
    if (!totsegm) {
        clean();
        G_fatal_error(_("Zero segments!"));
    }

    ((struct octdata *)(root->data))->x_orig = 0;
    ((struct octdata *)(root->data))->y_orig = 0;
    ((struct octdata *)(root->data))->z_orig = 0; /* was commented out */

    if (outz != NULL)
        ddisk += disk;
    if (gradient != NULL)
        ddisk += disk;
    if (aspect1 != NULL)
        ddisk += disk;
    if (ncurv != NULL)
        ddisk += disk;
    if (gcurv != NULL)
        ddisk += disk;
    if (mcurv != NULL)
        ddisk += disk;

    G_message("Processing all selected output files will require %d bytes of "
              "disk space for temp files",
              ddisk);

    /*
       fprintf(stderr,"xmin=%lf,xmax=%lf,ymin=%lf,ymax=%lf,zmin=%lf,zmax=%lf,
       wmin=%lf,wmax=%lf\n",xmin,xmax,ymin,ymax,zmin,zmax,wmin,wmax);
     */

    fprintf(stderr, "\n");
    if (OUTRANGE > 0)
        G_warning(_("There are points outside specified 2D/3D region--ignored "
                    "%d points (total points: %d)"),
                  OUTRANGE, k);
    if (NPOINT > 0)
        G_warning(_("Points are more dense than specified 'DMIN'--ignored %d "
                    "points (remain %d)"),
                  NPOINT, k - NPOINT);
    NPOINT = k - NPOINT - NPT - OUTRANGE;
    if (NPOINT < KMIN) {
        if (NPOINT != 0) {
            G_warning(_("%d points given for interpolation (after thinning) is "
                        "less than given NPMIN=%d"),
                      NPOINT, KMIN);
            KMIN = NPOINT;
        }
        else {
            fprintf(stderr, "ERROR: zero points in the given region!\n");
            Vect_destroy_field_info(Fi);
            db_close_database_shutdown_driver(Driver);
            return -1;
        }
    }
    if (NPOINT > KMAXPOINTS && KMIN <= KMAX) {
        fprintf(stderr,
                "ERROR: segmentation parameters set to invalid values: npmin = "
                "%d, segmax = %d \n",
                KMIN, KMAX);
        fprintf(stderr, "for smooth connection of segments, npmin > segmax "
                        "(see manual) \n");
        Vect_destroy_field_info(Fi);
        db_close_database_shutdown_driver(Driver);
        return -1;
    }

    if (NPOINT < KMAXPOINTS && KMAX != KMAXPOINTS)
        G_warning(_("There is less than %d points for interpolation, no "
                    "segmentation is necessary, to run the program faster, set "
                    "segmax=%d (see manual)"),
                  KMAXPOINTS, KMAXPOINTS);

    deltx = xmax - xmin;
    delty = ymax - ymin;
    deltz = zmax - zmin;
    nsg = (double)NPOINT / (double)KMIN;
    dnorm = deltx * delty * deltz / nsg;
    nsg = 3.0;
    nsg = 1. / nsg;
    dnorm = pow(dnorm, nsg);
    /* DEBUG
       if (fd4 != NULL)
       fprintf (fd4, "deltx,delty %f %f \n", deltx, delty);
     */
    nsizc = current_region.cols; /* ((int)(deltx/ew_res))+1;  */
    nsizr = current_region.rows; /* ((int)(delty/ns_res))+1;   */
    NPT = k;
    x0utm = 0.;
    y0utm = 0.;
    z0utm = 0.;

    /** create a bitmap mask from given raster map **/
    if (maskmap != NULL) {
        mapsetm = G_find_raster2(maskmap, "");
        if (!mapsetm) {
            clean();
            G_fatal_error(_("Mask raster map [%s] not found"), maskmap);
        }
        bitmask = BM_create(nsizc, nsizr);
        cellmask = Rast_allocate_c_buf();
        cfmask = Rast_open_old(maskmap, mapsetm);
        for (i = 0; i < nsizr; i++) {
            irev = nsizr - i - 1;
            Rast_get_c_row(cfmask, cellmask, i);
            for (j = 0; j < nsizc; j++) {
                if ((cellmask[j] == 0) || Rast_is_c_null_value(&cellmask[j]))
                    BM_set(bitmask, j, irev, 0);
                else
                    BM_set(bitmask, j, irev, 1);
            }
        }
        G_message(_("Bitmap mask created"));
    }
    Vect_destroy_field_info(Fi);
    db_close_database_shutdown_driver(Driver);

    return 1;
}

/*
 * OUTGR now writes 3d raster maps (mca 2/15/96)
 */

int OUTGR(void)
{
    void *cf1, *cf2, *cf3, *cf4, *cf5, *cf6, *cf7;
    size_t read_val;
    FCELL *cell;
    float *data;
    int i, iarc, cnt;
    int bmask = 1;
    int x, y;
    float value;

    if ((cellinp != NULL) && (cellout != NULL)) {
        cell = Rast_allocate_f_buf();

        for (i = 0; i < nsizr; i++) {
            /* seek to the right row */
            G_fseek(Tmp_fd_cell,
                    ((off_t)(nsizr - 1 - i) * nsizc * sizeof(FCELL)), 0);
            read_val = fread(cell, sizeof(FCELL), nsizc, Tmp_fd_cell);
            if (read_val != (size_t)nsizc) {
                clean();
                G_fatal_error(_("Unable to read data from temp file"));
            }
            Rast_put_f_row(fdcout, cell);
        }
    }

    /*** Initialize output g3d region ***/
    current_region.bottom = z_orig_in;
    current_region.top = nsizl * tb_res_in + z_orig_in;

    if (!(data = (float *)G_malloc(sizeof(float) * nsizr * nsizc * nsizl))) {
        clean();
        G_fatal_error(_("Out of memory"));
    }

    /*** Write elevation results ***/
    if (outz != NULL) {

        cf1 = Rast3d_open_new_opt_tile_size(outz, RASTER3D_USE_CACHE_DEFAULT,
                                            &current_region, FCELL_TYPE, 32);
        if (cf1 == NULL) {
            clean();
            G_fatal_error(_("Unable to open %s for writing"), outz);
        }

        /* seek to the beginning */
        G_fseek(Tmp_fd_z, 0L, 0);

        /* Read data in from temp file */
        read_val =
            fread(data, sizeof(float), (size_t)nsizr * nsizc * nsizl, Tmp_fd_z);
        if (read_val != (size_t)nsizr * nsizc * nsizl) {
            clean();
            G_fatal_error(_("Unable to read data from temp file"));
        }

        cnt = 0;
        for (iarc = 0; iarc < nsizl; iarc++) {

            for (y = nsizr - 1; y >= 0; y--) { /* changed by AV */
                for (x = 0; x < nsizc; x++) {
                    if (maskmap != NULL)
                        bmask = BM_get(bitmask, x, nsizr - y - 1);
                    else
                        bmask = 1;
                    value = data[cnt];
                    if (!bmask)
                        Rast3d_set_null_value(&value, 1, FCELL_TYPE);
                    if (Rast3d_put_float(cf1, x, y, iarc, value) == 0) {
                        clean();
                        G_fatal_error(
                            _("Error writing cell (%d,%d,%d) with value %f"), x,
                            y, iarc, value);
                    }

                    cnt++;
                }
            }
        }

        /* Close the file */
        if (Rast3d_close(cf1) == 0) {
            clean();
            G_fatal_error(_("Error closing output file %s"), outz);
        }
        else
            G_message(_("3D raster map <%s> created"), outz);
    }

    /*** Write out the gradient results ***/
    if (gradient != NULL) {

        cf2 =
            Rast3d_open_new_opt_tile_size(gradient, RASTER3D_USE_CACHE_DEFAULT,
                                          &current_region, FCELL_TYPE, 32);
        if (cf2 == NULL) {
            clean();
            G_fatal_error(_("Unable to open %s for writing"), gradient);
        }

        /* seek to the beginning */
        G_fseek(Tmp_fd_dx, 0L, 0);

        /* Read data in from temp file */
        read_val = fread(data, sizeof(float), (size_t)nsizr * nsizc * nsizl,
                         Tmp_fd_dx);
        if (read_val != (size_t)nsizr * nsizc * nsizl) {
            clean();
            G_fatal_error(_("Unable to read data from temp file"));
        }

        cnt = 0;
        for (iarc = 0; iarc < nsizl; iarc++) {

            for (y = nsizr - 1; y >= 0; y--) { /* changed by AV */
                for (x = 0; x < nsizc; x++) {
                    if (maskmap != NULL)
                        bmask = BM_get(bitmask, x, nsizr - y - 1);
                    else
                        bmask = 1;
                    value = data[cnt];
                    if (!bmask)
                        Rast3d_set_null_value(&value, 1, FCELL_TYPE);
                    if (Rast3d_put_float(cf2, x, y, iarc, value) == 0) {
                        clean();
                        G_fatal_error(
                            _("Error writing cell (%d,%d,%d) with value %f"), x,
                            y, iarc, value);
                    }

                    cnt++;
                }
            }
        }

        /* Close the file */
        if (Rast3d_close(cf2) == 0) {
            clean();
            G_fatal_error(_("Error closing output file %s"), gradient);
        }
        else
            G_message(_("3D raster map <%s> created"), gradient);
    }

    /*** Write out aspect1 results ***/
    if (aspect1 != NULL) {

        cf3 = Rast3d_open_new_opt_tile_size(aspect1, RASTER3D_USE_CACHE_DEFAULT,
                                            &current_region, FCELL_TYPE, 32);
        if (cf3 == NULL) {
            clean();
            G_fatal_error(_("Unable to open %s for writing"), aspect1);
        }

        /* seek to the beginning */
        G_fseek(Tmp_fd_dy, 0L, 0);

        /* Read data in from temp file */
        read_val = fread(data, sizeof(float), (size_t)nsizr * nsizc * nsizl,
                         Tmp_fd_dy);
        if (read_val != (size_t)nsizr * nsizc * nsizl) {
            clean();
            G_fatal_error(_("Unable to read data from temp file"));
        }

        cnt = 0;
        for (iarc = 0; iarc < nsizl; iarc++) {

            for (y = nsizr - 1; y >= 0; y--) { /* changed by AV */
                for (x = 0; x < nsizc; x++) {
                    if (maskmap != NULL)
                        bmask = BM_get(bitmask, x, nsizr - y - 1);
                    else
                        bmask = 1;
                    value = data[cnt] * 180 / M_PI;
                    if (!bmask)
                        Rast3d_set_null_value(&value, 1, FCELL_TYPE);
                    if (Rast3d_put_float(cf3, x, y, iarc, value) == 0) {
                        clean();
                        G_fatal_error(
                            _("Error writing cell (%d,%d,%d) with value %f"), x,
                            y, iarc, value);
                    }

                    cnt++;
                }
            }
        }

        /* Close the file */
        if (Rast3d_close(cf3) == 0) {
            clean();
            G_fatal_error(_("Error closing output file %s"), aspect1);
        }
        else
            G_message(_("3D raster map <%s> created"), aspect1);
    }

    /*** Write out aspect2 results ***/
    if (aspect2 != NULL) {

        cf4 = Rast3d_open_new_opt_tile_size(aspect2, RASTER3D_USE_CACHE_DEFAULT,
                                            &current_region, FCELL_TYPE, 32);
        if (cf4 == NULL) {
            clean();
            G_fatal_error(_("Unable to open %s for writing"), aspect2);
        }

        /* seek to the beginning */
        G_fseek(Tmp_fd_dz, 0L, 0);

        /* Read data in from temp file */
        read_val = fread(data, sizeof(float), (size_t)nsizr * nsizc * nsizl,
                         Tmp_fd_dz);
        if (read_val != (size_t)nsizr * nsizc * nsizl) {
            clean();
            G_fatal_error(_("Unable to read data from temp file"));
        }

        cnt = 0;
        for (iarc = 0; iarc < nsizl; iarc++) {

            for (y = nsizr - 1; y >= 0; y--) { /* changed by AV */
                for (x = 0; x < nsizc; x++) {
                    if (maskmap != NULL)
                        bmask = BM_get(bitmask, x, nsizr - y - 1);
                    else
                        bmask = 1;
                    value = data[cnt] * 180 / M_PI;
                    if (!bmask)
                        Rast3d_set_null_value(&value, 1, FCELL_TYPE);
                    if (Rast3d_put_float(cf4, x, y, iarc, value) == 0) {
                        clean();
                        G_fatal_error(
                            _("Error writing cell (%d,%d,%d) with value %f"), x,
                            y, iarc, value);
                    }

                    cnt++;
                }
            }
        }

        /* Close the file */
        if (Rast3d_close(cf4) == 0) {
            clean();
            G_fatal_error(_("Error closing output file %s"), aspect2);
        }
        else
            G_message(_("3D raster map <%s> created"), aspect2);
    }

    /*** Write out ncurv results ***/
    if (ncurv != NULL) {

        cf5 = Rast3d_open_new_opt_tile_size(ncurv, RASTER3D_USE_CACHE_DEFAULT,
                                            &current_region, FCELL_TYPE, 32);
        if (cf5 == NULL) {
            clean();
            G_fatal_error(_("Unable to open %s for writing"), ncurv);
        }

        /* seek to the beginning */
        G_fseek(Tmp_fd_xx, 0L, 0);

        /* Read data in from temp file */
        read_val = fread(data, sizeof(float), (size_t)nsizr * nsizc * nsizl,
                         Tmp_fd_xx);
        if (read_val != (size_t)nsizr * nsizc * nsizl) {
            clean();
            G_fatal_error(_("Unable to read data from temp file"));
        }

        cnt = 0;
        for (iarc = 0; iarc < nsizl; iarc++) {

            for (y = nsizr - 1; y >= 0; y--) { /* changed by AV */
                for (x = 0; x < nsizc; x++) {
                    if (maskmap != NULL)
                        bmask = BM_get(bitmask, x, nsizr - y - 1);
                    else
                        bmask = 1;
                    value = data[cnt];
                    if (!bmask)
                        Rast3d_set_null_value(&value, 1, FCELL_TYPE);
                    if (Rast3d_put_float(cf5, x, y, iarc, value) == 0) {
                        clean();
                        G_fatal_error(
                            _("Error writing cell (%d,%d,%d) with value %f"), x,
                            y, iarc, value);
                    }

                    cnt++;
                }
            }
        }

        /* Close the file */
        if (Rast3d_close(cf5) == 0) {
            clean();
            G_fatal_error(_("Error closing output file %s"), ncurv);
        }
        else
            G_message(_("3D raster map <%s> created"), ncurv);
    }

    /*** Write out gcurv results ***/
    if (gcurv != NULL) {

        cf6 = Rast3d_open_new_opt_tile_size(gcurv, RASTER3D_USE_CACHE_DEFAULT,
                                            &current_region, FCELL_TYPE, 32);
        if (cf6 == NULL) {
            clean();
            G_fatal_error(_("Unable to open %s for writing"), gcurv);
        }

        /* seek to the beginning */
        G_fseek(Tmp_fd_yy, 0L, 0);

        /* Read data in from temp file */
        read_val = fread(data, sizeof(float), (size_t)nsizr * nsizc * nsizl,
                         Tmp_fd_yy);
        if (read_val != (size_t)nsizr * nsizc * nsizl) {
            clean();
            G_fatal_error(_("Unable to read data from temp file"));
        }

        cnt = 0;
        for (iarc = 0; iarc < nsizl; iarc++) {

            for (y = nsizr - 1; y >= 0; y--) { /* changed by AV */
                for (x = 0; x < nsizc; x++) {
                    if (maskmap != NULL)
                        bmask = BM_get(bitmask, x, nsizr - y - 1);
                    else
                        bmask = 1;
                    value = data[cnt];
                    if (!bmask)
                        Rast3d_set_null_value(&value, 1, FCELL_TYPE);
                    if (Rast3d_put_float(cf6, x, y, iarc, value) == 0) {
                        clean();
                        G_fatal_error(
                            _("Error writing cell (%d,%d,%d) with value %f"), x,
                            y, iarc, value);
                    }

                    cnt++;
                }
            }
        }

        /* Close the file */
        if (Rast3d_close(cf6) == 0) {
            clean();
            G_fatal_error(_("Error closing output file %s"), gcurv);
        }
        else
            G_message(_("3D raster map <%s> created"), gcurv);
    }

    /*** Write mcurv results ***/
    if (mcurv != NULL) {

        cf7 = Rast3d_open_new_opt_tile_size(mcurv, RASTER3D_USE_CACHE_DEFAULT,
                                            &current_region, FCELL_TYPE, 32);
        if (cf7 == NULL) {
            clean();
            G_fatal_error(_("Unable to open %s for writing"), mcurv);
        }

        /* seek to the beginning */
        G_fseek(Tmp_fd_xy, 0L, 0);

        /* Read data in from temp file */
        read_val = fread(data, sizeof(float), (size_t)nsizr * nsizc * nsizl,
                         Tmp_fd_xy);
        if (read_val != (size_t)nsizr * nsizc * nsizl) {
            clean();
            G_fatal_error(_("Unable to read data from temp file"));
        }

        cnt = 0;
        for (iarc = 0; iarc < nsizl; iarc++) {

            for (y = nsizr - 1; y >= 0; y--) { /* changed by AV */
                for (x = 0; x < nsizc; x++) {
                    if (maskmap != NULL)
                        bmask = BM_get(bitmask, x, nsizr - y - 1);
                    else
                        bmask = 1;
                    value = data[cnt];
                    if (!bmask)
                        Rast3d_set_null_value(&value, 1, FCELL_TYPE);
                    if (Rast3d_put_float(cf7, x, y, iarc, value) == 0) {
                        clean();
                        G_fatal_error(
                            _("Error writing cell (%d,%d,%d) with value %f"), x,
                            y, iarc, value);
                    }

                    cnt++;
                }
            }
        }

        /* Close the file */
        if (Rast3d_close(cf7) == 0) {
            clean();
            G_fatal_error(_("Error closing output file %s"), mcurv);
        }
        else
            G_message(_("3D raster map <%s> created"), mcurv);
    }

    G_free(data);

    return 1;
}
