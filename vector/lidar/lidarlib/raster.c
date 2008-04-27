#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/config.h>

#include "PolimiFunct.h"

/*------------------------------------------------------------------------------------------------*/
void 
P_Sparse_Points (struct Map_info *Out, struct Cell_head *Elaboration, BOUND_BOX General, BOUND_BOX Overlap, double **obs, \
			double *param, int *line_num, double pe, double pn, double overlap, int nsplx, int nsply, int num_points, \
			int bilin, struct line_cats *categories, dbDriver *driver, double mean, char *tab_name) 
{
    int i;
    char buf[1024];
    
    double interpolation, csi, eta, weight;
    struct line_pnts *point;
    dbString sql;
    
    
/*CREARE LA TABELLA*/
    
    point = Vect_new_line_struct ();
    
    for (i=0; i<num_points; i++){
	
	if ( Vect_point_in_box (obs[i][0], obs[i][1], mean, &General) ) {	/*Here mean is just for asking if obs point is in box */
	
	   if (bilin)
		interpolation = dataInterpolateBilin (obs[i][0], obs[i][1], pe, pn, nsplx, nsply, Elaboration->west, \
					Elaboration->south, param); 
	   else
		interpolation = dataInterpolateBicubic (obs[i][0], obs[i][1], pe, pn, nsplx, nsply, Elaboration->west, \
				 	Elaboration->south, param);

	interpolation += mean;
	Vect_copy_xyz_to_pnts (point, &obs[i][0], &obs[i][1], &interpolation, 1);

	    if (Vect_point_in_box (obs[i][0], obs[i][1], interpolation, &Overlap)) {  /*(5)*/
		Vect_write_line (Out, GV_POINT, point, categories);	
	    
	    } else {
		    
	        db_init_string (&sql);
    
		sprintf (buf, "INSERT INTO %s (ID, X, Y, Interp)", tab_name);
		db_append_string (&sql, buf);
		
		sprintf (buf, " VALUES (");
		db_append_string (&sql, buf);
		sprintf (buf, "%d, %f, %f, ", line_num[i], obs[i][0], obs[i][1]);
		db_append_string (&sql, buf);

		if ((point->x[0] > Overlap.E)) {

		    if ((*point->y > Overlap.N)) {		/*(3)*/
			csi = (*point->x - Overlap.E)/overlap;
			eta = (*point->y - Overlap.N)/overlap;
			weight = (1-csi)*(1-eta);
			*point->z = weight*interpolation;
			
			sprintf (buf, "%lf", *point->z);
			db_append_string (&sql, buf);
			sprintf (buf, ")");
			db_append_string (&sql, buf);

			if (db_execute_immediate (driver, &sql) != DB_OK) 
			    G_fatal_error (_("Unable to create table: %s"), buf);

		    } else if ((*point->y < Overlap.S)) {	/*(1)*/
		        csi = (*point->x - Overlap.E)/overlap;
			eta = (Overlap.S - *point->y)/overlap;
			weight = (1-csi)*eta;
			*point->z = weight*interpolation;

			sprintf (buf, "%lf", *point->z);
			db_append_string (&sql, buf);
			sprintf (buf, ")");
			db_append_string (&sql, buf);

			if (db_execute_immediate (driver, &sql) != DB_OK) 
			    G_fatal_error (_("Unable to create table: %s"), buf);

		    } else {					/*(1)*/
		        weight = (*point->x - Overlap.E)/overlap;
			*point->z = (1-weight)*interpolation;

			sprintf (buf, "%lf", *point->z);
			db_append_string (&sql, buf);
			sprintf (buf, ")");
			db_append_string (&sql, buf);

			if (db_execute_immediate (driver, &sql) != DB_OK) 
			    G_fatal_error (_("Unable to create table: %s"), buf);
		    }

		} else if ((point->x[0] < Overlap.W)) {
		    if ((*point->y > Overlap.N)) {		/*(4)*/
		        csi = (Overlap.W - *point->x)/overlap;
			eta = (*point->y - Overlap.N)/overlap;
			weight = (1-eta)*csi;
			*point->z = weight*interpolation;

			sprintf (buf, "%lf", *point->z);
			db_append_string (&sql, buf);
			sprintf (buf, ")");
			db_append_string (&sql, buf);

			if (db_execute_immediate (driver, &sql) != DB_OK) 
			    G_fatal_error (_("Unable to create table: %s"), buf);
			
		    } else if ((*point->y < Overlap.S)) {	/*(2)*/
		            csi = (*point->x - General.W)/overlap;
			    eta = (Overlap.S - *point->y)/overlap;
			    weight = csi*eta;
			    *point->z = weight*interpolation;

			    sprintf (buf, "%lf", *point->z);
			    db_append_string (&sql, buf);
			    sprintf (buf, ")");
			    db_append_string (&sql, buf);

			if (db_execute_immediate (driver, &sql) != DB_OK) 
			    G_fatal_error (_("Unable to create table: %s"), buf);

		    } else {					/*(2)*/
		            weight = (Overlap.W - *point->x)/overlap;
			    *point->z = (1-weight)*interpolation;

			    sprintf (buf, "%lf", *point->z);
			    db_append_string (&sql, buf);
			    sprintf (buf, ")");
			    db_append_string (&sql, buf);

			if (db_execute_immediate (driver, &sql) != DB_OK) 
			    G_fatal_error (_("Unable to create table: %s"), buf);
	    	    }

		} else {
		    if ((point->y[0] > Overlap.N)) {		/*(3)*/
		    	weight = (*point->y - Overlap.N)/overlap;
		    	*point->z = (1-weight)*interpolation;

		    	sprintf (buf, "%lf", *point->z);
		    	db_append_string (&sql, buf);
		    	sprintf (buf, ")");
		    	db_append_string (&sql, buf);

			if (db_execute_immediate (driver, &sql) != DB_OK) 
			    G_fatal_error (_("Unable to create table: %s"), buf);
		    } else {						/*(1)*/
		    	weight = (Overlap.S - *point->y)/overlap;
		    	*point->z = (1-weight)*interpolation;

		    	sprintf (buf, "%lf", *point->z);
		    	db_append_string (&sql, buf);
		    	sprintf (buf, ")");
		    	db_append_string (&sql, buf);

			if (db_execute_immediate (driver, &sql) != DB_OK) 
			    G_fatal_error (_("Unable to create table: %s"), buf);
		    }
		} 
	    }
	} /*IF*/
    }	/*FOR*/
    return;
}


/*------------------------------------------------------------------------------------------------*/
double ** 
P_Regular_Points (struct Cell_head *Elaboration, BOUND_BOX General, BOUND_BOX Overlap, double **matrix, double *param,\
			double passoN, double passoE, double overlap, double mean, int nsplx, int nsply, int nrows, int ncols, int bilin) {

    int col, row;
    double X, Y, interpolation, weight, csi, eta;
    struct Cell_head Original;

    G_get_window (&Original);
    for (row = 0; row < nrows; row++) {
      for (col = 0; col < ncols; col++)  {
	X = G_col_to_easting((double)(col) + 0.5, &Original);
	Y = G_row_to_northing((double)(row) + 0.5, &Original);
	
	if (Vect_point_in_box (X, Y, mean, &General)) {	/* Here, mean is just for asking if obs point is in box */
	
	    if (bilin)
		interpolation = dataInterpolateBilin (X, Y, passoE, passoN, nsplx, nsply, Elaboration->west, \
	     			Elaboration->south, param); 
	    else
		interpolation = dataInterpolateBicubic (X, Y, passoE, passoN, nsplx, nsply, Elaboration->west, \
	     			Elaboration->south, param);

	    interpolation += mean;

	    if (Vect_point_in_box (X, Y, interpolation, &Overlap)) {  /* (5) */
	    	matrix[row][col] = interpolation;
	
	    } else {
		
		if ((X > Overlap.E)) {
		
		    if ((Y > Overlap.N)) {			/* (3) */
			csi = (X - Overlap.E)/overlap;
			eta = (Y - Overlap.N)/overlap;
			weight = (1-csi)*(1-eta);
			interpolation *= weight;
			matrix[row][col] += interpolation;

		    } else if ((Y < Overlap.S)) {		/* (1) */
		        csi = (X - Overlap.E)/overlap;
			eta = (Y - General.S)/overlap;
			weight = (1-csi)*eta;
			interpolation *= weight;
			matrix[row][col] = interpolation;
			
		    } else {					/* (1) */
		        weight = (X - Overlap.E)/overlap;
			interpolation *= 1 - weight;
			matrix[row][col] = interpolation;
		    }
		
		} else if ((X < Overlap.W)) {
		
		    if ((Y > Overlap.N)) {			/* (4) */
		        csi = (X - General.W)/overlap;
			eta = (Y - Overlap.N)/overlap;
			weight = (1-eta)*csi;
			interpolation *= weight;
			matrix[row][col] += interpolation;
			
		    } else if ((Y < Overlap.S)) {		/* (2) */
		        csi = (X - General.W)/overlap;
			eta = (Y - General.S)/overlap;
			weight = csi*eta;
			interpolation *= weight;
			matrix[row][col] += interpolation;
			
		    } else {					/* (2) */
		        weight = (Overlap.W - X)/overlap;
			interpolation *= 1 - weight;
			matrix[row][col] += interpolation;
	    	    }
		
		} else {
		    if ((Y > Overlap.N)){			/* (3) */
		    	weight = (Y - Overlap.N)/overlap;
		    	interpolation *= 1 - weight;
		    	matrix[row][col] += interpolation;
			
		    } else {					/* (1) */
		    	weight = (Overlap.S - Y)/overlap;
		    	interpolation *= 1 - weight;
		    	matrix[row][col] = interpolation;
		    } 
	    	}
	    }
	}
      }	/* END COL */
    }	/* END ROW */
    return matrix;
}
