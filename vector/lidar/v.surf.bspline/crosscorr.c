/***********************************************************************
 *
 * MODULE:       v.surf.bspline
 *
 * AUTHOR(S):    Roberto Antolin
 *
 * PURPOSE:      Spline Interpolation and cross correlation
 *
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano -
 *			     Polo Regionale di Como
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************************/

/*INCLUDES*/
#include <stdlib.h> 
#include <string.h> 

#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/config.h>

#include <grass/PolimiFunct.h>
#include "bspline.h"


#define NDATA_MAX 100
#define PARAM_LAMBDA 6
#define PARAM_SPLINE 0

#define SWAP(a, b) {double t=a; a=b; b=t;}


/*-------------------------------------------------------------------------------------------*/
    int 
cross_correlation (struct Map_info* Map, double passWE, double passNS)
    /*
       Map: Map in which cross crorrelation will take values
       passWE: spline step in West-East direction
       passNS: spline step in North-South direction

       RETURN:
       TRUE on success
       FALSE on failure
     */

{
    int bilin=TRUE;					/*booleans*/
    int nsplx, nsply, nparam_spl, ndata;
    double *mean, *rms, *stdev, rms_min, stdev_min; 

    double lambda[PARAM_LAMBDA] = {0.0001, 0.001, 0.01, 0.1, 1.0, 10.0};  	/* Fixed values (by the moment) */

    double *TN, *Q, *parVect;			/* Interpolation and least-square vectors*/
    double **N, **obsVect;			/* Interpolation and least-square matrix*/

    struct Point *observ;
    struct Stats stat_vect;
    /*struct line_pnts *points;*/
    /*struct line_cats *Cats;*/
    struct Cell_head region;

    G_get_window (&region);

    extern int bspline_field;
    extern char *bspline_column;
    dbCatValArray cvarr;

    G_debug (5, "CrossCorrelation: Some tests using different lambda_i values will be done");

    ndata = Vect_get_num_lines (Map);

    if (ndata > NDATA_MAX) 
	G_warning (_("CrossCorrelation: %d are too many points. "
		    "The cross validation would take too much time."), ndata);

    /*points = Vect_new_line_struct ();*/
    /*Cats = Vect_new_cats_struct ();*/

    /* Current region is read and points recorded into observ */
    observ = P_Read_Vector_Region_Map (Map, &region, &ndata, 1024, 1);
    G_debug (5, "CrossCorrelation: %d points read in region. ", ndata);
    fprintf (stdout, _("CrossCorrelation: %d points read in region.\n"), ndata);
    
    if (ndata > 50) 
	G_warning (_("CrossCorrelation: Maybe, it takes too long. "
		    "It will depend on how many points you are considering."));
    else 
	G_debug (5, "CrossCorrelation: It shouldn't take too long.");

    if (ndata > 0) {			/* If at least one point is in the region */	
	int i, j, lbd;			/* lbd: lambda index */
	int BW, lbd_min	;		/* lbd_min: index where minimun is found */
	double mean_reg, *obs_mean;

	int nrec, ctype = 0;
	struct field_info *Fi;
	dbDriver *driver_cats;

	mean = G_alloc_vector (PARAM_LAMBDA);	/* Alloc as much mean, rms and stdev values as the total*/
	rms = G_alloc_vector (PARAM_LAMBDA);	/* number of parameter used used for cross validation*/
	stdev = G_alloc_vector (PARAM_LAMBDA);	

	/* Working with attributes*/
	if (bspline_field > 0) {
	    db_CatValArray_init ( &cvarr );

	    Fi = Vect_get_field (Map, bspline_field);
	    if ( Fi == NULL )
		G_fatal_error (_("CrossCorrelation: Cannot read field info"));	

	    driver_cats = db_start_driver_open_database ( Fi->driver, Fi->database );
	    G_debug (1, _("CrossCorrelation: driver=%s db=%s"), Fi->driver, Fi->database);

	    if ( driver_cats == NULL )
		G_fatal_error(_("CrossCorrelation: Cannot open database %s by driver %s"), Fi->database, Fi->driver);

	    nrec = db_select_CatValArray ( driver_cats, Fi->table, Fi->key, bspline_column, NULL, &cvarr );
	    G_debug (3, "nrec = %d", nrec );

	    ctype = cvarr.ctype;
	    if ( ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE )
		G_fatal_error ( _("CrossCorrelation: Column type not supported") );

	    if ( nrec < 0 )
		G_fatal_error (_("CrossCorrelation: Cannot select data from table"));

	    G_message ( _("CrossCorrelation: %d records selected from table"), nrec);

	    db_close_database_shutdown_driver (driver_cats);
	}

	/* Setting number of splines as a function of WE and SN spline steps */
	nsplx = ceil ((region.east - region.west)/passWE);
	nsply = ceil ((region.north - region.south)/passNS);
	nparam_spl = nsplx * nsply; 	/* Total number of splines */

	if (nparam_spl > 22900)
	    G_fatal_error (_("CrossCorrelation: Too many splines (%d x %d). "
			"Consider changing spline steps \"sie=\" \"sin=\"."), nsplx, nsply);

	BW = P_get_BandWidth (bilin, nsply); 	/**/

	/*Least Squares system*/
	N = G_alloc_matrix (nparam_spl, BW);		/* Normal matrix */
	TN = G_alloc_vector (nparam_spl);		/* vector */
	parVect = G_alloc_vector (nparam_spl);		/* Parameters vector */
	obsVect = G_alloc_matrix (ndata, 3);	 	/* Observation vector */
	Q = G_alloc_vector (ndata);			/* "a priori" var-cov matrix */

	obs_mean = G_alloc_vector (ndata);
 	stat_vect = alloc_Stats (ndata);

	for (lbd=0; lbd < PARAM_LAMBDA; lbd++) {	/* For each lambda value */

	    fprintf (stdout, _("CrossCorrelation: Begining cross validation with "
			"lambda_i=%.4f ...\n"), lambda[lbd]);

	    /*
	       How cross correlation algorithm is done:
	       For each cicle, only the first ndata-1 "observ" elements are considered for the 
	       interpolation. Within every interpolation mean is calculated to lowering border 
	       errors. The point let out will be used for an estimation. The error between the 
	       estimation and the observation is recorded for further statistics.
	       At the end of the cicle, the last point, that is, the ndata-1 index, and the point 
	       with j index are swapped.
	     */
	    for (j=0; j<ndata; j++) {			/* Cross Correlation will use all ndata points*/
		double out_x, out_y, out_z;		/* This point is let out */	
		    
		for (i=0; i<ndata; i++) {		/* Each time, only the first ndata-1 points */
		    double dval;			/* are considered in the interpolation*/

		    /* Setting obsVect vector & Q matrix */
		    Q[i] = 1;					/* Q=I */
		    obsVect[i][0] = observ[i].coordX;
		    obsVect[i][1] = observ[i].coordY;

		    if (bspline_field > 0){
			int cat, ival, ret;

			/*type = Vect_read_line (Map, points, Cats, observ[i].lineID);*/
			/*if ( !(type & GV_POINTS ) ) continue;*/

			/*Vect_cat_get ( Cats, bspline_field, &cat );*/
			cat = observ[i].cat;

			if ( cat < 0 ) continue;

			if ( ctype == DB_C_TYPE_INT ) {
			    ret = db_CatValArray_get_value_int ( &cvarr, cat, &ival );
			    obsVect[i][2] = ival;
			    obs_mean [i]= ival;
			} else {		 /* DB_C_TYPE_DOUBLE */
			    ret = db_CatValArray_get_value_double ( &cvarr, cat, &dval );
			    obsVect[i][2] = dval;
			    obs_mean [i] = dval;
			}
			if ( ret != DB_OK ) {
			    G_warning (_("CrossCorrelation: No record for point (cat = %d)"), cat);
			    continue;
			}
		    }
		    else { 
			obsVect[i][2] = observ[i].coordZ; 
			obs_mean [i] = observ[i].coordZ;
		    }
		} /* i index */

		/* Mean calculation for every point less the last one */
		mean_reg = calc_mean (obs_mean, ndata-1);

		for (i=0; i<ndata; i++)
		    obsVect[i][2] -= mean_reg;

		/* This is let out */
		out_x = observ[ndata-1].coordX;
		out_y = observ[ndata-1].coordY;
		out_z = obsVect[ndata-1][2];

		if (bilin) {		/* Bilinear interpolation */
		    normalDefBilin (N, TN, Q, obsVect, passWE, passNS, nsplx, nsply, region.west, 
			    region.south, ndata-1, nparam_spl, BW);
		    nCorrectGrad (N, lambda[lbd], nsplx, nsply, passWE, passNS);
		} 
		else {			/* Bicubic interpolation */	
		    normalDefBicubic (N, TN, Q, obsVect, passWE, passNS, nsplx, nsply, region.west, 
			    region.south, ndata-1, nparam_spl, BW);
		    nCorrectGrad (N, lambda[lbd], nsplx, nsply, passWE, passNS);
		}

		/* 
		   if (bilin) interpolation (&interp, P_BILINEAR);
		   else interpolation (&interp, P_BICUBIC);
		 */
		tcholSolve (N, TN, parVect, nparam_spl, BW);

		/* Estimation of j-point */
		if (bilin) 
		    stat_vect.estima[j] = dataInterpolateBilin (out_x, out_y, passWE, passNS, 
			    nsplx, nsply, region.west, region.south, parVect);	

		else 
		    stat_vect.estima[j] = dataInterpolateBilin (out_x, out_y, passWE, passNS, 
			    nsplx, nsply, region.west, region.south, parVect);	

		/*Difference between estimated and observated i-point*/
		stat_vect.error[j] = out_z - stat_vect.estima[j];
		G_debug (1, "CrossCorrelation: stat_vect.error[%d]  =  %lf", j, stat_vect.error[j]);

		observ = swap (observ, j, ndata-1);	/* Once the last value is let out, it is swap with j-value */
	    } 

	    mean[lbd] = calc_mean (stat_vect.error, stat_vect.n_points);
	    rms[lbd] = calc_root_mean_square (stat_vect.error, stat_vect.n_points);
	    stdev[lbd] = calc_standard_deviation (stat_vect.error, stat_vect.n_points);

	    fprintf (stdout, _("CrossCorrelation: Mean = %.5lf\n"), mean[lbd]);
	    fprintf (stdout, _("CrossCorrelation: Root Means Square (RMS) = %.5lf\n"), rms[lbd]);
	    fprintf (stdout, "\n---------------------o-o-------------------\n\n");
	}	/* ENDFOR each lambda value */

	G_free_matrix (N);
	G_free_vector (TN);
	G_free_vector (Q);
	G_free_matrix (obsVect);
	G_free_vector (parVect);
#ifdef nodef
	/*TODO: if the minimum lambda is wanted, the function declaration must be changed*/
	/*At this moment, consider rms only*/
	rms_min = find_minimum (rms, &lbd_min);
	stdev_min = find_minimum (stdev, &lbd_min);

	/* Writing some output*/
	fprintf (stdout, _("CrossCorrelation: Different number of splines and lambda_i values have " \
		    "been taken for the cross correlation\n"));
	fprintf (stdout, _("CrossCorrelation: The minimum value for the test (rms=%lf) was obtained with:\n"), rms_min);
	fprintf (stdout, _("CrossCorrelation: lambda_i = %.3f\n"), lambda[lbd_min]);

	*lambda_min = lambda[lbd_min];
#endif

	fprintf (stdout, _("Now, the results into a table:\n"));
	fprintf (stdout, _(" lambda    | mean        | rms         |\n"));
	for  (lbd=0; lbd < PARAM_LAMBDA; lbd++) {
	    fprintf (stdout, _(" %-10.5f| %-12.4f| %-12.4f|\n"),lambda[lbd], mean[lbd], rms[lbd]);
	}
	fprintf (stdout, _("\nResults are over.\n"));

	G_free_vector (mean);
	G_free_vector (rms);
     }	/* ENDIF (ndata > 0) */
     else
	G_warning (_("CrossCorrelation: No point lies into the current region"));

    G_free (observ);
    return TRUE;
}

#ifdef nodef
void interpolation (struct ParamInterp *interp, boolean bilin) {
    if (bilin == P_BILINEAR) {	/* Bilinear interpolation */
	normalDefBilin (interp->N, interp->TN, interp->Q, interp->obsVect, interp->passoE, 
		interp->passoN, interp->nsplx, interp->nsply, interp->region.west, 
		interp->region.south, interp->ndata, interp->nparam_spl, interp->BW);

	nCorrectGrad (interp->N, interp->lambda[lbd], interp->nsplx, interp->nsply, 
		interp->passoE, interp->passoN);
    } 
    else {			/* Bicubic interpolation */	
	normalDefBicubic (interp->N, interp->TN, interp->Q, interp->obsVect, interp->passoE, 
		interp->passoN, interp->nsplx, interp->nsply, interp->region.west, 
		interp->region.south, interp->ndata, interp->nparam_spl, interp->BW);

	nCorrectGrad (interp->N, interp->lambda[lbd], interp->nsplx, interp->nsply, 
		interp->passoE, interp->passoN);
    }
    return TRUE; 
}
#endif

double
calc_mean (double *values, int nvalues) {
    int i;
    double sum=.0;

    if (nvalues == 0) return .0;
    for (i=0; i < nvalues; i++)
	sum += values[i];
    return sum/nvalues;
}


double 
calc_root_mean_square (double *values, int nvalues) {
    int i;
    double rms, sum=.0;

    if (nvalues == 0) return .0;

    for (i=0; i < nvalues; i++)
	sum += pow (values[i], 2)/nvalues;

    rms = sqrt(sum);
    return  rms;

}

double 
calc_standard_deviation (double *values, int nvalues) {
    double mean, rms, stdev;

    if ( nvalues == 0) return .0;

    rms = calc_root_mean_square (values, nvalues);
    mean = calc_mean (values, nvalues);

    stdev = sqrt (pow(rms, 2) - pow(mean, 2));
    return stdev;
}

struct  
Stats alloc_Stats (int n) {
    double *err, *stm; 
    struct Stats stat;

    stat.n_points = n;
    err = (double *) G_calloc (n, sizeof(double));
    stm = (double *) G_calloc (n, sizeof(double));

    stat.error = err;
    stat.estima = stm;

    return stat;
}

double
find_minimum (double *values, int *l_min) {
    int l; 
    double min;

    min =  values[0];

    for (l=0; l<PARAM_LAMBDA; l++) {
	if (min > values[l]) {
	    min = values[l];
	    *l_min = l;
	}
    }
    return min;
}

struct Point*
swap (struct Point *point, int a, int b) {
    
    SWAP (point[a].coordX, point[b].coordX);  /* Once the last value is let out, it is swap with j-value */
    SWAP (point[a].coordY, point[b].coordY);
    SWAP (point[a].coordZ, point[b].coordZ);
    SWAP (point[a].cat, point[b].cat);

    return point;
}

