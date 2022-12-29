#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "outlier.h"

typedef int (*outlier_fn)(double);

static outlier_fn is_outlier;

void P_set_outlier_fn(int filter_mode)
{
    if (filter_mode < 0)
	is_outlier = P_is_outlier_n;
    else if (filter_mode > 0)
	is_outlier = P_is_outlier_p;
    else
	is_outlier = P_is_outlier;
}

extern double Thres_Outlier;

void P_Outlier(struct Map_info *Out, struct Map_info *Outlier,
	       struct Map_info *Qgis, struct Cell_head Elaboration,
	       struct bound_box General, struct bound_box Overlap,
	       double **obs, double *parBilin, double mean, double overlap,
	       int *line_num, int num_points, dbDriver * driver,
	       char *tab_name)
{
    int i;
    double interpolation, weight, residual, eta, csi;
    extern int nsplx, nsply;
    extern double stepN, stepE;
    struct line_pnts *point;
    struct line_cats *categories;


    point = Vect_new_line_struct();
    categories = Vect_new_cats_struct();

    db_begin_transaction(driver);
    
    for (i = 0; i < num_points; i++) {	/* Sparse points */
	G_percent(i, num_points, 2);
	Vect_reset_line(point);
	Vect_reset_cats(categories);

	if (Vect_point_in_box(obs[i][0], obs[i][1], mean, &General)) {
	    interpolation =
		dataInterpolateBicubic(obs[i][0], obs[i][1], stepE, stepN,
				       nsplx, nsply, Elaboration.west,
				       Elaboration.south, parBilin);

	    interpolation += mean;

	    Vect_copy_xyz_to_pnts(point, &obs[i][0], &obs[i][1], &obs[i][2],
				  1);
	    *point->z += mean;

	    if (Vect_point_in_box(obs[i][0], obs[i][1], interpolation, &Overlap)) {	/*(5) */

		residual = *point->z - interpolation;

		if (FALSE == is_outlier(residual)) {
		    Vect_write_line(Out, GV_POINT, point, categories);
		    Vect_cat_set(categories, 1, (int)*point->z);
		    if (Qgis)
			Vect_write_line(Qgis, GV_POINT, point, categories);
		}

		else
		    Vect_write_line(Outlier, GV_POINT, point, categories);

	    }
	    else {
		if ((*point->x > Overlap.E) && (*point->x < General.E)) {
		    if ((*point->y > Overlap.N) && (*point->y < General.N)) {	/*(3) */
			csi = (General.E - *point->x) / overlap;
			eta = (General.N - *point->y) / overlap;
			weight = csi * eta;

			interpolation *= weight;

			if (Select_Outlier(&interpolation, line_num[i],
					   driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			if (UpDate_Outlier(interpolation, line_num[i],
			                   driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to update the database"));
		    }
		    else if ((*point->y < Overlap.S) && (*point->y > General.S)) {	/*(1) */
			csi = (General.E - *point->x) / overlap;
			eta = (*point->y - General.S) / overlap;
			weight = csi * eta;

			interpolation *= weight;

			if (Insert_Outlier(interpolation, line_num[i],
					   driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to write in the database"));
		    }
		    else if ((*point->y <= Overlap.N) && (*point->y >= Overlap.S)) {	/*(1) */
			weight = (General.E - *point->x) / overlap;

			interpolation *= weight;

			if (Insert_Outlier(interpolation, line_num[i],
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

			if (Select_Outlier(&interpolation, line_num[i],
			                   driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			residual = *point->z - interpolation;

			if (FALSE == is_outlier(residual)) {
			    Vect_write_line(Out, GV_POINT, point, categories);
			    Vect_cat_set(categories, 1, (int)*point->z);
			    if (Qgis)
				Vect_write_line(Qgis, GV_POINT, point,
						categories);
			}
			else {
			    Vect_write_line(Outlier, GV_POINT, point,
					    categories);
			}
		    }
		    else if ((*point->y < Overlap.S) && (*point->y > General.S)) {	/*(2) */
			csi = (*point->x - General.W) / overlap;
			eta = (*point->y - General.S) / overlap;
			weight = csi * eta;

			interpolation *= weight;

			if (Select_Outlier(&interpolation, line_num[i],
			                   driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			if (UpDate_Outlier(interpolation, line_num[i],
			                   driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to update the database"));
		    }
		    else if ((*point->y <= Overlap.N) && (*point->y >= Overlap.S)) {	/*(2) */
			weight = (*point->x - General.W) / overlap;

			interpolation *= weight;

			if (Select_Outlier(&interpolation, line_num[i],
			                   driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			residual = *point->z - interpolation;

			if (FALSE == is_outlier(residual)) {
			    Vect_write_line(Out, GV_POINT, point, categories);
			    Vect_cat_set(categories, 1, (int)*point->z);
			    if (Qgis)
				Vect_write_line(Qgis, GV_POINT, point,
						categories);
			}
			else
			    Vect_write_line(Outlier, GV_POINT, point,
					    categories);
		    }
		}
		else if ((*point->x <= Overlap.E) && (*point->x >= Overlap.W)) {
		    if ((*point->y > Overlap.N) && (*point->y < General.N)) {	/*(3) */
			weight = (General.N - *point->y) / overlap;

			interpolation *= weight;

			if (Select_Outlier(&interpolation, line_num[i],
			                   driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to read the database"));

			residual = *point->z - interpolation;

			if (FALSE == is_outlier(residual)) {
			    Vect_write_line(Out, GV_POINT, point, categories);
			    Vect_cat_set(categories, 1, (int)*point->z);
			    if (Qgis)
				Vect_write_line(Qgis, GV_POINT, point,
						categories);
			}
			else
			    Vect_write_line(Outlier, GV_POINT, point,
					    categories);
		    }
		    else if ((*point->y < Overlap.S) && (*point->y > General.S)) {	/*(1) */
			weight = (*point->y - General.S) / overlap;

			interpolation *= weight;

			if (Insert_Outlier(interpolation, line_num[i],
					   driver, tab_name) != DB_OK)
			    G_fatal_error(_("Impossible to write in the database"));

		    }		/*else (1) */
		}		/*else */
	    }
	}			/*end if obs */
    }				/*end for */

    G_percent(num_points, num_points, 2);
    G_debug(2, "P_outlier: done");

    db_commit_transaction(driver);

    Vect_destroy_line_struct(point);
    Vect_destroy_cats_struct(categories);
}				/*end puntisparsi_select */

int Insert_Outlier(double Interp, int line_num, dbDriver * driver,
		   char *tab_name)
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

int UpDate_Outlier(double Interp, int line_num, dbDriver * driver,
		   char *tab_name)
{
    char buf[1024];
    dbString sql;
    int ret;

    db_init_string(&sql);
    sprintf(buf, "UPDATE %s SET Interp=%lf WHERE ID=%d",
	    tab_name, Interp, line_num);
    db_append_string(&sql, buf);

    ret = db_execute_immediate(driver, &sql);
    db_free_string(&sql);

    return ret;
}

int Select_Outlier(double *Interp, int line_num, dbDriver * driver,
		   char *tab_name)
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

int P_is_outlier(double pippo)
{
    if (fabs(pippo) < Thres_Outlier)
	return FALSE;

    return TRUE;
}

int P_is_outlier_p(double pippo)
{
    if (pippo < Thres_Outlier)
	return FALSE;

    return TRUE;
}

int P_is_outlier_n(double pippo)
{
    if (pippo > Thres_Outlier)
	return FALSE;

    return TRUE;
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
