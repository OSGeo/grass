#include <grass/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/lidar.h>

/*------------------------------------------------------------------------------------------------*/
void
P_Sparse_Points(struct Map_info *Out, struct Cell_head *Elaboration,
		struct bound_box General, struct bound_box Overlap, double **obs,
		double *param, int *line_num, double pe, double pn,
		double overlap, int nsplx, int nsply, int num_points,
		int bilin, struct line_cats *categories, dbDriver * driver,
		double mean, char *tab_name)
{
    int i;
    char buf[1024];
    dbString sql;

    double interpolation, csi, eta, weight;
    struct line_pnts *point;

    point = Vect_new_line_struct();

    db_begin_transaction(driver);
    
    for (i = 0; i < num_points; i++) {

	if (Vect_point_in_box(obs[i][0], obs[i][1], mean, &General)) {	/*Here mean is just for asking if obs point is in box */

	    if (bilin)
		interpolation =
		    dataInterpolateBilin(obs[i][0], obs[i][1], pe, pn, nsplx,
					 nsply, Elaboration->west,
					 Elaboration->south, param);
	    else
		interpolation =
		    dataInterpolateBicubic(obs[i][0], obs[i][1], pe, pn,
					   nsplx, nsply, Elaboration->west,
					   Elaboration->south, param);

	    interpolation += mean;
	    Vect_copy_xyz_to_pnts(point, &obs[i][0], &obs[i][1],
				  &interpolation, 1);

	    if (Vect_point_in_box(obs[i][0], obs[i][1], interpolation, &Overlap)) {	/*(5) */
		Vect_write_line(Out, GV_POINT, point, categories);
	    }
	    else {
		db_init_string(&sql);

		sprintf(buf, "INSERT INTO %s (ID, X, Y, Interp)", tab_name);
		db_append_string(&sql, buf);

		sprintf(buf, " VALUES (");
		db_append_string(&sql, buf);
		sprintf(buf, "%d, %f, %f, ", line_num[i], obs[i][0],
			obs[i][1]);
		db_append_string(&sql, buf);

		if ((*point->x > Overlap.E) && (*point->x < General.E)) {
		    if ((*point->y > Overlap.N) && (*point->y < General.N)) {	/*(3) */
			csi = (General.E - *point->x) / overlap;
			eta = (General.N - *point->y) / overlap;
			weight = csi * eta;
			*point->z = weight * interpolation;

			sprintf(buf, "%lf", *point->z);
			db_append_string(&sql, buf);
			sprintf(buf, ")");
			db_append_string(&sql, buf);

			if (db_execute_immediate(driver, &sql) != DB_OK)
			    G_fatal_error(_("Unable to access table <%s>"),
					  tab_name);
		    }
		    else if ((*point->y < Overlap.S) && (*point->y > General.S)) {	/*(1) */
			csi = (General.E - *point->x) / overlap;
			eta = (*point->y - General.S) / overlap;
			weight = csi * eta;
			*point->z = weight * interpolation;

			sprintf(buf, "%lf", *point->z);
			db_append_string(&sql, buf);
			sprintf(buf, ")");
			db_append_string(&sql, buf);

			if (db_execute_immediate(driver, &sql) != DB_OK)
			    G_fatal_error(_("Unable to access table <%s>"),
					  tab_name);
		    }
		    else if ((*point->y <= Overlap.N) && (*point->y >= Overlap.S)) {	/*(1) */
			weight = (General.E - *point->x) / overlap;
			*point->z = weight * interpolation;

			sprintf(buf, "%lf", *point->z);
			db_append_string(&sql, buf);
			sprintf(buf, ")");
			db_append_string(&sql, buf);

			if (db_execute_immediate(driver, &sql) != DB_OK)
			    G_fatal_error(_("Unable to access table <%s>"),
					  tab_name);
		    }
		}
		else if ((*point->x < Overlap.W) && (*point->x > General.W)) {
		    if ((*point->y > Overlap.N) && (*point->y < General.N)) {	/*(4) */
			csi = (*point->x - General.W) / overlap;
			eta = (General.N - *point->y) / overlap;
			weight = eta * csi;
			*point->z = weight * interpolation;

			sprintf(buf, "%lf", *point->z);
			db_append_string(&sql, buf);
			sprintf(buf, ")");
			db_append_string(&sql, buf);

			if (db_execute_immediate(driver, &sql) != DB_OK)
			    G_fatal_error(_("Unable to access table <%s>"),
					  tab_name);
		    }
		    else if ((*point->y < Overlap.S) && (*point->y > General.S)) {	/*(2) */
			csi = (*point->x - General.W) / overlap;
			eta = (*point->y - General.S) / overlap;
			weight = csi * eta;
			*point->z = weight * interpolation;

			sprintf(buf, "%lf", *point->z);
			db_append_string(&sql, buf);
			sprintf(buf, ")");
			db_append_string(&sql, buf);

			if (db_execute_immediate(driver, &sql) != DB_OK)
			    G_fatal_error(_("Unable to access table <%s>"),
					  tab_name);
		    }
		    else if ((*point->y >= Overlap.S) && (*point->y <= Overlap.N)) {	/*(2) */
			weight = (*point->x - General.W) / overlap;
			*point->z = weight * interpolation;

			sprintf(buf, "%lf", *point->z);
			db_append_string(&sql, buf);
			sprintf(buf, ")");
			db_append_string(&sql, buf);

			if (db_execute_immediate(driver, &sql) != DB_OK)
			    G_fatal_error(_("Unable to access table <%s>"),
					  tab_name);
		    }
		}
		else if ((*point->x >= Overlap.W) && (*point->x <= Overlap.E)){
		    if ((*point->y > Overlap.N) && (*point->y < General.N)) {	/*(3) */
			weight = (General.N - *point->y) / overlap;
			*point->z = weight * interpolation;

			sprintf(buf, "%lf", *point->z);
			db_append_string(&sql, buf);
			sprintf(buf, ")");
			db_append_string(&sql, buf);

			if (db_execute_immediate(driver, &sql) != DB_OK)
			    G_fatal_error(_("Unable to access table <%s>"),
					  tab_name);
		    }
		    else if ((*point->y < Overlap.S) && (*point->y > General.S)) {	/*(1) */
			weight = (*point->y - General.S) / overlap;
			*point->z = (1 - weight) * interpolation;

			sprintf(buf, "%lf", *point->z);
			db_append_string(&sql, buf);
			sprintf(buf, ")");
			db_append_string(&sql, buf);

			if (db_execute_immediate(driver, &sql) != DB_OK)
			    G_fatal_error(_("Unable to access table <%s>"),
					  tab_name);
		    }
		}
	    }
	}  /*IF*/
    }  /*FOR*/

    db_commit_transaction(driver);

    return;
}


/*------------------------------------------------------------------------------------------------*/
int P_Regular_Points(struct Cell_head *Elaboration, struct Cell_head *Original,
                          struct bound_box General, struct bound_box Overlap,
			  SEGMENT *out_seg, double *param,
			  double passoN, double passoE, double overlap,
			  double mean, int nsplx, int nsply,
			  int nrows, int ncols, int bilin)
{

    int col, row, startcol, endcol, startrow, endrow;
    double X, Y, interpolation, weight, csi, eta, dval;

    /* G_get_window(&Original); */
    if (Original->north > General.N)
	startrow = (Original->north - General.N) / Original->ns_res - 1;
    else
	startrow = 0;
    if (Original->north > General.S) {
	endrow = (Original->north - General.S) / Original->ns_res + 1;
	if (endrow > nrows)
	    endrow = nrows;
    }
    else
	endrow = nrows;
    if (General.W > Original->west)
	startcol = (General.W - Original->west) / Original->ew_res - 1;
    else
	startcol = 0;
    if (General.E > Original->west) {
	endcol = (General.E - Original->west) / Original->ew_res + 1;
	if (endcol > ncols)
	    endcol = ncols;
    }
    else
	endcol = ncols;

    for (row = startrow; row < endrow; row++) {
	for (col = startcol; col < endcol; col++) {

	    X = Rast_col_to_easting((double)(col) + 0.5, Original);
	    Y = Rast_row_to_northing((double)(row) + 0.5, Original);

	    if (Vect_point_in_box(X, Y, mean, &General)) {	/* Here, mean is just for asking if obs point is in box */

		if (bilin)
		    interpolation =
			dataInterpolateBilin(X, Y, passoE, passoN, nsplx,
					     nsply, Elaboration->west,
					     Elaboration->south, param);
		else
		    interpolation =
			dataInterpolateBicubic(X, Y, passoE, passoN, nsplx,
					       nsply, Elaboration->west,
					       Elaboration->south, param);

		interpolation += mean;

		if (Vect_point_in_box(X, Y, interpolation, &Overlap)) {	/* (5) */
		    dval = interpolation;
		}
		else {
		    Segment_get(out_seg, &dval, row, col);
		    if ((X > Overlap.E) && (X < General.E)) {
			if ((Y > Overlap.N) && (Y < General.N)) {	/* (3) */
			    csi = (General.E - X) / overlap;
			    eta = (General.N - Y) / overlap;
			    weight = csi * eta;
			    interpolation *= weight;
			    dval += interpolation;
			}
			else if ((Y < Overlap.S) && (Y > General.S)) {	/* (1) */
			    csi = (General.E - X) / overlap;
			    eta = (Y - General.S) / overlap;
			    weight = csi * eta;
			    interpolation *= weight;
			    dval = interpolation;
			}
			else if ((Y >= Overlap.S) && (Y <= Overlap.N)) {	/* (1) */
			    weight = (General.E - X ) / overlap;
			    interpolation *= weight;
			    dval = interpolation;
			}
		    }
		    else if ((X < Overlap.W) && (X > General.W)) {
			if ((Y > Overlap.N) && (Y < General.N)) {	/* (4) */
			    csi = (X - General.W) / overlap;
			    eta = (General.N - Y) / overlap;
			    weight = eta * csi;
			    interpolation *= weight;
			    dval += interpolation;
			}
			else if ((Y < Overlap.S) && (Y > General.S)) {	/* (2) */
			    csi = (X - General.W) / overlap;
			    eta = (Y - General.S) / overlap;
			    weight = csi * eta;
			    interpolation *= weight;
			    dval += interpolation;
			}
			else if ((Y >= Overlap.S) && (Y <= Overlap.N)) {	/* (2) */
			    weight = (X - General.W) / overlap;
			    interpolation *= weight;
			    dval += interpolation;
			}
		    }
		    else if ((X >= Overlap.W) && (X <= Overlap.E)) {
			if ((Y > Overlap.N) && (Y < General.N)) {	/* (3) */
			    weight = (General.N - Y) / overlap;
			    interpolation *= weight;
			    dval += interpolation;
			}
			else if ((Y < Overlap.S) && (Y > General.S)) {	/* (1) */
			    weight = (Y - General.S) / overlap;
			    interpolation *= weight;
			    dval = interpolation;
			}
		    }
		}
		Segment_put(out_seg, &dval, row, col);
	    }
	}			/* END COL */
    }				/* END ROW */
    return 1;
}
