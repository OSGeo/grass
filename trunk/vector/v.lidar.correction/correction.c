
/********************************************************************
 *								    *
 * MODULE:       v.lidar.correction				    *
 * 								    *
 * AUTHOR(S):    Roberto Antolin & Gonzalo Moreno                   *
 *               general update Markus Metz      		    *
 *               						    *
 * PURPOSE:      Correction of the v.growing output		    *
 *               						    *
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano - 		    *
 *			     Polo Regionale di Como		    *
 *								    *
 *               This program is free software under the 	    *
 *               GNU General Public License (>=v2). 		    *
 *               Read the file COPYING that comes with GRASS	    *
 *               for details.					    *
 *								    *
 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "correction.h"

void
P_Sparse_Correction(struct Map_info *In, struct Map_info *Out,
		    struct Map_info *Terrain, struct Cell_head *Elaboration,
		    struct bound_box General, struct bound_box Overlap,
		    double **obs, struct lidar_cat *lcat, double *param,
		    int *line_num, double stepN, double stepE,
		    double overlap, double HighThresh, double LowThresh,
		    int nsplx, int nsply, int num_points,
		    dbDriver * driver, double mean, char *tab_name)
{
    int i = 0, class;
    double interpolation, csi, eta, weight;

    struct bound_box elaboration_box;
    struct line_pnts *point;
    struct line_cats *cats;

    point = Vect_new_line_struct();
    cats = Vect_new_cats_struct();

    Vect_region_box(Elaboration, &elaboration_box);

    db_begin_transaction(driver);
    
    for (i = 0; i < num_points; i++) {	/* Sparse points */
	G_percent(i, num_points, 2);
	Vect_reset_line(point);
	Vect_reset_cats(cats);
	
	if (Vect_point_in_box(obs[i][0], obs[i][1], mean, &General)) {
	    interpolation =
		dataInterpolateBilin(obs[i][0], obs[i][1], stepE, stepN,
				     nsplx, nsply, Elaboration->west,
				     Elaboration->south, param);
	    interpolation += mean;

	    Vect_copy_xyz_to_pnts(point, &obs[i][0], &obs[i][1], &obs[i][2],
				  1);
	    *point->z += mean;

	    if (Vect_point_in_box(*point->x, *point->y, *point->z, &Overlap)) {	/*(5) */

		Vect_cat_set(cats, F_EDGE_DETECTION_CLASS, lcat[i].cat_edge);
		Vect_cat_set(cats, F_INTERPOLATION, lcat[i].cat_interp);
		Vect_cat_set(cats, F_COUNTER_OBJ, lcat[i].cat_obj);
		class = lcat[i].cat_class;
		class =
		    correction(class, *point->z, interpolation, HighThresh,
			       LowThresh);
		Vect_cat_set(cats, F_CLASSIFICATION, class);

		if (class == TERRAIN_SINGLE || class == TERRAIN_DOUBLE)
		    Vect_write_line(Terrain, GV_POINT, point, cats);

		Vect_write_line(Out, GV_POINT, point, cats);
	    }
	    else {
		if ((*point->x > Overlap.E) && (*point->x < General.E)) {
		    if ((*point->y > Overlap.N) && (*point->y < General.N)) {	/*(3) */
			csi = (General.E - *point->x) / overlap;
			eta = (General.N - *point->y) / overlap;
			weight = csi * eta;
			interpolation *= weight;

			if (Select_Correction
			    (&interpolation, line_num[i], driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			if (UpDate_Correction
			    (interpolation, line_num[i], driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to update the database"));
		    }
		    else if ((*point->y < Overlap.S) && (*point->y > General.S)) {	/*(1) */
			csi = (General.E - *point->x) / overlap;
			eta = (*point->y - General.S) / overlap;
			weight = csi * eta;

			if (Insert_Correction
			    (interpolation * weight, line_num[i],
			     driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to write in the database"));
		    }
		    else if ((*point->y >= Overlap.S) && (*point->y <= Overlap.N)) {	/*(1) */
			weight = (General.E - *point->x) / overlap;

			if (Insert_Correction
			    (interpolation * weight, line_num[i],
			     driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to write in the database"));
		    }
		}
		else if ((*point->x < Overlap.W) && (*point->x > General.W)) {
		    if ((*point->y > Overlap.N) && (*point->y < General.N)) {	/*(4) */
			csi = (*point->x - General.W) / overlap;
			eta = (General.N - *point->y) / overlap;
			weight = eta * csi;
			interpolation *= weight;

			if (Select_Correction
			    (&interpolation, line_num[i], driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			Vect_cat_set(cats, F_EDGE_DETECTION_CLASS, lcat[i].cat_edge);
			Vect_cat_set(cats, F_INTERPOLATION, lcat[i].cat_interp);
			Vect_cat_set(cats, F_COUNTER_OBJ, lcat[i].cat_obj);
			class = lcat[i].cat_class;
			class =
			    correction(class, *point->z, interpolation,
				       HighThresh, LowThresh);
			Vect_cat_set(cats, F_CLASSIFICATION, class);

			Vect_write_line(Out, GV_POINT, point, cats);
			if (class == TERRAIN_SINGLE ||
			    class == TERRAIN_DOUBLE)
			    Vect_write_line(Terrain, GV_POINT, point, cats);
		    }
		    else if ((*point->y < Overlap.S) && (*point->y > General.S)) {	/*(2) */
			csi = (*point->x - General.W) / overlap;
			eta = (*point->y - General.S) / overlap;
			weight = csi * eta;
			interpolation *= weight;

			if (Select_Correction
			    (&interpolation, line_num[i], driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			if (UpDate_Correction
			    (interpolation, line_num[i], driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to update the database"));
		    }
		    else if ((*point->y >= Overlap.S) && (*point->y <= Overlap.N)) {	/*(2) */
			weight = (*point->x - General.W) / overlap;
			interpolation *= weight;

			if (Select_Correction
			    (&interpolation, line_num[i], driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			Vect_cat_set(cats, F_EDGE_DETECTION_CLASS, lcat[i].cat_edge);
			Vect_cat_set(cats, F_INTERPOLATION, lcat[i].cat_interp);
			Vect_cat_set(cats, F_COUNTER_OBJ, lcat[i].cat_obj);
			class = lcat[i].cat_class;
			class =
			    correction(class, *point->z, interpolation,
				       HighThresh, LowThresh);
			Vect_cat_set(cats, F_CLASSIFICATION, class);

			Vect_write_line(Out, GV_POINT, point, cats);
			if (class == TERRAIN_SINGLE ||
			    class == TERRAIN_DOUBLE)
			    Vect_write_line(Terrain, GV_POINT, point, cats);
		    }
		}
		else if ((*point->x >= Overlap.W) && (*point->x <= Overlap.E)) {
		    if ((*point->y > Overlap.N) && (*point->y < General.N)) {	/*(3) */
			weight = (General.N - *point->y) / overlap;
			interpolation *= weight;

			if (Select_Correction
			    (&interpolation, line_num[i], driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			Vect_cat_set(cats, F_EDGE_DETECTION_CLASS, lcat[i].cat_edge);
			Vect_cat_set(cats, F_INTERPOLATION, lcat[i].cat_interp);
			Vect_cat_set(cats, F_COUNTER_OBJ, lcat[i].cat_obj);
			class = lcat[i].cat_class;
			class =
			    correction(class, *point->z, interpolation,
				       HighThresh, LowThresh);
			Vect_cat_set(cats, F_CLASSIFICATION, class);

			Vect_write_line(Out, GV_POINT, point, cats);
			if (class == TERRAIN_SINGLE ||
			    class == TERRAIN_DOUBLE)
			    Vect_write_line(Terrain, GV_POINT, point, cats);

		    }
		    else if ((*point->y < Overlap.S) && (*point->y > General.S)) {	/*(1) */
			weight = (*point->y - General.S) / overlap;

			if (Insert_Correction
			    (interpolation * weight, line_num[i],
			     driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to write in the database"));
		    }
		}
	    }
	}  /* if in General box */
    }  /* while */
    G_percent(num_points, num_points, 2);
    Vect_destroy_line_struct(point);
    Vect_destroy_cats_struct(cats);

    db_commit_transaction(driver);

    return;
}

/*------------------------------------------------------------------------------------------------*/
int correction(int class, double obsZ, double interpolation,
	       double HighThresh, double LowThresh)
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

int Select_Correction(double *Interp, int line_num, dbDriver * driver, char *tab_name)
{
    int more;
    char buf[1024];
    dbString sql;
    dbTable *table;
    dbCursor cursor;
    dbColumn *Interp_col;
    dbValue *Interp_value;

    db_init_string(&sql);

    sprintf(buf, "SELECT ID, Interp FROM %s WHERE ID=%d", tab_name, line_num);
    db_append_string(&sql, buf);

    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK)
	return -1;

    while (db_fetch(&cursor, DB_NEXT, &more) == DB_OK && more) {
	table = db_get_cursor_table(&cursor);
	Interp_col = db_get_table_column(table, 1);

	if (db_sqltype_to_Ctype(db_get_column_sqltype(Interp_col)) ==
	    DB_C_TYPE_DOUBLE)
	    Interp_value = db_get_column_value(Interp_col);
	else
	    continue;

	*Interp += db_get_value_double(Interp_value);
    }
    db_close_cursor(&cursor);
    db_free_string(&sql);
    return DB_OK;
}

int Insert_Correction(double Interp, int line_num, dbDriver * driver, char *tab_name)
{
    char buf[1024];
    dbString sql;
    int ret;

    db_init_string(&sql);
    sprintf(buf, "INSERT INTO %s (ID, Interp)", tab_name);
    db_append_string(&sql, buf);
    sprintf(buf, " VALUES (%d, %lf)", line_num, Interp);
    db_append_string(&sql, buf);

    ret = db_execute_immediate(driver, &sql);
    db_free_string(&sql);
    return ret;
}

int UpDate_Correction(double Interp, int line_num, dbDriver * driver, char *tab_name)
{
    char buf[1024];
    dbString sql;
    int ret;

    db_init_string(&sql);
    sprintf(buf,
	    "UPDATE %s SET Interp=%lf WHERE ID=%d", tab_name, Interp, line_num);
    db_append_string(&sql, buf);

    ret = db_execute_immediate(driver, &sql);
    db_free_string(&sql);
    return ret;
}

struct Point *P_Read_Vector_Correction(struct Map_info *Map,
				       struct Cell_head *Elaboration,
				       int *num_points, int *num_terrain,
				       int dim_vect, struct lidar_cat **lcat)
{
    int line_num, npoints, nterrain, pippo, cat;
    double x, y, z;
    struct Point *obs;
    struct lidar_cat *lc;
    struct line_pnts *points;
    struct line_cats *categories;
    struct bound_box elaboration_box;

    pippo = dim_vect;

    obs = (struct Point *)G_calloc(pippo, sizeof(struct Point));
    lc = (struct lidar_cat *)G_calloc(pippo, sizeof(struct lidar_cat));

    points = Vect_new_line_struct();
    categories = Vect_new_cats_struct();

    /* Reading the elaboration zone points */
    Vect_region_box(Elaboration, &elaboration_box);

    npoints = 0;
    nterrain = 0;
    line_num = 0;

    /* Read every line for buffering points */
    Vect_rewind(Map);
    while (Vect_read_next_line(Map, points, categories) > 0) {

	line_num++;

	x = points->x[0];
	y = points->y[0];
	if (points->z != NULL)
	    z = points->z[0];
	else
	    z = 0.0;

	/* Reading and storing points only if it is in elaboration_reg */
	if (Vect_point_in_box(x, y, z, &elaboration_box)) {
	    npoints++;
	    if (npoints >= pippo) {
		pippo += dim_vect;
		obs = (struct Point *)G_realloc((void *)obs,
			    (signed int)(pippo * sizeof(struct Point)));
		lc = (struct lidar_cat *)G_realloc((void *)lc,
			    (signed int)(pippo * sizeof(struct lidar_cat)));
	    }

	    /* Storing observation vector */
	    obs[npoints - 1].coordX = x;
	    obs[npoints - 1].coordY = y;
	    obs[npoints - 1].coordZ = z;
	    obs[npoints - 1].lineID = line_num;	/* Storing also the line's number */
	    Vect_cat_get(categories, F_EDGE_DETECTION_CLASS, &cat);
	    obs[npoints - 1].cat = cat;
	    lc[npoints - 1].cat_edge = cat;

	    /* Only terrain points */
	    if (cat == TERRAIN_SINGLE)
		nterrain++;

	    Vect_cat_get(categories, F_CLASSIFICATION, &cat);
	    lc[npoints - 1].cat_class = cat;
	    Vect_cat_get(categories, F_INTERPOLATION, &cat);
	    lc[npoints - 1].cat_interp = cat;
	    Vect_cat_get(categories, F_COUNTER_OBJ, &cat);
	    lc[npoints - 1].cat_obj = cat;
	}
    }
    Vect_destroy_line_struct(points);
    Vect_destroy_cats_struct(categories);

    *num_points = npoints;
    *num_terrain = nterrain;
    *lcat = lc;
    return obs;
}

/*! DEFINITION OF THE SUBZONES 

  5: inside Overlap region
  all others: inside General region but outside Overlap region

   ---------------------------------
   | |       | |       | |       | |
   ---------------------------------
   | |       | |       | |       | |
   | |       | |       | |       | |
   | |       | |       | |       | |
   ---------------------------------
   | |       |4|   3   |3|       | |
   ---------------------------------
   | |       | |       | |       | |
   | |       |2|   5   |1|       | |
   | |       | |       | |       | |
   ---------------------------------
   | |       |2|   1   |1|       | |
   ---------------------------------
   | |       | |       | |       | |
   | |       | |       | |       | |
   | |       | |       | |       | |
   ---------------------------------
   | |       | |       | |       | |
   ---------------------------------
 */
