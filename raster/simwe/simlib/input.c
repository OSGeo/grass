/* input.c (simlib), 20.nov.2002, JH */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
/* #include <grass/site.h> */
#include <grass/bitmap.h>
#include <grass/glocale.h>
#include <grass/linkm.h>
#include <grass/gmath.h>
#include <grass/waterglobs.h>


/* ************************************************************** */
/*                         GRASS input procedures, allocations    */
/* *************************************************************** */

/*!
 * \brief allocate memory, read input rasters, assign UNDEF to NODATA
 * 
 *  \return int
 * sites related input/output commented out - needs update to vect, HM nov 2008
 */


int input_data(void)
{

    FCELL *elevin_cell, *traps_cell, *manin_cell;
    FCELL *detin_cell, *trainin_cell, *tauin_cell;
    DCELL *dxin_cell, *dyin_cell, *rain_cell, *infil_cell, *wdepth_cell;
    int elevin_fd, dxin_fd, dyin_fd, rain_fd, infil_fd, traps_fd, manin_fd, row, row_rev;
    int detin_fd, trainin_fd, tauin_fd, wdepth_fd;
    int j;
/*    int nn, cc, ii, dd; */
    double unitconv = 0.0000002;	/* mm/hr to m/s */
    const char *mapset;
/* output water depth and discharge at outlet points given in site file*/
/*    Site *site; 

    npoints = 0;
    npoints_alloc = 0;

    if (sfile != NULL) {
	fw = fopen("simwe_data.txt", "w");

	mapset = G_find_sites(sfile, "");
	if (mapset == NULL)
	    G_fatal_error(_("File [%s] not found"), sfile);

	if ((fdsfile = G_fopen_sites_old(sfile, mapset)) == NULL)
	    G_fatal_error(_("Unable to open file [%s]"), sfile);

	if (G_site_describe(fdsfile, &nn, &cc, &ii, &dd) != 0)
	    G_fatal_error(_("Failed to guess file format"));

	site = G_site_new_struct(cc, nn, ii, dd);
	G_message(_("Reading sites map (%s) ..."), sfile);

	   if (dd==0)
	   {
	   fprintf(stderr,"\n");
	   G_warning("I'm finding records that do not have 
	   a floating point attributes (fields prefixed with '%').");
	   } 

	while (G_site_get(fdsfile, site) >= 0) {
	    if (npoints_alloc <= npoints) {
		npoints_alloc += 128;
		points =
		    (struct Point *)G_realloc(points,
					      npoints_alloc *
					      sizeof(struct Point));
	    }
	    points[npoints].east = site->east * conv;
	    points[npoints].north = site->north * conv;
	    points[npoints].z1 = 0.;	
	    if ((points[npoints].east / conv <= cellhd.east &&
		 points[npoints].east / conv >= cellhd.west) &&
		(points[npoints].north / conv <= cellhd.north &&
		 points[npoints].north / conv >= cellhd.south))
		npoints++;
	}
	G_sites_close(fdsfile);
    }
*/

    /* Allocate raster buffers */
    elevin_cell = Rast_allocate_f_buf();
    dxin_cell = Rast_allocate_d_buf();
    dyin_cell = Rast_allocate_d_buf();

    if(manin != NULL)
        manin_cell = Rast_allocate_f_buf();

    if (rain != NULL)
	rain_cell = Rast_allocate_d_buf();

    if (infil != NULL)
	infil_cell = Rast_allocate_d_buf();

    if (traps != NULL)
	traps_cell = Rast_allocate_f_buf();

    if (detin != NULL)
	detin_cell = Rast_allocate_f_buf();

    if (tranin != NULL)
	trainin_cell = Rast_allocate_f_buf();

    if (tauin != NULL)
	tauin_cell = Rast_allocate_f_buf();

    if (wdepth != NULL)
	wdepth_cell = Rast_allocate_d_buf();

    /* Allocate some double dimension arrays for each input */
    zz = G_alloc_fmatrix(my, mx);
    v1 = G_alloc_matrix(my, mx);
    v2 = G_alloc_matrix(my, mx);
    cchez = G_alloc_fmatrix(my, mx);
    
    if (rain != NULL || rain_val >= 0.0) 
	si = G_alloc_matrix(my, mx);

    if (infil != NULL || infil_val >= 0.0)
	inf = G_alloc_matrix(my, mx);

    if (traps != NULL)
	trap = G_alloc_fmatrix(my, mx);

    if (detin != NULL)
	dc = G_alloc_fmatrix(my, mx);

    if (tranin != NULL)
	ct = G_alloc_fmatrix(my, mx);

    if (tauin != NULL)
	tau = G_alloc_fmatrix(my, mx);

    if (wdepth != NULL)
	gama = G_alloc_matrix(my, mx);

    G_debug(3, "Running MAR 2011 version, started modifications on 20080211");

    /* Check if data available in mapsets
     * if found, then open the files */
    elevin_fd = Rast_open_old(elevin, "");

    /* TO REPLACE BY INTERNAL PROCESSING of dx, dy from Elevin */
    if ((mapset = G_find_raster(dxin, "")) == NULL)
	G_fatal_error(_("Raster map <%s> not found"), dxin);

    dxin_fd = Rast_open_old(dxin, "");

    dyin_fd = Rast_open_old(dyin, "");
    /* END OF REPLACEMENT */

    /* Rendered Mannings n input map optional to run! */
    /* Careful!                     (Yann, 20080212) */
    if (manin)
	manin_fd = Rast_open_old(manin, "");

    /* Rendered Rainfall input map optional to run! */
    /* Careful!                     (Yann, 20080212) */
    if (rain)
	rain_fd = Rast_open_old(rain, "");

    if (infil)
	infil_fd = Rast_open_old(infil, "");

    if (traps)
	traps_fd = Rast_open_old(traps, "");

    if (detin)
	detin_fd = Rast_open_old(detin, "");

    if (tranin)
	trainin_fd = Rast_open_old(tranin, "");

    if (tauin)
	tauin_fd = Rast_open_old(tauin, "");

    if (wdepth)
	wdepth_fd = Rast_open_old(wdepth, "");

    for (row = 0; row < my; row++) {
	Rast_get_f_row(elevin_fd, elevin_cell, row);
	Rast_get_d_row(dxin_fd, dxin_cell, row);
	Rast_get_d_row(dyin_fd, dyin_cell, row);

	if (manin)
	    Rast_get_f_row(manin_fd, manin_cell, row);

	if (rain)
	    Rast_get_d_row(rain_fd, rain_cell, row);

	if (infil)
	    Rast_get_d_row(infil_fd, infil_cell, row);

	if (traps)
	    Rast_get_f_row(traps_fd, traps_cell, row);

	if (detin)
	    Rast_get_f_row(detin_fd, detin_cell, row);

	if (tranin)
	    Rast_get_f_row(trainin_fd, trainin_cell, row);

	if (tauin)
	    Rast_get_f_row(tauin_fd, tauin_cell, row);

	if (wdepth)
	    Rast_get_d_row(wdepth_fd, wdepth_cell, row);

	for (j = 0; j < mx; j++) {
	    row_rev = my - row - 1;
	    /*if elevation data exists store in zz[][] */
	    if (!Rast_is_f_null_value(elevin_cell + j))
		zz[row_rev][j] = (float)(conv * elevin_cell[j]);
	    else
		zz[row_rev][j] = UNDEF;

	    if (!Rast_is_d_null_value(dxin_cell + j))
		v1[row_rev][j] = (double)dxin_cell[j];
	    else
		v1[row_rev][j] = UNDEF;

	    if (!Rast_is_d_null_value(dyin_cell + j))
		v2[row_rev][j] = (double)dyin_cell[j];
	    else
		v2[row_rev][j] = UNDEF;

	    /* undef all area if something's missing */
	    if (v1[row_rev][j] == UNDEF || v2[row_rev][j] == UNDEF)
		zz[row_rev][j] = UNDEF;

	    /* should be ? 
	     * if(v1[row_rev][j] == UNDEF || v2[row_rev][j] == UNDEF || 
	     * zz[row_rev][j] == UNDEF) {
	     *      v1[row_rev][j] == UNDEF;
	     *      v2[row_rev][j] == UNDEF;
	     *      zz[row_rev][j] == UNDEF;
	     *      }
	     *//*printout warning? */

	    /* If Rain Exists, then load data */
	    if (rain) {
		if (!Rast_is_d_null_value(rain_cell + j))
		    si[row_rev][j] = ((double)rain_cell[j]) * unitconv;
		/*conv mm/hr to m/s */
		/*printf("\n INPUTrain, convert %f %f",si[row_rev][j],unitconv); */

		else {
		    si[row_rev][j] = UNDEF;
		    zz[row_rev][j] = UNDEF;
		}

		/* Load infiltration map too if it exists */
		if (infil) {
		    if (!Rast_is_d_null_value(infil_cell + j))
			inf[row_rev][j] = (double)infil_cell[j] * unitconv;
		    /*conv mm/hr to m/s */
		    /*printf("\nINPUT infilt,convert %f %f",inf[row_rev][j],unitconv); */
		    else {
			inf[row_rev][j] = UNDEF;
			zz[row_rev][j] = UNDEF;
		    }
		}
		else {		/* Added by Yann 20080216 */
		    /* If infil==NULL, then use infilval */
		    if (infil_val >= 0.0) {
			inf[row_rev][j] = infil_val * unitconv;	/*conv mm/hr to m/s */
			/*      printf("infil_val = %f \n",inf[row_rev][j]); */
		    }
		    else {
			inf[row_rev][j] = UNDEF;
			zz[row_rev][j] = UNDEF;
		    }
		}

		if (traps) {
		    if (!Rast_is_f_null_value(traps_cell + j))
			trap[row_rev][j] = (float)traps_cell[j];	/* no conv, unitless */
		    else {
			trap[row_rev][j] = UNDEF;
			zz[row_rev][j] = UNDEF;
		    }
		}
	    }
	    else {		/* Added by Yann 20080213 */
		/* If rain==NULL, then use rainval */
		if (rain_val >= 0.0) {
		    si[row_rev][j] = rain_val * unitconv;	/* conv mm/hr to m/s */
		    /*printf("\n INPUTrainval, convert %f %f",si[row_rev][j],unitconv); */
		}
		else {
		    si[row_rev][j] = UNDEF;
		    zz[row_rev][j] = UNDEF;
		}

		if (infil) {
		    if (!Rast_is_d_null_value(infil_cell + j))
			inf[row_rev][j] = (double)infil_cell[j] * unitconv;	/*conv mm/hr to m/s */
		    /*printf("\nINPUT infilt,convert %f %f",inf[row_rev][j],unitconv); */
		    else {
			inf[row_rev][j] = UNDEF;
			zz[row_rev][j] = UNDEF;
		    }
		}
		else {		/* Added by Yann 20080216 */
		    /* If infil==NULL, then use infilval */
		    if (infil_val >= 0.0) {
			inf[row_rev][j] = infil_val * unitconv;	/*conv mm/hr to m/s */
			/*printf("infil_val = %f \n",inf[row_rev][j]); */
		    }
		    else {
			inf[row_rev][j] = UNDEF;
			zz[row_rev][j] = UNDEF;
		    }
		}

		if (traps) {
		    if (!Rast_is_f_null_value(traps_cell + j))
			trap[row_rev][j] = (float)traps_cell[j];	/* no conv, unitless */
		    else {
			trap[row_rev][j] = UNDEF;
			zz[row_rev][j] = UNDEF;
		    }
		}
	    }			/* End of added by Yann 20080213 */
	    if (manin) {
		if (!Rast_is_f_null_value(manin_cell + j)) {
		    cchez[row_rev][j] = (float)manin_cell[j];	/* units in manual */
		}
		else {
		    cchez[row_rev][j] = UNDEF;
		    zz[row_rev][j] = UNDEF;
		}
	    }
	    else if (manin_val >= 0.0) {	/* Added by Yann 20080213 */
		cchez[row_rev][j] = (float)manin_val;
	    }
	    else {
		G_fatal_error(_("Raster map <%s> not found, and manin_val undefined, choose one to be allowed to process"),
			      manin);
	    }
	    if (detin) {
		if (!Rast_is_f_null_value(detin_cell + j))
		    dc[row_rev][j] = (float)detin_cell[j];	/*units in manual */
		else {
		    dc[row_rev][j] = UNDEF;
		    zz[row_rev][j] = UNDEF;
		}
	    }

	    if (tranin) {
		if (!Rast_is_f_null_value(trainin_cell + j))
		    ct[row_rev][j] = (float)trainin_cell[j];	/*units in manual */
		else {
		    ct[row_rev][j] = UNDEF;
		    zz[row_rev][j] = UNDEF;
		}
	    }

	    if (tauin) {
		if (!Rast_is_f_null_value(tauin_cell + j))
		    tau[row_rev][j] = (float)tauin_cell[j];	/*units in manual */
		else {
		    tau[row_rev][j] = UNDEF;
		    zz[row_rev][j] = UNDEF;
		}
	    }

	    if (wdepth) {
		if (!Rast_is_d_null_value(wdepth_cell + j))
		    gama[row_rev][j] = (double)wdepth_cell[j];	/*units in manual */
		else {
		    gama[row_rev][j] = UNDEF;
		    zz[row_rev][j] = UNDEF;
		}
	    }
	}
    }
    Rast_close(elevin_fd);
    Rast_close(dxin_fd);
    Rast_close(dyin_fd);

    if (rain)
	Rast_close(rain_fd);

    if (infil)
	Rast_close(infil_fd);

    if (traps)
	Rast_close(traps_fd);
    /* Maybe a conditional to manin!=NULL here ! */
    Rast_close(manin_fd);

	/****************/

    if (detin)
	Rast_close(detin_fd);

    if (tranin)
	Rast_close(trainin_fd);

    if (tauin)
	Rast_close(tauin_fd);

    if (wdepth)
	Rast_close(wdepth_fd);

    return 1;
}


/* data preparations, sigma, shear, etc. */
int grad_check(void)
{
    int k, l, i, j;
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

    sisum = 0.;
    infsum = 0.;
    cmul2 = rhow * gacc;

    /* mandatory alloc. - should be moved to main.c */
    slope = (double **)G_malloc(sizeof(double *) * (my));

    for (l = 0; l < my; l++)
	slope[l] = (double *)G_malloc(sizeof(double) * (mx));

    for (j = 0; j < my; j++) {
	for (i = 0; i < mx; i++)
	    slope[j][i] = 0.;
    }

	/*** */

    for (k = 0; k < my; k++) {
	for (l = 0; l < mx; l++) {
	    if (zz[k][l] != UNDEF) {
		zx = v1[k][l];
		zy = v2[k][l];
		zd2 = zx * zx + zy * zy;
		sinsl = sqrt(zd2) / sqrt(zd2 + 1);	/* sin(terrain slope) */
		/* Computing MIN */
		zd2 = sqrt(zd2);
		zd2min = amin1(zd2min, zd2);
		/* Computing MAX */
		zd2max = amax1(zd2max, zd2);
		zd4 = sqrt(zd2);	/* ^.25 */
		if (cchez[k][l] != 0.) {
		    cchez[k][l] = 1. / cchez[k][l];	/* 1/n */
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
		    if (wdepth)
			hh = pow(gama[k][l], 2. / 3.);
		    /* hh = 1 if there is no water depth input */
		    v1[k][l] = (double)hh *cchez[k][l] * zx / zd4;
		    v2[k][l] = (double)hh *cchez[k][l] * zy / zd4;

		    slope[k][l] =
			sqrt(v1[k][l] * v1[k][l] + v2[k][l] * v2[k][l]);
		}
		if (wdepth) {
		    sheer = (double)(cmul2 * gama[k][l] * sinsl);	/* shear stress */
		    /* if critical shear stress >= shear then all zero */
		    if ((sheer <= tau[k][l]) || (ct[k][l] == 0.)) {
			si[k][l] = 0.;
			sigma[k][l] = 0.;
		    }
		    else {
			si[k][l] = (double)(dc[k][l] * (sheer - tau[k][l]));
			sigma[k][l] = (double)(dc[k][l] / ct[k][l]) * (sheer - tau[k][l]) / (pow(sheer, 1.5));	/* rill erosion=1.5, sheet = 1.1 */
		    }
		}
		sisum += si[k][l];
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
		zmax = amax1(zmax, (double)zz[k][l]);	/* not clear were needed */
		if (wdepth)
		    sigmax = amax1(sigmax, sigma[k][l]);
		cchezmax = amax1(cchezmax, cchez[k][l]);
		/* saved sqrt(sinsl)*cchez to cchez array for output */
		cchez[k][l] *= sqrt(sinsl);
	    }			/* DEFined area */
	}
    }
    if (inf != NULL && smax < infmax)
	G_warning(_("Infiltration exceeds the rainfall rate everywhere! No overland flow."));

    cc = (double)mx *my;

    si0 = sisum / cc;
    vmean = vsum / cc;
    chmean = chsum / cc;

    if (inf)
	infmean = infsum / cc;

    if (wdepth)
	deltaw = 0.8 / (sigmax * vmax);	/*time step for sediment */
    deltap = 0.25 * sqrt(stepx * stepy) / vmean;	/*time step for water */

    if (deltaw > deltap)
	timec = 4.;
    else
	timec = 1.25;

    miter = (int)(timesec / (deltap * timec));	/* number of iterations = number of cells to pass */
    iterout = (int)(iterout / (deltap * timec));	/* number of cells to pass for time series output */

    fprintf(stderr, "\n");
    G_message(_("Min elevation \t= %.2f m\nMax elevation \t= %.2f m\n"), zmin,
	      zmax);
    G_message(_("Mean Source Rate (rainf. excess or sediment) \t= %f m/s or kg/m2s \n"),
	      si0);
    G_message(_("Mean flow velocity \t= %f m/s\n"), vmean);
    G_message(_("Mean Mannings \t= %f\n"), 1.0 / chmean);

    deltap = amin1(deltap, deltaw);

    G_message(_("Number of iterations \t= %d cells\n"), miter);
    G_message(_("Time step \t= %.2f s\n"), deltap);
    if (wdepth) {
	G_message(_("Sigmax \t= %f\nMax velocity \t= %f m/s\n"), sigmax,
		  vmax);
	G_message(_("Time step used \t= %.2f s\n"), deltaw);
    }
    /*    if (wdepth) deltap = 0.1; 
     *    deltap for sediment is ar. average deltap and deltaw */
    /*    if (wdepth) deltap = (deltaw+deltap)/2.; 
     *    deltap for sediment is ar. average deltap and deltaw */


    /*! For each cell (k,l) compute the length s=(v1,v2) of the path 
     *  that the particle will travel per one time step
     *  \f$ s(k,l)=v(k,l)*dt \f$, [m]=[m/s]*[s]
     *  give warning if there is a cell that will lead to path longer than 2 cells 
     *
     *  if running erosion, compute sediment transport capacity for each cell si(k,l)
     *  \f$
     * T({\bf r})=K_t({\bf r}) \bigl[\tau({\bf r})\bigr]^p
     * =K_t({\bf r}) \bigl[\rho_w\, g h({\bf r}) \sin \beta ({\bf r}) \bigr]^p
     * \f$
     * [kg/ms]=...
     */
    for (k = 0; k < my; k++) {
	for (l = 0; l < mx; l++) {
	    if (zz[k][l] != UNDEF) {
		v1[k][l] *= deltap;
		v2[k][l] *= deltap;
		/*if(v1[k][l]*v1[k][l]+v2[k][l]*v2[k][l] > cellsize, warning, napocitaj
		 *ak viac ako 10%a*/
		/* THIS IS CORRECT SOLUTION currently commented out */
		if (inf)
		    inf[k][l] *= timesec;
		if (wdepth)
		    gama[k][l] = 0.;
		if (et) {
		    if (sigma[k][l] == 0. || slope[k][l] == 0.)
			si[k][l] = 0.;
		    else
			/* temp for transp. cap. erod */
			si[k][l] = si[k][l] / (slope[k][l] * sigma[k][l]);
		}
	    }			/* DEFined area */
	}
    }

    /*! compute transport capacity limted erosion/deposition et 
     *   as a divergence of sediment transport capacity
     *   \f$
     D_T({\bf r})= \nabla\cdot {\bf T}({\bf r})
     *   \f$
     */
    if (et) {
	erod(si);		/* compute divergence of t.capc */
	if (output_et() != 1)
	    G_fatal_error(_("Unable to write et file"));
    }

    /*! compute the inversion operator and store it in sigma - note that after this
     *   sigma does not store the first order reaction coefficient but the operator
     *   WRITE the equation here
     */
    if (wdepth) {
	for (k = 0; k < my; k++) {
	    for (l = 0; l < mx; l++) {
		if (zz[k][l] != UNDEF) {
		    /* get back from temp */
		    if (et)
			si[k][l] = si[k][l] * slope[k][l] * sigma[k][l];
		    if (sigma[k][l] != 0.)
			/* rate of weight loss - w=w*sigma ,
			 * vaha prechadzky po n-krokoch je sigma^n */
			/* not clear what's here :-\ */
			sigma[k][l] =
			    exp(-sigma[k][l] * deltap * slope[k][l]);
		    /* if(sigma[k][l]<0.5) warning, napocitaj, 
		     * ak vacsie ako 50% skonci, zmensi deltap)*/
		}
	    }			/*DEFined area */
	}
    }
    return 1;
}


double amax1(double arg1, double arg2)
{
    double res;

    if (arg1 >= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }

    return res;
}


double amin1(double arg1, double arg2)
{
    double res;

    if (arg1 <= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }

    return res;
}


int min(int arg1, int arg2)
{
    int res;

    if (arg1 <= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }

    return res;
}


int max(int arg1, int arg2)
{
    int res;

    if (arg1 >= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }

    return res;
}
