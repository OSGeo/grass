/*!
 * \file segmen2d.c
 *
 * \author H. Mitasova, I. Kosinovsky, D. Gerdes (single core)
 * \author Stanislav Zubal, Michal Lacko (OpenMP version)
 * \author Anna Petrasova (OpenMP version GRASS integration)
 *
 * \copyright
 * (C) 1993-2017 by Helena Mitasova and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the
 * GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS
 * for details.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#if defined(_OPENMP)
#include <omp.h>
#endif
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/interpf.h>
#include <grass/gmath.h>

static int cut_tree(struct multtree *, struct multtree **, int *);


/*!
 * See documentation for IL_interp_segments_2d.
 * This is a parallel processing implementation.
 */
int IL_interp_segments_2d_parallel(struct interp_params *params, struct tree_info *info,        /*!< info for the quad tree */
                                   struct multtree *tree,       /*!< current leaf of the quad tree */
                                   struct BM *bitmask,  /*!< bitmask */
                                   double zmin, double zmax,    /*!< min and max input z-values */
                                   double *zminac, double *zmaxac,      /*!< min and max interp. z-values */
                                   double *gmin, double *gmax,  /*!< min and max inperp. slope val. */
                                   double *c1min, double *c1max,        /*!< min and max interp. curv. val. */
                                   double *c2min, double *c2max,        /*!< min and max interp. curv. val. */
                                   double *ertot,       /*!< total interplating func. error */
                                   int totsegm, /*!< total number of segments */
                                   off_t offset1,       /*!< offset for temp file writing */
                                   double dnorm, int threads)
{
    int some_thread_failed = 0;
    int tid;
    int i = 0;
    int j = 0;
    int i_cnt;
    int cursegm = 0;
    double smseg;
    double ***matrix = NULL;
    int **indx = NULL;
    double **b = NULL;
    double **A = NULL;
    struct quaddata **data_local;
    struct multtree **all_leafs;

    all_leafs =
        (struct multtree **)G_malloc(sizeof(struct multtree *) * totsegm);
    data_local =
        (struct quaddata **)G_malloc(sizeof(struct quaddata *) * threads);
    matrix = (double ***)G_malloc(sizeof(double **) * threads);
    indx = (int **)G_malloc(sizeof(int *) * threads);
    b = (double **)G_malloc(sizeof(double *) * threads);
    A = (double **)G_malloc(sizeof(double *) * threads);

    for (i_cnt = 0; i_cnt < threads; i_cnt++) {
        if (!
            (matrix[i_cnt] =
             G_alloc_matrix(params->KMAX2 + 1, params->KMAX2 + 1))) {
            G_fatal_error(_("Out of memory"));
            return -1;
        }
    }

    for (i_cnt = 0; i_cnt < threads; i_cnt++) {
        if (!(indx[i_cnt] = G_alloc_ivector(params->KMAX2 + 1))) {
            G_fatal_error(_("Out of memory"));
            return -1;
        }
    }

    for (i_cnt = 0; i_cnt < threads; i_cnt++) {
        if (!(b[i_cnt] = G_alloc_vector(params->KMAX2 + 3))) {
            G_fatal_error(_("Out of memory"));
            return -1;
        }
    }

    for (i_cnt = 0; i_cnt < threads; i_cnt++) {
        if (!
            (A[i_cnt] =
             G_alloc_vector((params->KMAX2 + 2) * (params->KMAX2 + 2) + 1))) {
            G_fatal_error(_("Out of memory"));
            return -1;
        }
    }

    smseg = smallest_segment(tree, 4);
    cut_tree(tree, all_leafs, &i);

    G_message(_("Starting parallel work"));
#pragma omp parallel firstprivate(tid, i, j, zmin, zmax, tree, totsegm, offset1, dnorm, smseg, ertot, params, info, all_leafs, bitmask, b, indx, matrix, data_local, A) shared(cursegm, threads, some_thread_failed, zminac, zmaxac, gmin, gmax, c1min, c1max, c2min, c2max)  default(none)
    {
#pragma omp for schedule(dynamic)
        for (i_cnt = 0; i_cnt < totsegm; i_cnt++) {
            /* Obtain thread id */
#if defined(_OPENMP)
            tid = omp_get_thread_num();
#endif

            double xmn, xmx, ymn, ymx, distx, disty, distxp, distyp, temp1,
                temp2;
            int npt, nptprev, MAXENC;
            double ew_res, ns_res;
            int MINPTS;
            double pr;
            struct triple *point;
            struct triple skip_point;
            int m_skip, skip_index, k, segtest;
            double xx, yy, zz;


            //struct quaddata *data_local;

            ns_res = (((struct quaddata *)(tree->data))->ymax -
                      ((struct quaddata *)(tree->data))->y_orig) /
                params->nsizr;
            ew_res =
                (((struct quaddata *)(tree->data))->xmax -
                 ((struct quaddata *)(tree->data))->x_orig) / params->nsizc;

            if (all_leafs[i_cnt] == NULL) {
                some_thread_failed = -1;
                continue;
            }
            if (all_leafs[i_cnt]->data == NULL) {
                some_thread_failed = -1;
                continue;
            }
            if (((struct quaddata *)(all_leafs[i_cnt]->data))->points == NULL) {
                continue;
            }
            else {
                distx =
                    (((struct quaddata *)(all_leafs[i_cnt]->data))->n_cols *
                     ew_res) * 0.1;
                disty =
                    (((struct quaddata *)(all_leafs[i_cnt]->data))->n_rows *
                     ns_res) * 0.1;
                distxp = 0;
                distyp = 0;
                xmn = ((struct quaddata *)(all_leafs[i_cnt]->data))->x_orig;
                xmx = ((struct quaddata *)(all_leafs[i_cnt]->data))->xmax;
                ymn = ((struct quaddata *)(all_leafs[i_cnt]->data))->y_orig;
                ymx = ((struct quaddata *)(all_leafs[i_cnt]->data))->ymax;
                i = 0;
                MAXENC = 0;
                /* data is a window with zero points; some fields don't make sense in this case
                   so they are zero (like resolution,dimensions */
                /* CHANGE */
                /* Calcutaing kmin for surrent segment (depends on the size) */

                /*****if (smseg <= 0.00001) MINPTS=params->kmin; else {} ***/
                pr = pow(2., (xmx - xmn) / smseg - 1.);
                MINPTS =
                    params->kmin * (pr /
                                    (1 + params->kmin * pr / params->KMAX2));
                /* fprintf(stderr,"MINPTS=%d, KMIN=%d, KMAX=%d, pr=%lf, smseg=%lf, DX=%lf \n", MINPTS,params->kmin,params->KMAX2,pr,smseg,xmx-xmn); */

                data_local[tid] =
                    (struct quaddata *)quad_data_new(xmn - distx, ymn - disty,
                                                     xmx + distx, ymx + disty,
                                                     0, 0, 0, params->KMAX2);
                npt =
                    MT_region_data(info, tree, data_local[tid], params->KMAX2,
                                   4);

                while ((npt < MINPTS) || (npt > params->KMAX2)) {
                    if (i >= 70) {
                        G_warning(_("Taking too long to find points for interpolation - "
                                   "please change the region to area where your points are. "
                                   "Continuing calculations..."));
                        break;
                    }
                    i++;
                    if (npt > params->KMAX2)
                        /* decrease window */
                    {
                        MAXENC = 1;
                        nptprev = npt;
                        temp1 = distxp;
                        distxp = distx;
                        distx = distxp - fabs(distx - temp1) * 0.5;
                        temp2 = distyp;
                        distyp = disty;
                        disty = distyp - fabs(disty - temp2) * 0.5;
                        /* decrease by 50% of a previous change in window */
                    }
                    else {
                        nptprev = npt;
                        temp1 = distyp;
                        distyp = disty;
                        temp2 = distxp;
                        distxp = distx;
                        if (MAXENC) {
                            disty = fabs(disty - temp1) * 0.5 + distyp;
                            distx = fabs(distx - temp2) * 0.5 + distxp;
                        }
                        else {
                            distx += distx;
                            disty += disty;
                        }
                        /* decrease by 50% of extra distance */
                    }
                    data_local[tid]->x_orig = xmn - distx;      /* update window */
                    data_local[tid]->y_orig = ymn - disty;
                    data_local[tid]->xmax = xmx + distx;
                    data_local[tid]->ymax = ymx + disty;
                    data_local[tid]->n_points = 0;
                    npt =
                        MT_region_data(info, tree, data_local[tid],
                                       params->KMAX2, 4);
                }

                if (totsegm != 0 && tid == 0) {
                    G_percent(cursegm, totsegm, 1);
                }
                data_local[tid]->n_rows =
                    ((struct quaddata *)(all_leafs[i_cnt]->data))->n_rows;
                data_local[tid]->n_cols =
                    ((struct quaddata *)(all_leafs[i_cnt]->data))->n_cols;

                /* for printing out overlapping segments */
                ((struct quaddata *)(all_leafs[i_cnt]->data))->x_orig =
                    xmn - distx;
                ((struct quaddata *)(all_leafs[i_cnt]->data))->y_orig =
                    ymn - disty;
                ((struct quaddata *)(all_leafs[i_cnt]->data))->xmax =
                    xmx + distx;
                ((struct quaddata *)(all_leafs[i_cnt]->data))->ymax =
                    ymx + disty;

                data_local[tid]->x_orig = xmn;
                data_local[tid]->y_orig = ymn;
                data_local[tid]->xmax = xmx;
                data_local[tid]->ymax = ymx;

                /* allocate memory for CV points only if cv is performed */
                if (params->cv) {
                    if (!
                        (point =
                         (struct triple *)G_malloc(sizeof(struct triple) *
                                                   data_local[tid]->
                                                   n_points))) {
                        G_warning(_("Out of memory"));
                        some_thread_failed = -1;
                        continue;
                    }
                }

                /*normalize the data so that the side of average segment is about 1m */
                /* put data_points into point only if CV is performed */

                for (i = 0; i < data_local[tid]->n_points; i++) {
                    data_local[tid]->points[i].x =
                        (data_local[tid]->points[i].x -
                         data_local[tid]->x_orig) / dnorm;
                    data_local[tid]->points[i].y =
                        (data_local[tid]->points[i].y -
                         data_local[tid]->y_orig) / dnorm;
                    if (params->cv) {
                        point[i].x = data_local[tid]->points[i].x;      /*cv stuff */
                        point[i].y = data_local[tid]->points[i].y;      /*cv stuff */
                        point[i].z = data_local[tid]->points[i].z;      /*cv stuff */
                    }

                    /* commented out by Helena january 1997 as this is not necessary
                       although it may be useful to put normalization of z back? 
                       data->points[i].z = data->points[i].z / dnorm;
                       this made smoothing self-adjusting  based on dnorm
                       if (params->rsm < 0.) data->points[i].sm = data->points[i].sm / dnorm;
                     */
                }

                /* cv stuff */
                if (params->cv) {
                    m_skip = data_local[tid]->n_points;
                }
                else {
                    m_skip = 1;
                }
                /* remove after cleanup - this is just for testing */
                skip_point.x = 0.;
                skip_point.y = 0.;
                skip_point.z = 0.;

                for (skip_index = 0; skip_index < m_skip; skip_index++) {
                    if (params->cv) {
                        segtest = 0;
                        j = 0;
                        xx = point[skip_index].x * dnorm +
                            data_local[tid]->x_orig + params->x_orig;
                        yy = point[skip_index].y * dnorm +
                            data_local[tid]->y_orig + params->y_orig;
                        zz = point[skip_index].z;
                        if (xx >= data_local[tid]->x_orig + params->x_orig &&
                            xx <= data_local[tid]->xmax + params->x_orig &&
                            yy >= data_local[tid]->y_orig + params->y_orig &&
                            yy <= data_local[tid]->ymax + params->y_orig) {
                            segtest = 1;
                            skip_point.x = point[skip_index].x;
                            skip_point.y = point[skip_index].y;
                            skip_point.z = point[skip_index].z;
                            for (k = 0; k < m_skip; k++) {
                                if (k != skip_index && params->cv) {
                                    data_local[tid]->points[j].x = point[k].x;
                                    data_local[tid]->points[j].y = point[k].y;
                                    data_local[tid]->points[j].z = point[k].z;
                                    j++;
                                }
                            }
                        }       /* segment area test */
                    }
                    if (!params->cv) {
                        if (    /* params */
                               IL_matrix_create_alloc(params,
                                                      data_local[tid]->points,
                                                      data_local[tid]->
                                                      n_points, matrix[tid],
                                                      indx[tid],
                                                      A[tid]) < 0) {
                            some_thread_failed = -1;
                            continue;
                        }
                    }
                    else if (segtest == 1) {
                        if (    /* params */
                               IL_matrix_create_alloc(params,
                                                      data_local[tid]->points,
                                                      data_local[tid]->
                                                      n_points - 1,
                                                      matrix[tid], indx[tid],
                                                      A[tid]) < 0) {
                            some_thread_failed = -1;
                            continue;
                        }
                    }
                    if (!params->cv) {
                        for (i = 0; i < data_local[tid]->n_points; i++) {
                            b[tid][i + 1] = data_local[tid]->points[i].z;
                        }
                        b[tid][0] = 0.;
                        G_lubksb(matrix[tid], data_local[tid]->n_points + 1,
                                 indx[tid], b[tid]);
                        /* put here condition to skip error if not needed */
                        params->check_points(params, data_local[tid], b[tid],
                                             ertot, zmin, dnorm, skip_point);
                    }
                    else if (segtest == 1) {
                        for (i = 0; i < data_local[tid]->n_points - 1; i++) {
                            b[tid][i + 1] = data_local[tid]->points[i].z;
                        }
                        b[tid][0] = 0.;
                        G_lubksb(matrix[tid], data_local[tid]->n_points,
                                 indx[tid], b[tid]);
                        params->check_points(params, data_local[tid], b[tid],
                                             ertot, zmin, dnorm, skip_point);
                    }
                }               /*end of cv loop */


                if (!params->cv) {
                    if ((params->Tmp_fd_z != NULL) ||
                        (params->Tmp_fd_dx != NULL) ||
                        (params->Tmp_fd_dy != NULL) ||
                        (params->Tmp_fd_xx != NULL) ||
                        (params->Tmp_fd_yy != NULL) ||
                        (params->Tmp_fd_xy != NULL)) {
#pragma omp critical
                        {
                            if (params->grid_calc
                                (params, data_local[tid], bitmask, zmin, zmax,
                                 zminac, zmaxac, gmin, gmax, c1min, c1max,
                                 c2min, c2max, ertot, b[tid], offset1,
                                 dnorm) < 0) {
                                some_thread_failed = -1;
                            }
                        }
                    }
                }

                /* show after to catch 100% */
#pragma omp atomic
                cursegm++;
                if (totsegm < cursegm) {
                    G_debug(1, "%d %d", totsegm, cursegm);
                }

                if (totsegm != 0 && tid == 0) {
                    G_percent(cursegm, totsegm, 1);
                }
                /* 
                   G_free_matrix(matrix);
                   G_free_ivector(indx);
                   G_free_vector(b);
                 */
                G_free(data_local[tid]->points);
                G_free(data_local[tid]);
            }
        }
    }                           /* All threads join master thread and terminate */

    for (i_cnt = 0; i_cnt < threads; i_cnt++) {
        G_free(matrix[i_cnt]);
        G_free(indx[i_cnt]);
        G_free(b[i_cnt]);
        G_free(A[i_cnt]);
    }
    G_free(all_leafs);
    G_free(data_local);
    G_free(matrix);
    G_free(indx);
    G_free(b);
    G_free(A);

    if (some_thread_failed != 0) {
        return -1;
    }
    return 1;
}


/* cut given tree into separate leafs */
int cut_tree(struct multtree *tree,     /* tree we want to cut */
             struct multtree **cut_leafs,       /* array of leafs */
             int *where_to_add /* index of leaf which will be next */ )
{
    if (tree == NULL)
        return -1;
    if (tree->data == NULL)
        return -1;
    if (((struct quaddata *)(tree->data))->points == NULL) {
        int i;

        for (i = 0; i < 4; i++) {
            cut_tree(tree->leafs[i], cut_leafs, where_to_add);
        }
        return 1;
    }
    else {
        cut_leafs[*where_to_add] = tree;
        (*where_to_add)++;
        return 1;
    }
}
