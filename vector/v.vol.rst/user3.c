/*
 ****************************************************************************
 *
 * MODULE:       s.vol.rst: program for 3D(volume) interpolation and geometry
 *               analysis from scattered point data using regularized spline
 *               with tension
 *
 * AUTHOR(S):    Original program (1989) and various modifications:
 *               Lubos Mitas
 *
 *               GRASS 4.2, GRASS 5.0 version and modifications:
 *               H. Mitasova,  I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 * PURPOSE:      s.vol.rst interpolates the values to 3-dimensional grid from
 *               point data (climatic stations, drill holes etc.) given in a
 *               sites file named input. Output grid3 file is elev. 
 *               Regularized spline with tension is used for the
 *               interpolation.
 *
 * COPYRIGHT:    (C) 1989, 1993, 2000 L. Mitas,  H. Mitasova,
 *               I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#include <grass/site.h>
#include "oct.h"
#include "surf.h"
#include "dataoct.h"
#include "userextern.h"
#include "userglobs.h"
#include "user.h"
#include <grass/raster3d.h>
#include "points.h"
#include <grass/bitmap.h>

/* needed for AIX */
#ifdef hz
#undef hz
#endif

int secpar_loop(int ngstc, int nszc, int i)
{
    double dnorm1, ro, dx2, dy2, dz2, grad1, grad2, slp, grad, oor1, oor2,
	curn, curm, curg, dxy2, dxz2, dyz2;
    double dg1, dg2, dg3, dg4, dg5, dg6, h11, h12, h22, h13, h23, h33,
	dm1, dm2, dm3, dm4, dm5, dm6, dnorm5;
    double gradmin;
    int bmask = 1;
    static int first_t = 1;

    ro = 57.295779;
    gradmin = 0.0;
    /*
       for (i = ngstc; i <= nszc; i++)
       { */
    /*      
       if(maskmap != NULL)
       {
       bmask = BM_get(bitmask, i, k);
       } */
    if (bmask == 1) {
	dx2 = adx[i] * adx[i];
	dy2 = ady[i] * ady[i];
	dz2 = adz[i] * adz[i];
	grad1 = dx2 + dy2;
	grad2 = dx2 + dy2 + dz2;
	grad = sqrt(grad2);	/* gradient */
	/* slope in %        slp = 100. * grad; */
	/* slope in degrees
	   slp = ro * atan (grad); */
	slp = atan(grad);
	if ((aspect1 != NULL) || (aspect2 != NULL)) {
	    if (grad <= gradmin) {
		oor1 = 0.;	/* horiz. angle */
		oor2 = 0.;	/* vertical angle */
	    }
	}

/***********aspect from r.slope.aspect, with adx, ady computed
	from interpol. function RST **************************/

	if (aspect1 != NULL) {
	    if (adx[i] == 0) {
		/*       if (ady[i] > 0) oor1 = M_PI / 2;
		   else oor1 = M_PI + M_PI / 2; */
		if (ady[i] > 0)
		    oor1 = 90.;
		else
		    oor1 = 270.;
	    }
	    else {
		/*                       oor1 = atan2(ady[i],adx[i]);
		   if(oor1 <= 0) oor1 = 2 * M_PI + oor1; */
		oor1 = ro * atan2(ady[i], adx[i]);
		if (oor1 <= 0)
		    oor1 = 360. + oor1;
	    }
	}

 /** vertical angle */
	if (aspect2 != NULL) {
	    if (adz[i] == 0) {
		oor2 = 0.;
	    }
	    else {
		/*                      oor2 = atan2( adz[i], sqrt(grad1) ); */
		oor2 = ro * atan2(adz[i], sqrt(grad1));
	    }
	}
	dnorm1 = sqrt(grad2 + 1.);
	dnorm5 = dnorm1 * dnorm1 * dnorm1 * dnorm1 * dnorm1;
	if (ncurv != NULL) {
	    dxy2 = 2. * adxy[i] * adx[i] * ady[i];
	    dxz2 = 2. * adxz[i] * adx[i] * adz[i];
	    dyz2 = 2. * adyz[i] * ady[i] * adz[i];
	    curn =
		-(adxx[i] * dx2 + dxy2 + dxz2 + dyz2 + adzz[i] * dz2 +
		  adyy[i] * dy2) / grad2;
	}
	if (gcurv != NULL) {
	    dg1 = -adxx[i] * adyy[i] * adzz[i];
	    dg2 = -adxy[i] * adxz[i] * adyz[i];
	    dg3 = -adxz[i] * adxy[i] * adyz[i];
	    dg4 = adyz[i] * adyz[i] * adxx[i];
	    dg5 = adxy[i] * adxy[i] * adzz[i];
	    dg6 = adxz[i] * adxz[i] * adyy[i];
	    curg = (dg1 + dg2 + dg3 + dg4 + dg5 + dg6) / dnorm5;
	}
	if (mcurv != NULL) {
	    h11 = -adxx[i] / dnorm1 + 2 * (1 + dx2);
	    h12 = -adxy[i] / dnorm1 + 2 * (adx[i] * ady[i]);
	    h22 = -adyy[i] / dnorm1 + 2 * (1 + dy2);
	    h13 = -adxz[i] / dnorm1 + 2 * (adx[i] * adz[i]);
	    h23 = -adyz[i] / dnorm1 + 2 * (ady[i] * adz[i]);
	    h33 = -adzz[i] / dnorm1 + 2 * (1 + dz2);
	    dm1 = h11 * h22 * h33;
	    dm2 = -h23 * h11 * h23;
	    dm3 = -h12 * h33 * h12;
	    dm4 = -h13 * h13 * h22;
	    dm5 = h12 * h23 * h13;
	    dm6 = h13 * h12 * h23;
	    curm = (dm1 + dm2 + dm3 + dm4 + dm5 + dm6) / (3. * (grad2 + 1.));
	}
	/*   temp = grad2 + 1.; */
	if (first_t) {
	    first_t = 0;
	    if (gradient != NULL)
		gmax = gmin = slp;
	    if (aspect1 != NULL)
		a1max = a1min = oor1;
	    if (aspect2 != NULL)
		a2max = a2min = oor2;
	    if (ncurv != NULL)
		c1max = c1min = curn;
	    if (gcurv != NULL)
		c2max = c2min = curg;
	    if (mcurv != NULL)
		c3max = c3min = curm;
	}

	if (gradient != NULL) {
	    gmin = amin1(gmin, slp);
	    gmax = amax1(gmax, slp);
	}
	if (aspect1 != NULL) {
	    a1min = amin1(a1min, oor1);
	    a1max = amax1(a1max, oor1);
	}
	if (aspect2 != NULL) {
	    a2min = amin1(a2min, oor2);
	    a2max = amax1(a2max, oor2);
	}
	if (ncurv != NULL) {
	    c1min = amin1(c1min, curn);
	    if (curn < 10.)
		c1max = amax1(c1max, curn);
	}
	if (gcurv != NULL) {
	    c2min = amin1(c2min, curg);
	    if (curg < 10.)
		c2max = amax1(c2max, curg);
	}
	if (mcurv != NULL) {
	    c3min = amin1(c3min, curm);
	    if (curn < 10.)
		c3max = amax1(c3max, curm);
	}

	if (gradient != NULL)
	    adx[i] = slp;
	if (aspect1 != NULL)
	    ady[i] = oor1 / ro;
	if (aspect2 != NULL)
	    adz[i] = oor2 / ro;
	if (ncurv != NULL)
	    adxx[i] = curn;	/* change of gradient */
	if (gcurv != NULL)
	    adyy[i] = curg;	/* Gaussian curvature */
	if (mcurv != NULL)
	    adxy[i] = curm;	/* Mean curvature */
	/*printf(" parametre grad %lf\n", slp); */
    }
    /*      } secapr loop */

    return 1;
}




int
COGRR1(double x_or, double y_or, double z_or, int n_rows, int n_cols,
       int n_levs, int n_points, struct quadruple *points,
       struct point_3d skip_point)

/*C
   C       INTERPOLATION BY FUNCTIONAL METHOD : TPS + complete regul.
   c
 */
{
    int secpar_loop();
    static double *w2 = NULL;
    static double *wz2 = NULL;
    static double *wz1 = NULL;
    double amaxa;
    double stepix, stepiy, stepiz, RO, xx, yy, zz, xg, yg, zg, xx2;
    double wm, dx, dy, dz, dxx, dyy, dxy, dxz, dyz, dzz, h, bmgd1,
	bmgd2, etar, zcon, r, ww, wz, r2, hcell, zzcell2,
	etarcell, rcell, wwcell, zzcell;
    double x_crs,x_crsd,x_crsdd,x_crsdr2;
    int n1, k1, k2, k, i1, l, l1, n4, n5, m, i;
    int NGST, LSIZE, ngstc, nszc, ngstr, nszr, ngstl, nszl;
    int POINT();
    int ind, ind1;
    static int first_time_z = 1;
    off_t offset, offset1, offset2;
    int bmask = 1;
    static FCELL *cell = NULL;

    int cond1 = (gradient != NULL) || (aspect1 != NULL) || (aspect2 != NULL);
    int cond2 = (ncurv != NULL) || (gcurv != NULL) || (mcurv != NULL);

#define CEULER .57721566
    /*
       C
       c        character*32 fncdsm
       c normalization
       c
     */
    offset1 = nsizr * nsizc;

    stepix = ew_res / dnorm;
    stepiy = ns_res / dnorm;
    stepiz = tb_res / dnorm;

    if (!w2) {
	if (!(w2 = (double *)G_malloc(sizeof(double) * (KMAX2 + 1))))
	    clean_fatal_error("Cannot allocate w2");
    }
    if (!wz2) {
	if (!(wz2 = (double *)G_malloc(sizeof(double) * (KMAX2 + 1))))
	    clean_fatal_error("Cannot allocate wz2");
    }
    if (!wz1) {
	if (!(wz1 = (double *)G_malloc(sizeof(double) * (KMAX2 + 1))))
	    clean_fatal_error("Cannot allocate wz1");
    }

    if (cell == NULL)
	cell = Rast_allocate_f_buf();

    for (i = 1; i <= n_points; i++) {
	points[i - 1].x = (points[i - 1].x - x_or) / dnorm;
	points[i - 1].y = (points[i - 1].y - y_or) / dnorm;
	points[i - 1].z = (points[i - 1].z - z_or) / dnorm;
    }
    if (cv) {
	skip_point.x = (skip_point.x - x_or) / dnorm;
	skip_point.y = (skip_point.y - y_or) / dnorm;
	skip_point.z = (skip_point.z - z_or) / dnorm;
    }
    n1 = n_points + 1;
    /*
       C
       C      GENERATION OF MATRIX
       C
       C      FIRST COLUMN
       C
     */
    A[1] = 0.;
    for (k = 1; k <= n_points; k++) {
	i1 = k + 1;
	A[i1] = 1.;
    }
    /*
       C
       C      OTHER COLUMNS
       C
     */
    RO = rsm;
    for (k = 1; k <= n_points; k++) {
	k1 = k * n1 + 1;
	k2 = k + 1;
	i1 = k1 + k;
	if (rsm < 0.) {		/*indicates variable smoothing */
	    A[i1] = points[k - 1].sm;
	}
	else {
	    A[i1] = RO;		/* constant smoothing */
	}
	for (l = k2; l <= n_points; l++) {
	    xx = points[k - 1].x - points[l - 1].x;
	    yy = points[k - 1].y - points[l - 1].y;
	    zz = points[k - 1].z - points[l - 1].z;
	    r = sqrt(xx * xx + yy * yy + zz * zz);
	    etar = (fi * r) / 2.;
	    if (etar == 0.) {
		/*              printf ("ident. points in segm.  \n");
		   printf ("x[%d]=%lf,x[%d]=%lf,y[%d]=%lf,y[%d]=%lf\n",
		   k - 1, points[k - 1].x, l - 1, points[l - 1].x, k - 1, points[k - 1].y, l - 1, points[l - 1].y); */
	    }
	    i1 = k1 + l;
	    A[i1] = crs(etar);
	}
    }
    /*
       C
       C       SYMMETRISATION
       C
     */
    amaxa = 1.;
    for (k = 1; k <= n1; k++) {
	k1 = (k - 1) * n1;
	k2 = k + 1;
	for (l = k2; l <= n1; l++) {
	    m = (l - 1) * n1 + k;
	    A[m] = A[k1 + l];
	    amaxa = amax1(A[m], amaxa);
	}
    }

    /*
       C        RIGHT SIDE
       C
     */
    n4 = n1 * n1 + 1;
    A[n4] = 0.;
    for (l = 1; l <= n_points; l++) {
	l1 = n4 + l;
	A[l1] = points[l - 1].w;
    }
    n5 = n1 * (n1 + 1);
    for (i = 1; i <= n5; i++)
	A[i] = A[i] / amaxa;

    /*
       SOLVING OF SYSTEM
     */

    if (LINEQS(n1, n1, 1, &NERROR, &DETERM)) {

	for (k = 1; k <= n_points; k++) {
	    l = n4 + k;
	    b[k] = A[l];
	}
	b[n_points + 1] = A[n4];

	POINT(n_points, points, skip_point);
	if (cv)
	    return 1;
	if (devi != NULL && sig1 == 1)
	    return 1;
	/*
	   C
	   C         INTERPOLATION   *  MOST INNER LOOPS !
	   C
	 */
	NGST = 1;
	LSIZE = 0;

	ngstc = (int)(x_or / ew_res + 0.5) + 1;
	nszc = ngstc + n_cols - 1;
	ngstr = (int)(y_or / ns_res + 0.5) + 1;
	nszr = ngstr + n_rows - 1;
	ngstl = (int)(z_or / tb_res + 0.5) + 1;
	nszl = ngstl + n_levs - 1;

	/*        fprintf(stderr," Progress percentage for each segment ..." ); */
	/*fprintf(stderr,"Before loops,ngstl = %d,nszl =%d\n",ngstl,nszl); */
	for (i = ngstl; i <= nszl; i++) {
	    /*fprintf(stderr,"level=%d\n",i); */
	    /*      G_percent(i, nszl, 2); */
	    offset = offset1 * (i - 1);	/* levels offset */
	    zg = (i - ngstl) * stepiz;
	    for (m = 1; m <= n_points; m++) {
		wz = zg - points[m - 1].z;
		wz1[m] = wz;
		wz2[m] = wz * wz;
	    }
	    for (k = ngstr; k <= nszr; k++) {
		yg = (k - ngstr) * stepiy;
		for (m = 1; m <= n_points; m++) {
		    wm = yg - points[m - 1].y;
		    w[m] = wm;
		    w2[m] = wm * wm;
		}
		if ((cellinp != NULL) && (cellout != NULL) && (i == ngstl))
		    Rast_get_f_row(fdcell, cell, n_rows_in - k);

		for (l = ngstc; l <= nszc; l++) {
		    LSIZE = LSIZE + 1;
		    if (maskmap != NULL)
			bmask = BM_get(bitmask, l - 1, k - 1);	/*bug fix 02/03/00 jh */
		    xg = (l - ngstc) * stepix;
		    ww = 0.;
		    wwcell = 0.;
		    dx = 0.;
		    dy = 0.;
		    dz = 0.;
		    dxx = 0.;
		    dxy = 0.;
		    dxz = 0.;
		    dyy = 0.;
		    dyz = 0.;
		    dzz = 0.;
		    if (bmask == 1) {	/* compute everything for area which is not masked out */
			h = b[n1];
			hcell = b[n1];
			for (m = 1; m <= n_points; m++) {
			    xx = xg - points[m - 1].x;
			    xx2 = xx * xx;
			    if ((cellinp != NULL) && (cellout != NULL) &&
				(i == ngstl)) {
				zcon = (double)(cell[l - 1] * zmult - z_or) - z_orig_in * zmult;	/* bug fix 02/03/00 jh */
				zcon = zcon / dnorm;
				zzcell = zcon - points[m - 1].z;
				zzcell2 = zzcell * zzcell;
				rcell = sqrt(xx2 + w2[m] + zzcell2);
				etarcell = (fi * rcell) / 2.;
				hcell = hcell + b[m] * crs(etarcell);
			    }
			    r2 = xx2 + w2[m] + wz2[m];
			    r = sqrt(r2);
			    etar = (fi * r) / 2.;

                            crs_full(
                              etar,fi,
                              &x_crs,
                              cond1?&x_crsd:NULL,
                              cond2?&x_crsdr2:NULL,
                              cond2?&x_crsdd:NULL
                            );
                            h = h + b[m] * x_crs;
                            if(cond1)
                            {
                                   bmgd1 = b[m] * x_crsd;
			    dx = dx + bmgd1 * xx;
			    dy = dy + bmgd1 * w[m];
			    dz = dz + bmgd1 * wz1[m];
                            }
                            if(cond2)
                            {
                                   bmgd2 = b[m] * x_crsdd;
                                   bmgd1 = b[m] * x_crsdr2;
			    dyy = dyy + bmgd2 * w2[m] + bmgd1 * w2[m];
			    dzz = dzz + bmgd2 * wz2[m] + bmgd1 * wz2[m];
			    dxy = dxy + bmgd2 * xx * w[m] + bmgd1 * xx * w[m];
                                   dxz = dxz + bmgd2 * xx * wz1[m] + bmgd1 * xx * wz1[m];
                                   dyz = dyz + bmgd2 * w[m] * wz1[m] + bmgd1 * w[m] * wz1[m];
                            }                            
			}
			ww = h + wmin;
			if ((cellinp != NULL) && (cellout != NULL) &&
			    (i == ngstl))
			    wwcell = hcell + wmin;
			A[l] = h;
			az[l] = ww;
			if (first_time_z) {
			    first_time_z = 0;
			    zmaxac = zminac = ww;
			    if ((cellinp != NULL) && (cellout != NULL) &&
				(i == ngstl))
				zmaxacell = zminacell = wwcell;
			}
			zmaxac = amax1(ww, zmaxac);
			zminac = amin1(ww, zminac);
			if ((cellinp != NULL) && (cellout != NULL) &&
			    (i == ngstl)) {
			    zmaxacell = amax1(wwcell, zmaxacell);
			    zminacell = amin1(wwcell, zminacell);
			}
			if ((ww > wmax + 0.1 * (wmax - wmin))
			    || (ww < wmin - 0.1 * (wmax - wmin))) {
			    static int once = 0;

			    if (!once) {
				once = 1;
				fprintf(stderr, "WARNING:\n");
				fprintf(stderr,
					"Overshoot -- increase in tension suggested.\n");
				fprintf(stderr,
					"Overshoot occures at (%d,%d,%d) cell\n",
					l, k, i);
				fprintf(stderr,
					"The w-value is %lf, wmin is %lf,wmax is %lf\n",
					ww, wmin, wmax);
			    }
			}
		    }		/* skip here if you are in masked area, ww should be 0 */
		    az[l] = ww;
		    adx[l] = dx;
		    ady[l] = dy;
		    adz[l] = dz;
		    /*              printf("\n %f", ww); */
		    adxx[l] = dxx;
		    adxy[l] = dxy;
		    adxz[l] = dxz;
		    adyy[l] = dyy;
		    adyz[l] = dyz;
		    adzz[l] = dzz;
		    if ((gradient != NULL) || (aspect1 != NULL) ||
			(aspect2 != NULL)
			|| (ncurv != NULL) || (gcurv != NULL) ||
			(mcurv != NULL))
			if (!(secpar_loop(ngstc, nszc, l)))
			    clean_fatal_error("Secpar_loop failed");
		    if ((cellinp != NULL) && (cellout != NULL) &&
			(i == ngstl)) {
			zero_array_cell[l - 1] = (FCELL) (wwcell);
		    }
		    if (outz != NULL) {
			zero_array1[l - 1] = (float)(az[l] * sciz);
		    }
		    if (gradient != NULL) {
			zero_array2[l - 1] = (float)(adx[l]);
		    }
		    if (aspect1 != NULL) {
			zero_array3[l - 1] = (float)(ady[l]);
		    }
		    if (aspect2 != NULL) {
			zero_array4[l - 1] = (float)(adz[l]);
		    }
		    if (ncurv != NULL) {
			zero_array5[l - 1] = (float)(adxx[l]);
		    }
		    if (gcurv != NULL) {
			zero_array6[l - 1] = (float)(adyy[l]);
		    }
		    if (mcurv != NULL) {
			zero_array7[l - 1] = (float)(adxy[l]);
		    }
		}		/* columns */
		ind = nsizc * (k - 1) + (ngstc - 1);
		ind1 = ngstc - 1;
		offset2 = offset + ind;	/* rows*cols offset */

		if ((cellinp != NULL) && (cellout != NULL) && (i == ngstl)) {
		    G_fseek(Tmp_fd_cell, ((off_t)ind * sizeof(FCELL)), 0);
		    if (!
			(fwrite
			 (zero_array_cell + ind1, sizeof(FCELL),
			  nszc - ngstc + 1, Tmp_fd_cell)))
			clean_fatal_error
			    ("Not enough disk space--cannot write files");
		}
		if (outz != NULL) {
		    G_fseek(Tmp_fd_z, (off_t)(offset2 * sizeof(float)), 0);
		    if (!
			(fwrite
			 (zero_array1 + ind1, sizeof(float), nszc - ngstc + 1,
			  Tmp_fd_z)))
			clean_fatal_error
			    ("Not enough disk space--cannot write files");
		}
		if (gradient != NULL) {
		    G_fseek(Tmp_fd_dx, (off_t)(offset2 * sizeof(float)), 0);
		    if (!
			(fwrite
			 (zero_array2 + ind1, sizeof(float), nszc - ngstc + 1,
			  Tmp_fd_dx)))
			clean_fatal_error
			    ("Not enough disk space--cannot write files");
		}
		if (aspect1 != NULL) {
		    G_fseek(Tmp_fd_dy, (off_t)(offset2 * sizeof(float)), 0);
		    if (!
			(fwrite
			 (zero_array3 + ind1, sizeof(float), nszc - ngstc + 1,
			  Tmp_fd_dy)))
			clean_fatal_error
			    ("Not enough disk space--cannot write files");
		}
		if (aspect2 != NULL) {
		    G_fseek(Tmp_fd_dz, (off_t)(offset2 * sizeof(float)), 0);
		    if (!
			(fwrite
			 (zero_array4 + ind1, sizeof(float), nszc - ngstc + 1,
			  Tmp_fd_dz)))
			clean_fatal_error
			    ("Not enough disk space--cannot write files");
		}
		if (ncurv != NULL) {
		    G_fseek(Tmp_fd_xx, (off_t)(offset2 * sizeof(float)), 0);
		    if (!
			(fwrite
			 (zero_array5 + ind1, sizeof(float), nszc - ngstc + 1,
			  Tmp_fd_xx)))
			clean_fatal_error
			    ("Not enough disk space--cannot write files");
		}
		if (gcurv != NULL) {
		    G_fseek(Tmp_fd_yy, (off_t)(offset2 * sizeof(float)), 0);
		    if (!
			(fwrite
			 (zero_array6 + ind1, sizeof(float), nszc - ngstc + 1,
			  Tmp_fd_yy)))
			clean_fatal_error
			    ("Not enough disk space--cannot write files");
		}
		if (mcurv != NULL) {
		    G_fseek(Tmp_fd_xy, (off_t)(offset2 * sizeof(float)), 0);
		    if (!
			(fwrite
			 (zero_array7 + ind1, sizeof(float), nszc - ngstc + 1,
			  Tmp_fd_xy)))
			clean_fatal_error
			    ("Not enough disk space--cannot write files");
		}

	    }
	}
    }				/* falls here if LINEQS() returns 0 */
    /*    total++; */
    /*fprintf(stderr,"wminac=%lf,wmaxac=%lf\n",zminac,zmaxac); */
    return 1;

}




int POINT(int n_points, struct quadruple *points, struct point_3d skip_point)

/*
   c  interpolation check of z-values in given points
   c
 */
{
    double errmax, h, xx, yy, r2, hz, zz, ww, err, xmm, ymm,
	zmm, wmm, r, etar;
    int n1, mm, m, mmax, inside;
    Site *site;

    if ((site = G_site_new_struct(-1, 3, 0, 1)) == NULL)
	G_fatal_error("Memory error for site struct");
    errmax = .0;
    n1 = n_points + 1;
    if (!cv) {
	for (mm = 1; mm <= n_points; mm++) {
	    h = b[n1];
	    for (m = 1; m <= n_points; m++) {
		xx = points[mm - 1].x - points[m - 1].x;
		yy = points[mm - 1].y - points[m - 1].y;
		zz = points[mm - 1].z - points[m - 1].z;
		r2 = yy * yy + xx * xx + zz * zz;
		r = sqrt(r2);
		etar = (fi * r) / 2.;
		h = h + b[m] * crs(etar);
	    }
	    hz = h + wmin;
	    ww = points[mm - 1].w + wmin;
	    err = hz - ww;

	    xmm = (points[mm - 1].x * dnorm) + xmn + current_region.west;
	    ymm = (points[mm - 1].y * dnorm) + ymn + current_region.south;
	    zmm =
		(points[mm - 1].z * dnorm) / zmult + zmn / zmult +
		current_region.bottom;

	    if ((xmm >= xmn + current_region.west) &&
		(xmm <= xmx + current_region.west) &&
		(ymm >= ymn + current_region.south) &&
		(ymm <= ymx + current_region.south) &&
		(zmm >= zmn / zmult + current_region.bottom) &&
		(zmm <= zmx / zmult + current_region.bottom))
		inside = 1;
	    else
		inside = 0;
	    if (devi != NULL && inside == 1)
		point_save(xmm, ymm, zmm, err);

	    if (err < 0) {
		err = -err;
	    }
	    if (err >= errmax) {
		errmax = err;
		mmax = mm;
	    }
	}

	ertot = amax1(errmax, ertot);
	if (errmax > ertre) {
	    xmm = (points[mmax - 1].x * dnorm) +
		((struct octdata *)(root->data))->x_orig;
	    ymm = (points[mmax - 1].y * dnorm) +
		((struct octdata *)(root->data))->y_orig;
	    zmm = (points[mmax - 1].z * dnorm) +
		((struct octdata *)(root->data))->z_orig;
	    wmm = points[mmax - 1].w + wmin;
	    /*      printf (" max. error = %f at point i = %d \n", errmax, mmax);
	       printf (" x(i) = %f  y(i) = %f \n", xmm, ymm);
	       printf (" z(i) = %f  w(i) = %f \n", zmm, wmm); */
	}
    }

    /* cv stuff */
    if (cv) {

	h = b[n1];		/* check this if h=b[0] used in 2d should be applied here */
	for (m = 1; m <= n_points; m++) {	/* number of points is already 1 less (skip_point) */
	    xx = points[m - 1].x - skip_point.x;
	    yy = points[m - 1].y - skip_point.y;
	    zz = points[m - 1].z - skip_point.z;

	    r2 = yy * yy + xx * xx + zz * zz;
	    if (r2 != 0.) {
		r = sqrt(r2);
		etar = (fi * r) / 2.;
		h = h + b[m] * crs(etar);
	    }
	}
	hz = h + wmin;
	ww = skip_point.w + wmin;
	err = hz - ww;
	xmm = (skip_point.x * dnorm) + xmn + current_region.west;
	ymm = (skip_point.y * dnorm) + ymn + current_region.south;
	zmm =
	    (skip_point.z * dnorm) / zmult + zmn / zmult +
	    current_region.bottom;

	if ((xmm >= xmn + current_region.west) &&
	    (xmm <= xmx + current_region.west) &&
	    (ymm >= ymn + current_region.south) &&
	    (ymm <= ymx + current_region.south) &&
	    (zmm >= zmn / zmult + current_region.bottom) &&
	    (zmm <= zmx / zmult + current_region.bottom))
	    inside = 1;
	else
	    inside = 0;

	if (inside == 1)
	    point_save(xmm, ymm, zmm, err);

    }				/* cv */


    return 1;
}
