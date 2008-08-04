#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>

#include "outlier.h"

void P_Outlier(struct Map_info *Out, struct Map_info *Outlier,
	       struct Map_info *Qgis, struct Cell_head Elaboration,
	       BOUND_BOX General, BOUND_BOX Overlap, double **obs,
	       double *parBilin, double mean, double overlap, int *line_num,
	       int num_points, dbDriver * driver)
{
    int i;
    double interpolation, weight, residual, eta, csi, *gradient;

    extern int nsplx, nsply;
    extern double passoN, passoE;

    struct line_pnts *point;
    struct line_cats *categories;

    point = Vect_new_line_struct();
    categories = Vect_new_cats_struct();

    for (i = 0; i < num_points; i++) {	/* Sparse points */
	Vect_reset_line(point);
	Vect_reset_cats(categories);

	if (Vect_point_in_box(obs[i][0], obs[i][1], mean, &General)) {
	    interpolation =
		dataInterpolateBicubic(obs[i][0], obs[i][1], passoE, passoN,
				       nsplx, nsply, Elaboration.west,
				       Elaboration.south, parBilin);

	    interpolation += mean;

	    Vect_copy_xyz_to_pnts(point, &obs[i][0], &obs[i][1], &obs[i][2],
				  1);
	    *point->z += mean;

	    if (Vect_point_in_box(obs[i][0], obs[i][1], interpolation, &Overlap)) {	/*(5) */

		residual = *point->z - interpolation;

		if (FALSE == P_is_outlier(residual)) {
		    Vect_write_line(Out, GV_POINT, point, categories);
		    Vect_cat_set(categories, 1, (int)*point->z);
		    if (Qgis)
			Vect_write_line(Qgis, GV_POINT, point, categories);
		}

		else
		    Vect_write_line(Outlier, GV_POINT, point, categories);

	    }
	    else {
		if ((*point->x > Overlap.E) && (*point->x != General.E)) {

		    if ((*point->y > Overlap.N) && (*point->y != General.N)) {	/*(3) */
			csi = (*point->x - Overlap.E) / overlap;
			eta = (*point->y - Overlap.N) / overlap;
			weight = (1 - csi) * (1 - eta);

			gradient[0] *= weight;
			gradient[1] *= weight;
			interpolation *= weight;

			if (Select_Outlier(&gradient[0], line_num[i], driver)
			    != DB_OK)
			    G_fatal_error(_
					  ("Impossible to read the database"));

			if (UpDate_Outlier(gradient[0], line_num[i], driver)
			    != DB_OK)
			    G_fatal_error(_
					  ("Impossible to update the database"));

		    }
		    else if ((*point->y < Overlap.S) && (*point->y != General.S)) {	/*(1) */
			csi = (*point->x - Overlap.E) / overlap;
			eta = (*point->y - General.S) / overlap;
			weight = (1 - csi) * eta;

			if (Insert_Outlier
			    (interpolation * weight, line_num[i],
			     driver) != DB_OK)
			    G_fatal_error(_
					  ("Impossible to write in the database"));

		    }
		    else if ((*point->y <= Overlap.N) && (*point->y >= Overlap.S)) {	/*(1) */
			weight = (*point->x - Overlap.E) / overlap;

			if (Insert_Outlier
			    (interpolation * weight, line_num[i],
			     driver) != DB_OK)
			    G_fatal_error(_
					  ("Impossible to write in the database"));
		    }

		}
		else if ((*point->x < Overlap.W) && (*point->x != General.W)) {

		    if ((*point->y > Overlap.N) && (*point->y != General.N)) {	/*(4) */
			csi = (*point->x - General.W) / overlap;
			eta = (*point->y - Overlap.N) / overlap;
			weight = (1 - eta) * csi;

			gradient[0] *= weight;
			gradient[1] *= weight;
			interpolation *= weight;

			if (Select_Outlier(&gradient[0], line_num[i], driver)
			    != DB_OK)
			    G_fatal_error(_
					  ("Impossible to read the database"));

			residual = *point->z - interpolation;

			if (FALSE == P_is_outlier(residual)) {
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
		    else if ((*point->y < Overlap.S) && (*point->y != General.S)) {	/*(2) */
			csi = (*point->x - General.W) / overlap;
			eta = (*point->y - General.S) / overlap;
			weight = csi * eta;

			gradient[0] *= weight;
			gradient[1] *= weight;
			interpolation *= weight;

			if (Select_Outlier(&gradient[0], line_num[i], driver)
			    != DB_OK)
			    G_fatal_error(_
					  ("Impossible to read the database"));

			if (UpDate_Outlier(gradient[0], line_num[i], driver)
			    != DB_OK)
			    G_fatal_error(_
					  ("Impossible to update the database"));

		    }
		    else if ((*point->y <= Overlap.N) && (*point->y >= Overlap.S)) {	/*(2) */
			weight = (Overlap.W - *point->x) / overlap;

			gradient[0] *= weight;
			gradient[1] *= weight;
			interpolation *= weight;

			if (Select_Outlier(&gradient[0], line_num[i], driver)
			    != DB_OK)
			    G_fatal_error(_
					  ("Impossible to read the database"));

			residual = *point->z - interpolation;

			if (FALSE == P_is_outlier(residual)) {
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
		    if ((*point->y > Overlap.N) && (*point->y != General.N)) {	/*(3) */
			weight = (*point->y - Overlap.N) / overlap;

			gradient[0] *= weight;
			gradient[1] *= weight;
			interpolation *= weight;

			if (Select_Outlier(&gradient[0], line_num[i], driver)
			    != DB_OK)
			    G_fatal_error(_
					  ("Impossible to read the database"));

			residual = *point->z - interpolation;

			if (FALSE == P_is_outlier(residual)) {
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
		    else if ((*point->y < Overlap.S) && (*point->y != General.S)) {	/*(1) */
			weight = (Overlap.S - *point->y) / overlap;

			if (Insert_Outlier
			    (interpolation * weight, line_num[i], driver)
			    != DB_OK)
			    G_fatal_error(_
					  ("Impossible to write in the database"));

		    }		/*else (1) */
		}		/*else */
	    }
	}			/*end if obs */
    }				/*end for */
    Vect_destroy_line_struct(point);
    Vect_destroy_cats_struct(categories);
}				/*end puntisparsi_select */

int Insert_Outlier(double Interp, int line_num, dbDriver * driver)
{

    char buf[1024];
    dbString sql;

    db_init_string(&sql);
    sprintf(buf, "INSERT INTO Auxiliar_outlier_table (ID, Interp)");
    db_append_string(&sql, buf);
    sprintf(buf, " VALUES (%d, %lf)", line_num, Interp);
    db_append_string(&sql, buf);

    return db_execute_immediate(driver, &sql);
}

int UpDate_Outlier(double Interp, int line_num, dbDriver * driver)
{

    char buf[1024];
    dbString sql;

    db_init_string(&sql);
    sprintf(buf, "UPDATE Auxiliar_outlier_table SET Interp=%lf WHERE ID=%d",
	    Interp, line_num);
    db_append_string(&sql, buf);

    return db_execute_immediate(driver, &sql);
}

int Select_Outlier(double *Interp, int line_num, dbDriver * driver)
{

    int more;
    char buf[1024];
    dbString sql;
    dbTable *table;
    dbCursor cursor;
    dbColumn *Interp_col;
    dbValue *Interp_value;

    db_init_string(&sql);
    sprintf(buf, "SELECT ID, Interp FROM Auxiliar_outlier_table WHERE ID=%d",
	    line_num);
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
    return DB_OK;
}

int P_is_outlier(double pippo)
{
    extern double Thres_Outlier;

    if (fabs(pippo) < Thres_Outlier)
	return FALSE;

    return TRUE;
}

#ifdef notdef
/*! DEFINITION OF THE SUBZONES 

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
 */
#endif
