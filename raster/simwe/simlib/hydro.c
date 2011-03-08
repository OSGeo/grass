
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
/* #include <grass/site.h> */
#include <grass/bitmap.h>
#include <grass/linkm.h>
#include <grass/glocale.h>

#include <grass/waterglobs.h>

struct options parm;
struct flags flag;

/*
 * Soeren 8. Mar 2011 TODO:
 * Put all these global variables into several meaningful structures and 
 * document use and purpose.
 * 
 * Example:
 * Put all file descriptors into a input_files struct and rename the variables:
 * input_files.elev 
 * input_files.dx 
 * input_files.dy 
 * input_files.drain
 * ... 
 * 
 */

FILE *fdelevin, *fddxin, *fddyin, *fdrain, *fdinfil, *fdtraps,
    *fdmanin, *fddepth, *fddisch, *fderr;
FILE *fdwdepth, *fddetin, *fdtranin, *fdtauin, *fdtc, *fdet, *fdconc,
    *fdflux, *fderdep;
FILE *fdsfile, *fw;

char *elevin;
char *dxin;
char *dyin;
char *rain;
char *infil;
char *traps;
char *manin;
/* char *sfile; */
char *depth;
char *disch;
char *err;
char *outwalk; 
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

struct Cell_head cellhd;

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
			++lw;

			if (lw > MAXW)
			    G_fatal_error(_("nwalk (%d) > maxw (%d)!"), lw, MAXW);

			w[lw][1] = x + stepx * (ulec() - 0.5);
			w[lw][2] = y + stepy * (ulec() - 0.5);
			w[lw][3] = wei;

			walkwe += w[lw][3];
			vavg[lw][1] = v1[k][l];
			vavg[lw][2] = v2[k][l];
			if (w[lw][1] >= xmin && w[lw][2] >= ymin &&
			    w[lw][1] <= xmax && w[lw][2] <= ymax) {
			    iflag[lw] = 0;
			}
			else {
			    iflag[lw] = 1;
			}

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

	    for (lw = 1; lw <= nwalk; lw++) {
		if (w[lw][3] > EPS) {	/* check the walker weight */
		    ++nwalka;
		    l = (int)((w[lw][1] + stxm) / stepx) - mx - 1;
		    k = (int)((w[lw][2] + stym) / stepy) - my - 1;

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

				decr = pow(addac * w[lw][3], 3. / 5.);	/* decreasing factor in m */
				if (inf[k][l] > decr) {
				    inf[k][l] -= decr;	/* decrease infilt. in cell and eliminate the walker */
				    w[lw][3] = 0.;
				}
				else {
				    w[lw][3] -= pow(inf[k][l], 5. / 3.) / addac;	/* use just proportional part of the walker weight */
				    inf[k][l] = 0.;

				}
			    }
			}

			gama[k][l] += (addac * w[lw][3]);	/* add walker weigh to water depth or conc. */

			d1 = gama[k][l] * conn;
			hhc = pow(d1, 3. / 5.);

			if (hhc > hhmax && wdepth == NULL) {	/* increased diffusion if w.depth > hhmax */
			    dif[k][l] = (halpha + 1) * deldif;
			    velx = vavg[lw][1];
			    vely = vavg[lw][2];
			}
			else {
			    dif[k][l] = deldif;
			    velx = v1[k][l];
			    vely = v2[k][l];
			}


			if (traps != NULL && trap[k][l] != 0.) {	/* traps */

			    eff = ulec();	/* random generator */

			    if (eff <= trap[k][l]) {
				velx = -0.1 * v1[k][l];	/* move it slightly back */
				vely = -0.1 * v2[k][l];
			    }
			}

			gaux = gasdev();
			gauy = gasdev();

			w[lw][1] += (velx + dif[k][l] * gaux);	/* move the walker */
			w[lw][2] += (vely + dif[k][l] * gauy);

			if (hhc > hhmax && wdepth == NULL) {
			    vavg[lw][1] = hbeta * (vavg[lw][1] + v1[k][l]);
			    vavg[lw][2] = hbeta * (vavg[lw][2] + v2[k][l]);
			}

			if (w[lw][1] <= xmin || w[lw][2] <= ymin || w[lw][1]
			    >= xmax || w[lw][2] >= ymax) {
			    w[lw][3] = 1e-10;	/* eliminate walker if it is out of area */
			}
			else {
			    if (wdepth != NULL) {
				l = (int)((w[lw][1] + stxm) / stepx) - mx - 1;
				k = (int)((w[lw][2] + stym) / stepy) - my - 1;
				w[lw][3] *= sigma[k][l];
			    }

			}	/* else */
		    }		/*DEFined area */
		    else {
			w[lw][3] = 1e-10;	/* eliminate walker if it is out of area */
		    }
		}
            } /* lw loop */
            
            /* Changes made by Soeren 8. Mar 2011 to replace the site walker output implementation */
            /* Save all walkers located within the computational region and with valid 
               z coordinates */
            if ((i == miter || i == iter1)) {	
                nstack = 0;
                
                for (lw = 1; lw <= nwalk; lw++) {
                    /* Compute the  elevation raster map index */
                    l = (int)((w[lw][1] + stxm) / stepx) - mx - 1;
                    k = (int)((w[lw][2] + stym) / stepy) - my - 1;
                    
		    /* Check for correct elevation raster map index */
		    if(l < 0 || l >= mx || k < 0 || k >= my)
			 continue;

                    if (w[lw][3] > EPS && zz[k][l] != UNDEF) {

                        /* Save the 3d position of the walker */
                        stack[nstack][1] = mixx / conv + w[lw][1] / conv;
                        stack[nstack][2] = miyy / conv + w[lw][2] / conv;
                        stack[nstack][3] = zz[k][l];

                        nstack++;
                    }
                }
            } /* lw loop */

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

            /* Soeren 8. Mar 2011 TODO:
             *  This hould be replaced by vector functionality and sql database storage */
/* ascii data site file output for gamma  - hydrograph or sediment*/
/* cchez incl. sqrt(sinsl) */
/* sediment */
/*defined area */
/*
	    if (sfile != NULL) {	

		for (p = 0; p < npoints; p++) {

		    l = (int)((points[p].east - mixx + stxm) / stepx) - mx -
			1;
		    k = (int)((points[p].north - miyy + stym) / stepy) - my -
			1;

		    if (zz[k][l] != UNDEF) {

			if (wdepth == NULL) {
			    points[p].z1 = step * gama[k][l] * cchez[k][l];	
			}
			else
			    points[p].z1 = gama[k][l] * slope[k][l];	

			G_debug(2, " k,l,z1 %d %d %f", k, l, points[p].z1);

			fprintf(fw, "%f %f %f\n", points[p].east / conv,
				points[p].north / conv, points[p].z1);
		    }		

		}

	    }
*/

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

}
