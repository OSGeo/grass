/****************************************************************************
 *
 * MODULE:       simwe library
 * AUTHOR(S):    Helena Mitasova, Jaro Hofierka, Lubos Mitas:
 * PURPOSE:      Hydrologic and sediment transport simulation (SIMWE)
 *
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* hydro.c (simlib), 20.nov.2002, JH */

#include "simlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>
#include <grass/glocale.h>

#include <grass/waterglobs.h>
#include <grass/simlib.h>
/*
 * Soeren 8. Mar 2011 TODO:
 * Put all these global variables into several meaningful structures and
 * document use and purpose.
 *
 */

struct point2D;
struct point3D;

float **zz, **cchez;
double **v1, **v2, **slope;
double **gama, **gammas, **si, **inf, **sigma;
float **dc, **tau, **er, **ct, **trap;
float **dif;

/* int iflag[MAXW]; */
struct point3D *w;
struct point2D *vavg;

/* **************************************************** */
/*       create walker representation of si */
/* ******************************************************** */
/*                       .......... iblock loop */

void main_loop(const Setup *setup, const Geometry *geometry,
               const Settings *settings, Simulation *sim,
               ObservationPoints *points, const Inputs *inputs,
               const Outputs *outputs)
{
    int i, l, k;
    int iblock;
    int iter1;
    double conn;
    double addac;

    int nblock = 1;

    double stxm = geometry->stepx * (double)(geometry->mx + 1) - geometry->xmin;
    double stym = geometry->stepy * (double)(geometry->my + 1) - geometry->ymin;
    double deldif = sqrt(setup->deltap) * settings->frac; /* diffuse factor */

    if (sim->maxwa > (MAXW - geometry->mx * geometry->my)) {
        nblock = 1 + sim->maxwa / (MAXW - geometry->mx * geometry->my);
        sim->maxwa = sim->maxwa / nblock;
    }
    double factor =
        setup->deltap * setup->sisum / (sim->rwalk * (double)nblock);

    G_debug(2, " deldif, factor %f %e", deldif, factor);
    G_debug(2, " maxwa, nblock %d %d", sim->maxwa, nblock);
    G_debug(2, "rwalk,sisum: %f %f", sim->rwalk, setup->sisum);

    for (iblock = 1; iblock <= nblock; iblock++) {
        int lw = 0;
        double walkwe = 0.;

        G_message(_("Processing block %d of %d"), iblock, nblock);

        /* write hh.walkers0 */

        for (k = 0; k < geometry->my; k++) {
            for (l = 0; l < geometry->mx; l++) { /* run thru the whole area */
                if (zz[k][l] != UNDEF) {

                    double x = geometry->xp0 + geometry->stepx * (double)(l);
                    double y = geometry->yp0 + geometry->stepy * (double)(k);

                    double gen = sim->rwalk * si[k][l] / setup->sisum;
                    int mgen = (int)gen;
                    double wei = gen / (double)(mgen + 1);

                    for (int iw = 1; iw <= mgen + 1;
                         iw++) { /* assign walkers */
                        w[lw].x = x + geometry->stepx * (simwe_rand() - 0.5);
                        w[lw].y = y + geometry->stepy * (simwe_rand() - 0.5);
                        w[lw].m = wei;

                        walkwe += w[lw].m;
                        vavg[lw].x = v1[k][l];
                        vavg[lw].y = v2[k][l];
                        lw++;
                    }
                } /* defined area */
            }
        }
        sim->nwalk = lw;
        G_debug(2, " nwalk, maxw %d %d", sim->nwalk, MAXW);
        G_debug(2, " walkwe (walk weight),frac %f %f", walkwe, settings->frac);

        sim->nwalka = 0;
        int nwalka = 0;

        /* ********************************************************** */
        /*       main loop over the projection time */
        /* *********************************************************** */

        G_debug(2, "main loop over the projection time... ");

        for (i = 1; i <= setup->miter;
             i++) { /* iteration loop depending on simulation time and deltap */
            G_percent(i, setup->miter, 1);
            iter1 = i / setup->iterout;
            iter1 *= setup->iterout;
            if (iter1 == i) {
                /* nfiterw = i / iterout + 10;
                   nfiterh = i / iterout + 40; */
                G_debug(2, "iblock=%d i=%d miter=%d nwalk=%d nwalka=%d", iblock,
                        i, setup->miter, sim->nwalk, sim->nwalka);
            }

            if (sim->nwalka == 0 && i > 1)
                goto L_800;

            /* ************************************************************ */
            /*                               .... propagate one step */
            /* ************************************************************ */

            addac = factor;
            conn = (double)nblock / (double)iblock;
            if (i == 1) {
                addac = factor * .5;
            }
            nwalka = 0;
            sim->nstack = 0;

#pragma omp parallel firstprivate(l, lw, k) reduction(+ : nwalka)
            {
#if defined(_OPENMP)
                int steps = (int)((((double)sim->nwalk) /
                                   ((double)omp_get_num_threads())) +
                                  0.5);
                int tid = omp_get_thread_num();
                int min_loop = tid * steps;
                int max_loop = ((tid + 1) * steps) > sim->nwalk
                                   ? sim->nwalk
                                   : (tid + 1) * steps;

                for (lw = min_loop; lw < max_loop; lw++) {
#else
                for (lw = 0; lw < sim->nwalk; lw++) {
#endif
                    if (w[lw].m > EPS) { /* check the walker weight */
                        ++(nwalka);
                        l = (int)((w[lw].x + stxm) / geometry->stepx) -
                            geometry->mx - 1;
                        k = (int)((w[lw].y + stym) / geometry->stepy) -
                            geometry->my - 1;

                        if (l > geometry->mx - 1 || k > geometry->my - 1 ||
                            k < 0 || l < 0) {

                            G_debug(2, " k,l=%d,%d", k, l);
                            printf("    lw,w=%d %f %f", lw, w[lw].y, w[lw].m);
                            G_debug(2, "    stxym=%f %f", stxm, stym);
                            printf("    step=%f %f", geometry->stepx,
                                   geometry->stepy);
                            G_debug(2, "    m=%d %d", geometry->my,
                                    geometry->mx);
                            printf("    nwalka,nwalk=%d %d", sim->nwalka,
                                   sim->nwalk);
                            G_debug(2, "  ");
                        }

                        if (zz[k][l] != UNDEF) {
                            if (inf[k][l] != UNDEF) { /* infiltration part */
                                if (inf[k][l] - si[k][l] > 0.) {

                                    double decr = pow(
                                        addac * w[lw].m,
                                        3. / 5.); /* decreasing factor in m */
                                    if (inf[k][l] > decr) {
                                        inf[k][l] -=
                                            decr; /* decrease infilt. in cell
                                                     and eliminate the walker */
                                        w[lw].m = 0.;
                                    }
                                    else {
                                        w[lw].m -=
                                            pow(inf[k][l], 5. / 3.) /
                                            addac; /* use just proportional part
                                                      of the walker weight */
                                        inf[k][l] = 0.;
                                    }
                                }
                            }

                            gama[k][l] +=
                                (addac * w[lw].m); /* add walker weigh to water
                                                      depth or conc. */

                            double d1 = gama[k][l] * conn;
                            double gaux, gauy;
#if defined(_OPENMP)
                            gasdev_for_paralel(&gaux, &gauy);
#else
                            gaux = gasdev();
                            gauy = gasdev();
#endif
                            double hhc = pow(d1, 3. / 5.);
                            double velx, vely;
                            if (hhc > settings->hhmax &&
                                inputs->wdepth == NULL) { /* increased diffusion
                                                     if w.depth > hhmax */
                                dif[k][l] = (settings->halpha + 1) * deldif;
                                velx = vavg[lw].x;
                                vely = vavg[lw].y;
                            }
                            else {
                                dif[k][l] = deldif;
                                velx = v1[k][l];
                                vely = v2[k][l];
                            }

                            if (inputs->traps != NULL &&
                                trap[k][l] != 0.) { /* traps */

                                float eff = simwe_rand(); /* random generator */

                                if (eff <= trap[k][l]) {
                                    velx = -0.1 *
                                           v1[k][l]; /* move it slightly back */
                                    vely = -0.1 * v2[k][l];
                                }
                            }

                            w[lw].x +=
                                (velx + dif[k][l] * gaux); /* move the walker */
                            w[lw].y += (vely + dif[k][l] * gauy);

                            if (hhc > settings->hhmax &&
                                inputs->wdepth == NULL) {
                                vavg[lw].x =
                                    settings->hbeta * (vavg[lw].x + v1[k][l]);
                                vavg[lw].y =
                                    settings->hbeta * (vavg[lw].y + v2[k][l]);
                            }

                            if (w[lw].x <= geometry->xmin ||
                                w[lw].y <= geometry->ymin ||
                                w[lw].x >= geometry->xmax ||
                                w[lw].y >= geometry->ymax) {
                                w[lw].m = 1e-10; /* eliminate walker if it is
                                                    out of area */
                            }
                            else {
                                if (inputs->wdepth != NULL) {
                                    l = (int)((w[lw].x + stxm) /
                                              geometry->stepx) -
                                        geometry->mx - 1;
                                    k = (int)((w[lw].y + stym) /
                                              geometry->stepy) -
                                        geometry->my - 1;
                                    w[lw].m *= sigma[k][l];
                                }

                            } /* else */
                        } /*DEFined area */
                        else {
                            w[lw].m = 1e-10; /* eliminate walker if it is out of
                                                area */
                        }
                    }
                } /* lw loop */
            }
            /* Total remaining walkers for this iteration */
            sim->nwalka = nwalka;

            /* Changes made by Soeren 8. Mar 2011 to replace the site walker
             * output implementation */
            /* Save all walkers located within the computational region and with
               valid z coordinates */
            if (outputs->outwalk != NULL && (i == setup->miter || i == iter1)) {
                sim->nstack = 0;

                for (lw = 0; lw < sim->nwalk; lw++) {
                    /* Compute the  elevation raster map index */
                    l = (int)((w[lw].x + stxm) / geometry->stepx) -
                        geometry->mx - 1;
                    k = (int)((w[lw].y + stym) / geometry->stepy) -
                        geometry->my - 1;

                    /* Check for correct elevation raster map index */
                    if (l < 0 || l >= geometry->mx || k < 0 ||
                        k >= geometry->my)
                        continue;

                    if (w[lw].m > EPS && zz[k][l] != UNDEF) {

                        /* Save the 3d position of the walker */
                        sim->stack[sim->nstack].x =
                            geometry->mixx / geometry->conv +
                            w[lw].x / geometry->conv;
                        sim->stack[sim->nstack].y =
                            geometry->miyy / geometry->conv +
                            w[lw].y / geometry->conv;
                        sim->stack[sim->nstack].m = zz[k][l];

                        sim->nstack++;
                    }
                } /* lw loop */
            }

            if (i == iter1 && settings->ts) {
                /* call output for iteration output */
                if (outputs->erdep != NULL)
                    erod(gama, setup, geometry); /* divergence of gama field */

                conn = (double)nblock / (double)iblock;
                int itime = (int)(i * setup->deltap * setup->timec);
                int ii = output_data(itime, conn, setup, geometry, settings,
                                     sim, inputs, outputs);
                if (ii != 1)
                    G_fatal_error(_("Unable to write raster maps"));
            }

            /* Write the water depth each time step at an observation point */
            if (points->is_open) {
                double value = 0.0;
                int p;

                fprintf(points->output, "%.6d ", i);
                /* Write for each point */
                for (p = 0; p < points->npoints; p++) {
                    l = (int)((points->x[p] - geometry->mixx + stxm) /
                              geometry->stepx) -
                        geometry->mx - 1;
                    k = (int)((points->y[p] - geometry->miyy + stym) /
                              geometry->stepy) -
                        geometry->my - 1;

                    if (zz[k][l] != UNDEF) {

                        if (inputs->wdepth == NULL)
                            value = geometry->step * gama[k][l] * cchez[k][l];
                        else
                            value = gama[k][l] * slope[k][l];

                        fprintf(points->output, "%2.4f ", value);
                    }
                    else {
                        /* Point is invalid, so a negative value is written */
                        fprintf(points->output, "%2.4f ", -1.0);
                    }
                }
                fprintf(points->output, "\n");
            }
        } /* miter */

    L_800:
        /* Soeren 8. Mar 2011: Why is this commented out? */
        /*        if (iwrib != nblock) {
           icount = icoub / iwrib;

           if (icoub == (icount * iwrib)) {
           ++icfl;
           nflw = icfl + 50;
           conn = (double) nblock / (double) iblock;

           }
           } */

        if (outputs->err != NULL) {
            for (k = 0; k < geometry->my; k++) {
                for (l = 0; l < geometry->mx; l++) {
                    if (zz[k][l] != UNDEF) {
                        double d1 = gama[k][l] * (double)conn;
                        gammas[k][l] += pow(d1, 3. / 5.);
                    } /* DEFined area */
                }
            }
        }
        if (outputs->erdep != NULL)
            erod(gama, setup, geometry);
    }
    /*                       ........ end of iblock loop */

    /* Write final maps here because we know the last time stamp here */
    if (!settings->ts) {
        conn = (double)nblock / (double)iblock;
        int itime = (int)(i * setup->deltap * setup->timec);
        int ii = output_data(itime, conn, setup, geometry, settings, sim,
                             inputs, outputs);
        if (ii != 1)
            G_fatal_error(_("Cannot write raster maps"));
    }
    /* Close the observation logfile */
    if (points->is_open)
        fclose(points->output);

    points->is_open = 0;
}
