#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local_proto.h"


/* function prototypes */
static void cpvalue(struct RASTER_MAP_PTR *, int, struct RASTER_MAP_PTR *,
		    int);
static double cell_as_dbl(struct RASTER_MAP_PTR *, int);
static void set_to_null(struct RASTER_MAP_PTR *, int);


int execute_random(struct rr_state *theState)
{
    long nt;
    long nc;
    struct Cell_head window;
    int nrows, ncols, row, col;
    int infd, cinfd, outfd;
    struct Map_info Out;
    struct field_info *fi;
    dbTable *table;
    dbColumn *column;
    dbString sql;
    dbDriver *driver;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int cat;
    RASTER_MAP_TYPE type;
    int do_check;

    G_get_window(&window);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* open the data files, input raster should be set-up already */
    if ((infd = theState->fd_old) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"),
		      theState->inraster);
    if (theState->docover == TRUE) {
	if ((cinfd = theState->fd_cold) < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  theState->inrcover);
    }

    if (theState->outraster != NULL) {
	if (theState->docover == TRUE)
	    type = theState->cover.type;
	else
	    type = theState->buf.type;
	outfd = Rast_open_new(theState->outraster, type);
	theState->fd_new = outfd;

    }

    if (theState->outvector) {
	if (theState->z_geometry)
	    Vect_open_new(&Out, theState->outvector, 1);
	else
	    Vect_open_new(&Out, theState->outvector, 0);
	Vect_hist_command(&Out);

	fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);

	driver =
	    db_start_driver_open_database(fi->driver,
					  Vect_subst_var(fi->database, &Out));
	if (!driver)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Vect_subst_var(fi->database, &Out), fi->driver);

	Vect_map_add_dblink(&Out, 1, NULL, fi->table, GV_KEY_COLUMN, fi->database,
			    fi->driver);

	if (theState->docover == TRUE)
	    table = db_alloc_table(3);
	else
	    table = db_alloc_table(2);
	db_set_table_name(table, fi->table);

	column = db_get_table_column(table, 0);
	db_set_column_name(column, GV_KEY_COLUMN);
	db_set_column_sqltype(column, DB_SQL_TYPE_INTEGER);

	column = db_get_table_column(table, 1);
	db_set_column_name(column, "value");
	db_set_column_sqltype(column, DB_SQL_TYPE_DOUBLE_PRECISION);

	if (theState->docover == TRUE) {
	    column = db_get_table_column(table, 2);
	    db_set_column_name(column, "covervalue");
	    db_set_column_sqltype(column, DB_SQL_TYPE_DOUBLE_PRECISION);
	}
	if (db_create_table(driver, table) != DB_OK)
	    G_warning(_("Cannot create new table"));

	db_begin_transaction(driver);

	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
	db_init_string(&sql);
    }

    if (theState->outvector && theState->outraster)
	G_message(_("Writing raster map <%s> and vector map <%s> ..."),
		  theState->outraster, theState->outvector);
    else if (theState->outraster)
	G_message(_("Writing raster map <%s> ..."), theState->outraster);
    else if (theState->outvector)
	G_message(_("Writing vector map <%s> ..."), theState->outvector);

    G_percent(0, theState->nRand, 2);

    init_rand();
    nc = (theState->use_nulls) ? theState->nCells :
	theState->nCells - theState->nNulls;
    nt = theState->nRand;	/* Number of points to generate */
    cat = 1;

    /* Execute for loop for every row if nt>1 */
    for (row = 0; row < nrows && nt; row++) {
	Rast_get_row(infd, theState->buf.data.v, row, theState->buf.type);
	if (theState->docover == TRUE) {
	    Rast_get_row(cinfd, theState->cover.data.v, row,
			 theState->cover.type);
	}

	for (col = 0; col < ncols && nt; col++) {
	    do_check = 0;

	    if (theState->use_nulls || !is_null_value(theState->buf, col))
		do_check = 1;
	    if (do_check && theState->docover == TRUE) {	/* skip no data cover points */
		if (!theState->use_nulls &&
		    is_null_value(theState->cover, col))
		    do_check = 0;
	    }

	    if (do_check && make_rand() % nc < nt) {
		nt--;
		if (is_null_value(theState->buf, col))
		    cpvalue(&theState->nulls, 0, &theState->buf, col);
		if (theState->docover == TRUE) {
		    if (is_null_value(theState->cover, col))
			cpvalue(&theState->cnulls, 0, &theState->cover, col);
		}

		if (theState->outvector) {
		    double x, y, val, coverval;
		    char buf[500];

		    Vect_reset_line(Points);
		    Vect_reset_cats(Cats);

		    x = window.west + (col + .5) * window.ew_res;
		    y = window.north - (row + .5) * window.ns_res;

		    val = cell_as_dbl(&theState->buf, col);
		    if (theState->docover == 1)
			coverval = cell_as_dbl(&theState->cover, col);

		    if (theState->z_geometry)
			Vect_append_point(Points, x, y, val);
		    else
			Vect_append_point(Points, x, y, 0.0);
		    Vect_cat_set(Cats, 1, cat);

		    Vect_write_line(&Out, GV_POINT, Points, Cats);

		    if (theState->docover == 1)
			if (is_null_value(theState->cover, col))
			    sprintf(buf,
				    "insert into %s values ( %d, %f, NULL )",
				    fi->table, cat, val);
			else
			    sprintf(buf,
				    "insert into %s values ( %d, %f, %f )",
				    fi->table, cat, val, coverval);
		    else
			sprintf(buf, "insert into %s values ( %d, %f )",
				fi->table, cat, val);
		    db_set_string(&sql, buf);

		    if (db_execute_immediate(driver, &sql) != DB_OK)
			G_fatal_error(_("Cannot insert new record: %s"),
				      db_get_string(&sql));

		    cat++;
		}
		G_percent((theState->nRand - nt), theState->nRand, 2);
	    }
	    else {
		set_to_null(&theState->buf, col);
		if (theState->docover == 1)
		    set_to_null(&theState->cover, col);
	    }

	    if (do_check)
		nc--;
	}

	while (col < ncols) {
	    set_to_null(&theState->buf, col);
	    if (theState->docover == 1)
		set_to_null(&theState->cover, col);
	    col++;
	}

	if (theState->outraster) {
	    if (theState->docover == 1)
		Rast_put_row(outfd, theState->cover.data.v,
				 theState->cover.type);
	    else
		Rast_put_row(outfd, theState->buf.data.v,
				 theState->buf.type);
	}
    }

    /* Catch any remaining rows in the window */
    if (theState->outraster && row < nrows) {
	for (col = 0; col < ncols; col++) {
	    if (theState->docover == 1)
		set_to_null(&theState->cover, col);
	    else
		set_to_null(&theState->buf, col);
	}
	for (; row < nrows; row++) {
	    if (theState->docover == 1)
		Rast_put_row(outfd, theState->cover.data.v,
				 theState->cover.type);
	    else
		Rast_put_row(outfd, theState->buf.data.v,
				 theState->buf.type);
	}
    }

    if (nt > 0)
	G_warning(_("Only [%ld] random points created"),
		  theState->nRand - nt);

    /* close files */
    Rast_close(infd);
    if (theState->docover == TRUE)
	Rast_close(cinfd);
    if (theState->outvector) {
	db_commit_transaction(driver);
	db_close_database_shutdown_driver(driver);
	if (theState->notopol != 1)
	    Vect_build(&Out);
	Vect_close(&Out);
    }
    if (theState->outraster)
	Rast_close(outfd);

    return 0;
}				/* execute_random() */


static void cpvalue(struct RASTER_MAP_PTR *from, int fcol,
		    struct RASTER_MAP_PTR *to, int tcol)
{
    switch (from->type) {
    case CELL_TYPE:
	to->data.c[tcol] = from->data.c[fcol];
	break;
    case FCELL_TYPE:
	to->data.f[tcol] = from->data.f[fcol];
	break;
    case DCELL_TYPE:
	to->data.d[tcol] = from->data.d[fcol];
	break;
    }
}


static double cell_as_dbl(struct RASTER_MAP_PTR *buf, int col)
{
    switch (buf->type) {
    case CELL_TYPE:
	return (double)buf->data.c[col];
    case FCELL_TYPE:
	return (double)buf->data.f[col];
    case DCELL_TYPE:
	return (double)buf->data.d[col];
    }

    return 0.;
}


static void set_to_null(struct RASTER_MAP_PTR *buf, int col)
{
    switch (buf->type) {
    case CELL_TYPE:
	Rast_set_c_null_value(&(buf->data.c[col]), 1);
	break;
    case FCELL_TYPE:
	Rast_set_f_null_value(&(buf->data.f[col]), 1);
	break;
    case DCELL_TYPE:
	Rast_set_d_null_value(&(buf->data.d[col]), 1);
	break;
    }
}


int is_null_value(struct RASTER_MAP_PTR buf, int col)
{
    switch (buf.type) {
    case CELL_TYPE:
	return Rast_is_c_null_value(&buf.data.c[col]);
	break;
    case FCELL_TYPE:
	return Rast_is_f_null_value(&buf.data.f[col]);
	break;
    case DCELL_TYPE:
	return Rast_is_d_null_value(&buf.data.d[col]);
	break;
    }

    return -1;
}
