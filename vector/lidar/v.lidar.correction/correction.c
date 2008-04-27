/**************************************************************
 *									*
 * MODULE:       v.lidar.correction					*
 * 									*
 * AUTHOR(S):    Roberto Antolin & Gonzalo Moreno			*
 *               							*
 * PURPOSE:      Correction of the v.growing output			*
 *               							*
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano - 			*
 *			     Polo Regionale di Como			*
 *									*
 *               This program is free software under the 		*
 *               GNU General Public License (>=v2). 			*
 *               Read the file COPYING that comes with GRASS		*
 *               for details.						*
 *									*
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>

#include "correction.h"

void 
P_Sparse_Correction (struct Map_info *In, struct Map_info *Out, struct Map_info *Terrain, struct Cell_head *Elaboration, BOUND_BOX General,\
		BOUND_BOX Overlap, double **obs, double *param, int *line_num, double passoN, double passoE, double overlap, double HighThresh,\
		double LowThresh, int nsplx, int nsply, int num_points,dbDriver *driver, double mean)
{
    int i=0, class;
    double interpolation, csi, eta, weight;
    
    struct line_pnts *points;
    struct line_cats *cats;
    
    points = Vect_new_line_struct ();
    cats = Vect_new_cats_struct ();
	
    Vect_rewind (In);
    while (Vect_read_next_line (In, points, cats) > 0) {
	i++;	
	if ( Vect_point_in_box (*points->x, *points->y, *points->z, &General) ) {	
	    interpolation = dataInterpolateBilin (*points->x, *points->y, passoE, passoN, nsplx, nsply, Elaboration->west, \
	    			Elaboration->south, param); 
	    interpolation += mean;
	
	    if (Vect_point_in_box (*points->x, *points->y, *points->z, &Overlap)) {  /*(5)*/

		Vect_cat_get (cats, F_CLASSIFICATION, &class);
		class = correction (class, *points->z, interpolation, HighThresh, LowThresh);
		Vect_cat_del (cats, F_CLASSIFICATION);
		Vect_cat_set (cats, F_CLASSIFICATION, class);

		if (class == TERRAIN_SINGLE || class == TERRAIN_DOUBLE)
			Vect_write_line (Terrain, GV_POINT, points, cats);

		Vect_write_line (Out, GV_POINT, points, cats);
    
	    } else {
		    
		if ((*points->x > Overlap.E)) {

		    if ((*points->y > Overlap.N)) {		/*(3)*/
			csi = (*points->x - Overlap.E)/overlap;
			eta = (*points->y - Overlap.N)/overlap;
			weight = (1-csi)*(1-eta);
			interpolation *= weight;

			if (Select_Correction (&interpolation, line_num[i], driver) != DB_OK) 
				G_fatal_error (_("Impossible to read the database"));

			if (UpDate_Correction (interpolation, line_num[i], driver) != DB_OK)
				G_fatal_error (_("Impossible to update the database"));
			
		    } else if ((*points->y < Overlap.S)) {	/*(1)*/
		        csi = (*points->x - Overlap.E)/overlap;
			eta = (*points->y - General.S)/overlap;
			weight = (1-csi)*eta;

			if (Insert_Correction (interpolation*weight, line_num[i], driver) != DB_OK) 
			    G_fatal_error (_("Impossible to write in the database"));
			
		    } else {					/*(1)*/
		        weight = (*points->x - Overlap.E)/overlap;

			if (Insert_Correction (interpolation*weight, line_num[i], driver) != DB_OK) 
			    G_fatal_error (_("Impossible to write in the database"));
		    }
		} else if ((*points->x < Overlap.W)) {
		    if ((*points->y > Overlap.N)) {		/*(4)*/  
			csi = (*points->x - General.W)/overlap;
			eta = (*points->y - Overlap.N)/overlap;
			weight = (1-eta)*csi;

			interpolation *= weight;
			if (Select_Correction (&interpolation, line_num[i], driver) != DB_OK) 
				G_fatal_error (_("Impossible to read the database"));

			Vect_cat_get (cats, F_CLASSIFICATION, &class);
			class = correction (class, *points->z, interpolation, HighThresh, LowThresh);
			Vect_cat_set (cats, F_CLASSIFICATION, class);
		
			Vect_write_line (Out, GV_POINT, points, cats);	
			if (class == TERRAIN_SINGLE || class == TERRAIN_DOUBLE)
			    Vect_write_line (Terrain, GV_POINT, points, cats);

		    } else if ((*points->y < Overlap.S)) {	/*(2)*/
			csi = (*points->x - General.W)/overlap;
			eta = (*points->y - General.S)/overlap;
			weight = csi*eta;
			interpolation *= weight;

			if (Select_Correction (&interpolation, line_num[i], driver) != DB_OK) 
				G_fatal_error (_("Impossible to read the database"));

			if (UpDate_Correction (interpolation, line_num[i], driver) != DB_OK)
				G_fatal_error (_("Impossible to update the database"));
		    } else {					/*(2)*/
			weight = (Overlap.W - *points->x)/overlap;
			interpolation *= weight;
			
			if (Select_Correction (&interpolation, line_num[i], driver) != DB_OK) 
				G_fatal_error (_("Impossible to read the database"));

			Vect_cat_get (cats, F_CLASSIFICATION, &class);
			class = correction (class, *points->z, interpolation, HighThresh, LowThresh);
			Vect_cat_set (cats, F_CLASSIFICATION, class);

			Vect_write_line (Out, GV_POINT, points, cats);	
			if (class == TERRAIN_SINGLE || class == TERRAIN_DOUBLE)
			    Vect_write_line (Terrain, GV_POINT, points, cats);

	    	    }
		    
		} else {
		    if ((*points->y > Overlap.N)) {		/*(3)*/
		    	weight = (*points->y - Overlap.N)/overlap;
			interpolation *= weight;
			
			if (Select_Correction (&interpolation, line_num[i], driver) != DB_OK) 
				G_fatal_error (_("Impossible to read the database"));

			Vect_cat_get (cats, F_CLASSIFICATION, &class);
			class = correction (class, *points->z, interpolation, HighThresh, LowThresh);
			Vect_cat_set (cats, F_CLASSIFICATION, class);

			Vect_write_line (Out, GV_POINT, points, cats);	
			if (class == TERRAIN_SINGLE || class == TERRAIN_DOUBLE)
			    Vect_write_line (Terrain, GV_POINT, points, cats);

		    } else {						/*(1)*/
		    	weight = (Overlap.S - *points->y)/overlap;

			if (Insert_Correction (interpolation*weight, line_num[i], driver) != DB_OK) 
			    G_fatal_error (_("Impossible to write in the database"));
		    }
		} 
	    }
	} /*IF*/
	
	Vect_reset_line (points);
	Vect_reset_cats (cats); 
	
    }	/*FOR*/	

    Vect_destroy_line_struct (points);
    Vect_destroy_cats_struct (cats);

    return;
}

/*------------------------------------------------------------------------------------------------*/
int correction (int class, double obsZ, double interpolation, double HighThresh, double LowThresh)
{
    if ((class == TERRAIN_SINGLE) && ((obsZ - interpolation) >= HighThresh))
	return OBJECT_SINGLE;

    if ((class == TERRAIN_DOUBLE) && ((obsZ - interpolation) >= HighThresh))
	return OBJECT_DOUBLE;

    if ((class == OBJECT_SINGLE) && (fabs(interpolation - obsZ) <= LowThresh))
	return TERRAIN_SINGLE;

    if ((class == OBJECT_DOUBLE) && (fabs(interpolation - obsZ) <= LowThresh))
	return TERRAIN_DOUBLE;
	
    return class;
}

int Select_Correction (double *Interp, int line_num, dbDriver *driver)
{
    int more;
    char buf[1024];
    dbString sql;
    dbTable *table;
    dbCursor cursor;
    dbColumn *Interp_col;
    dbValue *Interp_value;

    db_init_string (&sql);
    db_zero_string (&sql);

    sprintf (buf, "SELECT Interp FROM Auxiliar_correction_table WHERE ID=%d", line_num);
    db_append_string (&sql, buf);

    if (db_open_select_cursor (driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK) return -1;

    while ( db_fetch (&cursor, DB_NEXT, &more) == DB_OK && more ) { 
	table = db_get_cursor_table (&cursor);
	Interp_col = db_get_table_column (table,1);

	if ( db_sqltype_to_Ctype (db_get_column_sqltype (Interp_col)) == DB_C_TYPE_DOUBLE ) 
	    Interp_value = db_get_column_value (Interp_col);
	else continue;

	*Interp += db_get_value_double (Interp_value);
    }
    return DB_OK;
}

int Insert_Correction (double Interp, int line_num, dbDriver *driver)
{
    char buf[1024];
    dbString sql;

    db_init_string (&sql);
    sprintf (buf, "INSERT INTO Auxiliar_correction_table (ID, Interp)");
    db_append_string (&sql, buf);
    sprintf (buf, " VALUES (%d, %lf)", line_num, Interp);
    db_append_string (&sql, buf);
    
    return db_execute_immediate (driver, &sql);
}

int UpDate_Correction (double Interp, int line_num, dbDriver *driver)
{
    char buf[1024];
    dbString sql;
    
    db_init_string (&sql);
    sprintf (buf, "UPDATE Auxiliar_correction_table SET Interp=%lf WHERE ID=%d", Interp, line_num);
    db_append_string (&sql, buf);

    return db_execute_immediate (driver, &sql);
}

struct Point*
P_Read_Vector_Correction (struct Map_info *Map, struct Cell_head *Elaboration, int *num_points, int *num_terrain, int dim_vect) {
    int line_num, npoints, nterrain, pippo, cat_edge;
    double x, y, z;
    struct Point* obs;
    struct line_pnts *points;
    struct line_cats *categories;
    BOUND_BOX elaboration_box;

    pippo = dim_vect;

    /*if (first_it) */
        obs = (struct Point*) G_calloc (pippo, sizeof(struct Point));
    /*else */
        /*obs = (struct Point*) G_realloc ((void *) obs, pippo * sizeof(struct Point));*/

    points = Vect_new_line_struct ();
    categories = Vect_new_cats_struct ();

/* Reading the elaboration zone points */
    Vect_region_box (Elaboration, &elaboration_box);

    npoints = 0;
    nterrain = 0;
    line_num = 0;

    /* Read every line for buffering points */
    Vect_rewind (Map);
    while (Vect_read_next_line (Map, points, categories) > 0) {

        line_num ++;

        x = points->x[0];
        y = points->y[0];
        if (points->z != NULL) z = points->z[0];
        else z = 0.0;

    /* Reading and storing points only if it is in elaboration_reg */
        if (Vect_point_in_box (x, y, z, &elaboration_box)) {
           npoints ++;
           if (npoints >= pippo) {
                pippo += dim_vect;
                obs = (struct Point *) G_realloc ((void *) obs, (signed int) (pippo * sizeof(struct Point)));
                /*lineID = (int *) G_realloc ((void *) lineID, pippo * sizeof(int));*/
           }

    /* Storing observation vector */
           obs[npoints-1].coordX = x;
           obs[npoints-1].coordY = y;
           obs[npoints-1].coordZ = z;
           obs[npoints-1].lineID = line_num;               /* Storing also the line's number */
	   Vect_cat_get (categories, F_EDGE_DETECTION_CLASS, &cat_edge);
	   obs[npoints-1].cat = cat_edge;
	}
    
    	/* Only terrain points */
	if (cat_edge == TERRAIN_SINGLE)
	    nterrain ++;
    }
    Vect_destroy_line_struct (points);
    Vect_destroy_cats_struct (categories);

    *num_points = npoints;
    *num_terrain = nterrain;
    return obs;
}



#ifdef notdef
/* SUBZONES DEFINITION */
		-----------------------
		|4|   3   |3|       | |
		-----------------------
		| |       | |       | |
		|2|   5   |1|       | |
		| |       | |       | |
		-----------------------
		|2|   1   |1|       | |
		-----------------------
		| |       | |       | |
		| |       | |       | |
		| |       | |       | |
		-----------------------
		| |       | |       | |
		-----------------------
#endif
