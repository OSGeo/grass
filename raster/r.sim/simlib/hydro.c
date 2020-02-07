
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

struct _points points;

char *elevin;
char *dxin;
char *dyin;
char *rain;
char *infil;
char *traps;
char *manin;
char *depth;
char *disch;
char *err;
char *outwalk;
char *observation;
char *logfile;
char *mapset;
char *mscale;
char *tserie;

char *wdepth;
char *detin;
char *tranin;
char *tauin;
char *tc;
char *et;
char *conc;
char *flux;
char *erdep;

char *rainval;
char *maninval;
char *infilval;

struct seed seed;

double xmin, ymin, xmax, ymax;
double mayy, miyy, maxx, mixx;
int mx, my;
int mx2, my2;

double bxmi, bymi, bxma, byma, bresx, bresy;
int maxwab;
double step, conv;

double frac;
double bxmi, bymi;

float **zz, **cchez;
double **v1, **v2, **slope;
double **gama, **gammas, **si, **inf, **sigma;
float **dc, **tau, **er, **ct, **trap;
float **dif;

/* suspected BUG below: fist array subscripts go from 1 to MAXW
 * correct: from 0 to MAXW - 1, e.g for (lw = 0; lw < MAXW; lw++) */
double vavg[MAXW][2], stack[MAXW][3], w[MAXW][3]; 
int iflag[MAXW];

double hbeta;
double hhmax, sisum, vmean;
double infsum, infmean;
int maxw, maxwa, nwalk;
double rwalk, bresx, bresy, xrand, yrand;
double stepx, stepy, xp0, yp0;
double chmean, si0, deltap, deldif, cch, hhc, halpha;
double eps;
int maxwab, nstack;
int iterout, mx2o, my2o;
int miter, nwalka;
double timec;
int ts, timesec;

double rain_val;
double manin_val;
double infil_val;

struct History history;	/* holds meta-data (title, comments,..) */

/* **************************************************** */
/*       create walker representation of si */
/* ******************************************************** */
/*                       .......... iblock loop */

void main_loop(void)
{

    int i, ii, l, k;
    int icoub, nmult; 
    int iw, iblock, lw;
    int itime, iter1;
    int nfiterh, nfiterw;
    int mgen, mgen2, mgen3;
    int nblock;
    int icfl;
    int mitfac;
/*  int mitfac, p; */
    double x, y;
    double velx, vely, stxm, stym;
    double factor, conn, gaux, gauy;
    double d1, addac, decr;
    double barea, sarea, walkwe;
    double gen, gen2, wei2, wei3, wei, weifac;
    float eff;

    nblock = 1;
    icoub = 0;
    icfl = 0;
    nstack = 0; 

    if (maxwa > (MAXW - mx * my)) {
	mitfac = maxwa / (MAXW - mx * my);
	nblock = mitfac + 1;
	maxwa = maxwa / nblock;
    }
    
    /* Create the observation points */
    create_observation_points();

    G_debug(2, " maxwa, nblock %d %d", maxwa, nblock);

    for (iblock = 1; iblock <= nblock; iblock++) {
	++icoub;

	lw = 0;
	walkwe = 0.;
	barea = stepx * stepy;
	sarea = bresx * bresy;
	G_debug(2, " barea,sarea,rwalk,sisum: %f %f %f %f", barea, sarea,
		rwalk, sisum);
	/* write hh.walkers0 */

	for (k = 0; k < my; k++) {
	    for (l = 0; l < mx; l++) {	/* run thru the whole area */
		if (zz[k][l] != UNDEF) {

		    x = xp0 + stepx * (double)(l);
		    y = yp0 + stepy * (double)(k);

		    gen = rwalk * si[k][l] / sisum;
		    mgen = (int)gen;
		    wei = gen / (double)(mgen + 1);

		    /*if (si[k][l] != 0.) { */
		    /* this stuff later for multiscale */

		    gen2 =
			(double)maxwab *si[k][l] / (si0 *
						    (double)(mx2o * my2o));
		    gen2 = gen2 * (barea / sarea);
		    mgen2 = (int)gen2;
		    wei2 = gen2 / (double)(mgen2 + 1);
		    mgen3 =
			(int)((double)mgen2 * wei2 / ((double)mgen * wei));
		    nmult = mgen3 + 1;
		    wei3 = gen2 / (double)((mgen + 1) * (mgen2 + 1));
		    weifac = wei3 / wei;
		    /*              } else {
		       nmult = 1;
		       weifac = 1.;
		       fprintf(stderr, "\n zero rainfall excess in cell"); 
		       } */

		    /*G_debug(2, " gen,gen2,wei,wei2,mgen3,nmult: %f %f %f %f %d %d",gen,gen2,wei,wei2,mgen3,nmult);
		     */
		    for (iw = 1; iw <= mgen + 1; iw++) {	/* assign walkers */

			if (lw >= MAXW)  /* max valid value is MAXW - 1, not MAXW */
			    G_fatal_error(_("nwalk (%d) > maxw (%d)!"), lw, MAXW);

			w[lw][0] = x + stepx * (simwe_rand() - 0.5);
			w[lw][1] = y + stepy * (simwe_rand() - 0.5);
			w[lw][2] = wei;

			walkwe += w[lw][2];
			vavg[lw][0] = v1[k][l];
			vavg[lw][1] = v2[k][l];
			if (w[lw][0] >= xmin && w[lw][1] >= ymin &&
			    w[lw][0] <= xmax && w[lw][1] <= ymax) {
			    iflag[lw] = 0;
			}
			else {
			    iflag[lw] = 1;
			}
			lw++;
		    }
		}		/*DEFined area */
	    }
	}
	nwalk = lw;
	G_debug(2, " nwalk, maxw %d %d", nwalk, MAXW);
	G_debug(2, " walkwe (walk weight),frac %f %f", walkwe, frac);

	stxm = stepx * (double)(mx + 1) - xmin;
	stym = stepy * (double)(my + 1) - ymin;
	nwalka = 0;
	deldif = sqrt(deltap) * frac;	/* diffuse factor */


	factor = deltap * sisum / (rwalk * (double)nblock);

	G_debug(2, " deldif,factor %f %e", deldif, factor);

	/* ********************************************************** */
	/*       main loop over the projection time */
	/* *********************************************************** */


	G_debug(2, "main loop over the projection time... ");

	for (i = 1; i <= miter; i++) {	/* iteration loop depending on simulation time and deltap */
	    G_percent(i, miter, 1);
	    iter1 = i / iterout;
	    iter1 *= iterout;
	    if (iter1 == i) {
		nfiterw = i / iterout + 10;
		nfiterh = i / iterout + 40;
		G_debug(2, "iblock=%d i=%d miter=%d nwalk=%d nwalka=%d",
			iblock, i, miter, nwalk, nwalka);
	    }

	    if (nwalka == 0 && i > 1)
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
	    nstack = 0;

#pragma omp parallel firstprivate(l,lw,k,decr,d1,hhc,velx,vely,eff,gaux,gauy)//nwalka
{
#if defined(_OPENMP)
        int steps = (int)((((double)nwalk) / ((double) omp_get_num_threads())) + 0.5);
        int tid = omp_get_thread_num();
        int min_loop = tid * steps;
        int max_loop = ((tid + 1) * steps) > nwalk ? nwalk : (tid + 1) * steps;

	    for (lw = min_loop; lw < max_loop; lw++) {
#else
        for (lw = 0; lw < nwalk; lw++) {
#endif
		if (w[lw][2] > EPS) {	/* check the walker weight */
		    ++nwalka;
		    l = (int)((w[lw][0] + stxm) / stepx) - mx - 1;
		    k = (int)((w[lw][1] + stym) / stepy) - my - 1;

		    if (l > mx - 1 || k > my - 1 || k < 0 || l < 0) {

			G_debug(2, " k,l=%d,%d", k, l);
			printf("    lw,w=%d %f %f", lw, w[lw][1], w[lw][2]);
			G_debug(2, "    stxym=%f %f", stxm, stym);
			printf("    step=%f %f", stepx, stepy);
			G_debug(2, "    m=%d %d", my, mx);
			printf("    nwalka,nwalk=%d %d", nwalka, nwalk);
			G_debug(2, "  ");
		    }

		    if (zz[k][l] != UNDEF) {
			if (infil != NULL) {	/* infiltration part */
			    if (inf[k][l] - si[k][l] > 0.) {

				decr = pow(addac * w[lw][2], 3. / 5.);	/* decreasing factor in m */
				if (inf[k][l] > decr) {
				    inf[k][l] -= decr;	/* decrease infilt. in cell and eliminate the walker */
				    w[lw][2] = 0.;
				}
				else {
				    w[lw][2] -= pow(inf[k][l], 5. / 3.) / addac;	/* use just proportional part of the walker weight */
				    inf[k][l] = 0.;

				}
			    }
			}

			gama[k][l] += (addac * w[lw][2]);	/* add walker weigh to water depth or conc. */

			d1 = gama[k][l] * conn;
#if defined(_OPENMP)
			gasdev_for_paralel(&gaux, &gauy);
#else
			gaux = gasdev();
			gauy = gasdev();
#endif
			hhc = pow(d1, 3. / 5.);

			if (hhc > hhmax && wdepth == NULL) {	/* increased diffusion if w.depth > hhmax */
			    dif[k][l] = (halpha + 1) * deldif;
			    velx = vavg[lw][0];
			    vely = vavg[lw][1];
			}
			else {
			    dif[k][l] = deldif;
			    velx = v1[k][l];
			    vely = v2[k][l];
			}


			if (traps != NULL && trap[k][l] != 0.) {	/* traps */

			    eff = simwe_rand();	/* random generator */

			    if (eff <= trap[k][l]) {
				velx = -0.1 * v1[k][l];	/* move it slightly back */
				vely = -0.1 * v2[k][l];
			    }
			}

			w[lw][0] += (velx + dif[k][l] * gaux);	/* move the walker */
			w[lw][1] += (vely + dif[k][l] * gauy);

			if (hhc > hhmax && wdepth == NULL) {
			    vavg[lw][0] = hbeta * (vavg[lw][0] + v1[k][l]);
			    vavg[lw][1] = hbeta * (vavg[lw][1] + v2[k][l]);
			}

			if (w[lw][0] <= xmin || w[lw][1] <= ymin || w[lw][0]
			    >= xmax || w[lw][1] >= ymax) {
			    w[lw][2] = 1e-10;	/* eliminate walker if it is out of area */
			}
			else {
			    if (wdepth != NULL) {
				l = (int)((w[lw][0] + stxm) / stepx) - mx - 1;
				k = (int)((w[lw][1] + stym) / stepy) - my - 1;
				w[lw][2] *= sigma[k][l];
			    }

			}	/* else */
		    }		/*DEFined area */
		    else {
			w[lw][2] = 1e-10;	/* eliminate walker if it is out of area */
		    }
		}
            } /* lw loop */
            }
            /* Changes made by Soeren 8. Mar 2011 to replace the site walker output implementation */
            /* Save all walkers located within the computational region and with valid 
               z coordinates */
            if (outwalk != NULL && (i == miter || i == iter1)) {
                nstack = 0;
                
                for (lw = 0; lw < nwalk; lw++) {
                    /* Compute the  elevation raster map index */
                    l = (int)((w[lw][0] + stxm) / stepx) - mx - 1;
                    k = (int)((w[lw][1] + stym) / stepy) - my - 1;
                    
		    /* Check for correct elevation raster map index */
		    if(l < 0 || l >= mx || k < 0 || k >= my)
			 continue;

                    if (w[lw][2] > EPS && zz[k][l] != UNDEF) {

                        /* Save the 3d position of the walker */
                        stack[nstack][0] = mixx / conv + w[lw][0] / conv;
                        stack[nstack][1] = miyy / conv + w[lw][1] / conv;
                        stack[nstack][2] = zz[k][l];

                        nstack++;
                    }
                } /* lw loop */
            } 

	    if (i == iter1 && ts == 1) {
            /* call output for iteration output */
                if (erdep != NULL)
                    erod(gama);	/* divergence of gama field */

                conn = (double)nblock / (double)iblock;
                itime = (int)(i * deltap * timec);
                ii = output_data(itime, conn);
                if (ii != 1)
                    G_fatal_error(_("Unable to write raster maps"));
	    }
            
            /* Write the water depth each time step at an observation point */
            if(points.is_open)
            {
                double value = 0.0;
                int p;
                fprintf(points.output, "%.6d ", i);
                /* Write for each point */
                for(p = 0; p < points.npoints; p++)
                {
                    l = (int)((points.x[p] - mixx + stxm) / stepx) - mx - 1;
		    k = (int)((points.y[p] - miyy + stym) / stepy) - my - 1;
                    
		    if (zz[k][l] != UNDEF) {

			if (wdepth == NULL) 
			    value = step * gama[k][l] * cchez[k][l];
			else
			    value = gama[k][l] * slope[k][l];	

			fprintf(points.output, "%2.4f ", value);
		    } else {
                        /* Point is invalid, so a negative value is written */
			fprintf(points.output, "%2.4f ", -1.0);
                    }		
                }
                fprintf(points.output, "\n");
            }
	}			/* miter */

      L_800:
      /* Soeren 8. Mar 2011: Why is this commented out?*/
	/*        if (iwrib != nblock) {
	   icount = icoub / iwrib;

	   if (icoub == (icount * iwrib)) {
	   ++icfl;
	   nflw = icfl + 50;
	   conn = (double) nblock / (double) iblock;

	   }
	   } */


	if (err != NULL) {
	    for (k = 0; k < my; k++) {
		for (l = 0; l < mx; l++) {
		    if (zz[k][l] != UNDEF) {
			d1 = gama[k][l] * (double)conn;
			gammas[k][l] += pow(d1, 3. / 5.);
		    }		/* DEFined area */
		}
	    }
	}
	if (erdep != NULL)
	    erod(gama);
    }
    /*                       ........ end of iblock loop */

    /* Write final maps here because we know the last time stamp here */
    if (ts == 0) {
        conn = (double)nblock / (double)iblock;
        itime = (int)(i * deltap * timec);
        ii = output_data(itime, conn);
        if (ii != 1)
	    G_fatal_error(_("Cannot write raster maps"));
    }
    /* Close the observation logfile */
    if(points.is_open)
        fclose(points.output);
    
    points.is_open = 0;

}
