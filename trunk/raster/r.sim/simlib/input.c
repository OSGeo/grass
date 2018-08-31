/* input.c (simlib), 20.nov.2002, JH */

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
static float ** read_float_raster_map(int rows, int cols, char *name, float unitconv);
static double ** read_double_raster_map(int rows, int cols, char *name, double unitconv);
static float ** create_float_matrix(int rows, int cols, float fill_value);
static double ** create_double_matrix(int rows, int cols, double fill_value);
static void copy_matrix_undef_double_to_float_values(int rows, int cols, double **source, float **target);
static void  copy_matrix_undef_float_values(int rows, int cols, float **source, float **target);

/*!
 * \brief Initialize WaterParams structure.
 */
void WaterParams_init(struct WaterParams *wp)
{
    /* this is little bit lengthy and perhaps error-prone
     * but it simplifies initialization since then there is no
     * difference in between initialization in water and sediment
     * for the variables which are not used and would have been
     * initialized if they were just global variables */
    wp->ymin = 0;
    wp->xmax = 0;
    wp->ymax = 0;
    wp->mayy = 0;
    wp->miyy = 0;
    wp->maxx = 0;
    wp->mixx = 0;
    wp->mx = 0;
    wp->my = 0;
    wp->mx2 = 0;
    wp->my2 = 0;

    wp->bxmi = 0;
    wp->bymi = 0;
    wp->bxma = 0;
    wp->byma = 0;
    wp->bresx = 0;
    wp->bresy = 0;
    wp->maxwab = 0;
    wp->step = 0;
    wp->conv = 0;

    wp->frac = 0;
    wp->bxmi = 0;
    wp->bymi = 0;

    wp->hbeta = 0;
    wp->hhmax = 0;
    wp->sisum = 0;
    wp->vmean = 0;
    wp->infsum = 0;
    wp->infmean = 0;
    wp->maxw = 0;
    wp->maxwa = 0;
    wp->nwalk = 0;
    wp->rwalk = 0;
    wp->bresx = 0;
    wp->bresy = 0;
    wp->xrand = 0;
    wp->yrand = 0;
    wp->stepx = 0;
    wp->stepy = 0;
    wp->xp0 = 0;
    wp->yp0 = 0;
    wp->chmean = 0;
    wp->si0 = 0;
    wp->deltap = 0;
    wp->deldif = 0;
    wp->cch = 0;
    wp->hhc = 0;
    wp->halpha = 0;
    wp->eps = 0;
    wp->maxwab = 0;
    wp->nstack = 0; 
    wp->iterout = 0;
    wp->mx2o = 0;
    wp->my2o = 0;
    wp->miter = 0;
    wp->nwalka = 0;
    wp->timec = 0;
    wp->ts = 0;
    wp->timesec = 0;

    wp->rain_val = 0;
    wp->manin_val = 0;
    wp->infil_val = 0;

    wp->elevin = NULL;
    wp->dxin = NULL;
    wp->dyin = NULL;
    wp->rain = NULL;
    wp->infil = NULL;
    wp->traps = NULL;
    wp->manin = NULL;
    wp->depth = NULL;
    wp->disch = NULL;
    wp->err = NULL;
    wp->outwalk = NULL;
    wp->observation = NULL;
    wp->logfile = NULL;
    wp->mapset = NULL;
    wp->mscale = NULL;
    wp->tserie = NULL;

    wp->wdepth = NULL;
    wp->detin = NULL;
    wp->tranin = NULL;
    wp->tauin = NULL;
    wp->tc = NULL;
    wp->et = NULL;
    wp->conc = NULL;
    wp->flux = NULL;
    wp->erdep = NULL;

    wp->rainval = NULL;
    wp->maninval = NULL;
    wp->infilval = NULL;
}

/*!
 * \brief Initialize global variables in the library.
 */
void init_library_globals(struct WaterParams *wp)
{
    /* this is little bit lengthy and perhaps error-prone
     * but it separates library from its interface */
    ymin = wp->ymin;
    xmax = wp->xmax;
    ymax = wp->ymax;
    mayy = wp->mayy;
    miyy = wp->miyy;
    maxx = wp->maxx;
    mixx = wp->mixx;
    mx = wp->mx;
    my = wp->my;
    mx2 = wp->mx2;
    my2 = wp->my2;

    bxmi = wp->bxmi;
    bymi = wp->bymi;
    bxma = wp->bxma;
    byma = wp->byma;
    bresx = wp->bresx;
    bresy = wp->bresy;
    maxwab = wp->maxwab;
    step = wp->step;
    conv = wp->conv;

    frac = wp->frac;
    bxmi = wp->bxmi;
    bymi = wp->bymi;

    hbeta = wp->hbeta;
    hhmax = wp->hhmax;
    sisum = wp->sisum;
    vmean = wp->vmean;
    infsum = wp->infsum;
    infmean = wp->infmean;
    maxw = wp->maxw;
    maxwa = wp->maxwa;
    nwalk = wp->nwalk;
    rwalk = wp->rwalk;
    bresx = wp->bresx;
    bresy = wp->bresy;
    xrand = wp->xrand;
    yrand = wp->yrand;
    stepx = wp->stepx;
    stepy = wp->stepy;
    xp0 = wp->xp0;
    yp0 = wp->yp0;
    chmean = wp->chmean;
    si0 = wp->si0;
    deltap = wp->deltap;
    deldif = wp->deldif;
    cch = wp->cch;
    hhc = wp->hhc;
    halpha = wp->halpha;
    eps = wp->eps;
    maxwab = wp->maxwab;
    nstack = wp->nstack; 
    iterout = wp->iterout;
    mx2o = wp->mx2o;
    my2o = wp->my2o;
    miter = wp->miter;
    nwalka = wp->nwalka;
    timec = wp->timec;
    ts = wp->ts;
    timesec = wp->timesec;

    rain_val = wp->rain_val;
    manin_val = wp->manin_val;
    infil_val = wp->infil_val;

    elevin = wp->elevin;
    dxin = wp->dxin;
    dyin = wp->dyin;
    rain = wp->rain;
    infil = wp->infil;
    traps = wp->traps;
    manin = wp->manin;
    depth = wp->depth;
    disch = wp->disch;
    err = wp->err;
    outwalk = wp->outwalk;
    observation = wp->observation;
    logfile = wp->logfile;
    mapset = wp->mapset;
    mscale = wp->mscale;
    tserie = wp->tserie;

    wdepth = wp->wdepth;
    detin = wp->detin;
    tranin = wp->tranin;
    tauin = wp->tauin;
    tc = wp->tc;
    et = wp->et;
    conc = wp->conc;
    flux = wp->flux;
    erdep = wp->erdep;

    rainval = wp->rainval;
    maninval = wp->maninval;
    infilval = wp->infilval;
}

/* we do the allocation inside because we anyway need to set the variables */

void alloc_grids_water()
{
    /* memory allocation for output grids */
    G_debug(1, "beginning memory allocation for output grids");

    gama = G_alloc_matrix(my, mx);
    if (err != NULL)
        gammas = G_alloc_matrix(my, mx);
    dif = G_alloc_fmatrix(my, mx);
}

void alloc_grids_sediment()
{
    /* mandatory for si,sigma */

    si = G_alloc_matrix(my, mx);
    sigma = G_alloc_matrix(my, mx);

    /* memory allocation for output grids */

    dif = G_alloc_fmatrix(my, mx);
    if (erdep != NULL || et != NULL)
        er = G_alloc_fmatrix(my, mx);
}

void init_grids_sediment()
{
    /* this should be fulfilled for sediment but not water */
    if (et != NULL)
        erod(si);
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
int input_data(void)
{
    int rows = my, cols = mx; /* my and mx are global variables */
    double unitconv = 0.000000278;	/* mm/hr to m/s */
    int if_rain = 0;

    G_debug(1, "Running MAR 2011 version, started modifications on 20080211");
    G_debug(1, "Reading input data");
    
    /* Elevation and gradients are mandatory */
    zz = read_float_raster_map(rows, cols, elevin, 1.0);
    v1 = read_double_raster_map(rows, cols, dxin, 1.0);
    v2 = read_double_raster_map(rows, cols, dyin, 1.0);

    /* Update elevation map */
    copy_matrix_undef_double_to_float_values(rows, cols, v1, zz);
    copy_matrix_undef_double_to_float_values(rows, cols, v2, zz);

    /* Manning surface roughnes: read map or use a single value */
    if(manin != NULL) {
    	cchez = read_float_raster_map(rows, cols, manin, 1.0);
     } else if(manin_val >= 0.0) { /* If no value set its set to -999.99 */
	cchez = create_float_matrix(rows, cols, manin_val);
    }else{
        G_fatal_error(_("Raster map <%s> not found, and manin_val undefined, choose one to be allowed to process"), manin);
    }
       
    /* Rain: read rain map or use a single value for all cells */
    if (rain != NULL) {
	si = read_double_raster_map(rows, cols, rain, unitconv);
	if_rain = 1;
    } else if(rain_val >= 0.0) { /* If no value set its set to -999.99 */
	si = create_double_matrix(rows, cols, rain_val * unitconv);
	if_rain = 1;
    } else{
	si = create_double_matrix(rows, cols, (double)UNDEF);
	if_rain = 0;
    }

    /* Update elevation map */
    copy_matrix_undef_double_to_float_values(rows, cols, si, zz);

    /* Load infiltration and traps if rain is present */
    if(if_rain == 1) {
	/* Infiltration: read map or use a single value */
        if (infil != NULL) {
            inf = read_double_raster_map(rows, cols, infil, unitconv);
        } else if(infil_val >= 0.0) { /* If no value set its set to -999.99 */
	    inf = create_double_matrix(rows, cols, infil_val * unitconv);
        } else{
	    inf = create_double_matrix(rows, cols, (double)UNDEF);
        }

   	/* Traps */
        if (traps != NULL)
            trap = read_float_raster_map(rows, cols, traps, 1.0);
	else
	    trap = create_float_matrix(rows, cols, (double)UNDEF);
    }

    if (detin != NULL) {
    	 dc = read_float_raster_map(rows, cols, detin, 1.0);
         copy_matrix_undef_float_values(rows, cols, dc, zz);
    }

    if (tranin != NULL) {
    	 ct = read_float_raster_map(rows, cols, tranin, 1.0);
         copy_matrix_undef_float_values(rows, cols, ct, zz);
    }

    if (tauin != NULL) {
    	 tau = read_float_raster_map(rows, cols, tauin, 1.0);
         copy_matrix_undef_float_values(rows, cols, tau, zz);
    }

    if (wdepth != NULL) {
        gama = read_double_raster_map(rows, cols, wdepth, 1.0);
        copy_matrix_undef_double_to_float_values(rows, cols, gama, zz);
    }
    
    /* Array for gradient checking */
    slope = create_double_matrix(rows, cols, 0.0);
    
    /* Create the observation points and open the logfile */
    create_observation_points();

  return 1;
}

/* ************************************************************************* */

/* data preparations, sigma, shear, etc. */
int grad_check(void)
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

    sisum = 0.;
    infsum = 0.;
    cmul2 = rhow * gacc;

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

    G_message(n_("Number of iterations \t= %d cell\n", 
        "Number of iterations \t= %d cells\n", miter), miter);
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

			/*!!!!! not clear what's here :-\ !!!!!*/

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

/* ************************************************************************* */

void copy_matrix_undef_double_to_float_values(int rows, int cols, double **source, float **target)
{
    int col = 0, row = 0;

    for(row = 0; row < rows; row++) {
        for(col = 0; col < cols; col++) {
	    if(source[row][col] == UNDEF)
		target[row][col] = UNDEF;
	}
    }
}

/* ************************************************************************* */

void copy_matrix_undef_float_values(int rows, int cols, float **source, float **target)
{
    int col = 0, row = 0;

    for(row = 0; row < rows; row++) {
        for(col = 0; col < cols; col++) {
	    if(source[row][col] == UNDEF)
		target[row][col] = UNDEF;
	}
    }
}

/* ************************************************************************* */

float ** create_float_matrix(int rows, int cols, float fill_value)
{
    int col = 0, row = 0;
    float **matrix = NULL;

    G_verbose_message("Creating float matrix with value %g", fill_value);

    /* Allocate the float marix */
    matrix = G_alloc_fmatrix(rows, cols);

    for(row = 0; row < rows; row++) {
        for(col = 0; col < cols; col++) {
	    matrix[row][col] = fill_value;
	}
    }

    return matrix;
}

/* ************************************************************************* */

double ** create_double_matrix(int rows, int cols, double fill_value)
{
    int col = 0, row = 0;
    double **matrix = NULL;

    G_verbose_message("Creating double matrix with value %g", fill_value);

    /* Allocate the float marix */
    matrix = G_alloc_matrix(rows, cols);

    for(row = 0; row < rows; row++) {
        for(col = 0; col < cols; col++) {
	    matrix[row][col] = fill_value;
	}
    }

    return matrix;
}

/* ************************************************************************* */

float ** read_float_raster_map(int rows, int cols, char *name, float unitconv)
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

    for(row = 0; row < rows; row++) {
	Rast_get_f_row(fd, row_buff, row);

        for(col = 0; col < cols; col++) {
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
    if(row_buff)
    	G_free(row_buff);

    Rast_close(fd);

    return matrix;
}

/* ************************************************************************* */

double ** read_double_raster_map(int rows, int cols, char *name, double unitconv)
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

    for(row = 0; row < rows; row++) {
	Rast_get_d_row(fd, row_buff, row);

        for(col = 0; col < cols; col++) {
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
    if(row_buff)
    	G_free(row_buff);

    Rast_close(fd);

    return matrix;
}
