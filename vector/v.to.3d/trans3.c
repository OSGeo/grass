#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

static int srch(const void *, const void *);

/*!
   \brief transform 3d vector features to 2d (z-coordinate is omitted)

   \param In input vector
   \param Out output vector
   \param type feature type to be transformed
   \param field layer number
   \param zcolumn attribute column where to store height

   \return number of writen features
 */
int trans3d(struct Map_info *In, struct Map_info *Out, int type,
	    int field, const char *zcolumn)
{
    int ltype;
    int line;
    int ctype;

    struct line_pnts *Points;
    struct line_cats *Cats;

    struct field_info *Fi;
    dbDriver *driver;
    dbString stmt;
    char buf[2000];
    int ncats, *cats, cat, *cex;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    db_init_string(&stmt);

    if (zcolumn) {
	Fi = Vect_get_field(Out, field);
	if (!Fi)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  field);

	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (!driver) {
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
	}

	/* column type must numeric */
	ctype = db_column_Ctype(driver, Fi->table, zcolumn);
	if (ctype == -1)
	    G_fatal_error(_("Column <%s> not found in table <%s>"),
			  zcolumn, Fi->table);
	if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE) {
	    G_fatal_error(_("Column must be numeric"));
	}

	db_begin_transaction(driver);

	/* select existing categories (layer) to array (array is sorted) */
	ncats = db_select_int(driver, Fi->table, Fi->key, NULL, &cats);
	G_debug(3, "Existing categories: %d", ncats);
    }

    line = 1;
    while (1) {
	ltype = Vect_read_next_line(In, Points, Cats);
	if (ltype == -1) {
	    G_fatal_error(_("Unable to read vector map"));
	}
	if (ltype == -2) {	/* EOF */
	    break;
	}

	if (G_verbose() > G_verbose_min() && (line - 1) % 1000 == 0) {
	    fprintf(stderr, "%7d\b\b\b\b\b\b\b", (line - 1));
	}

	if (!(ltype & type))
	    continue;

	Vect_cat_get(Cats, field, &cat);

	/* get first cat */
	if (cat == -1) {
	    G_warning(_("Feature id %d has no category - skipping"), line);
	}
	else if (Cats->n_cats > 1) {
	    G_warning(_("Feature id %d has more categories. "
			"Using category %d."), line, field, cat);
	}

	if (zcolumn && ltype == GV_POINT && cat > -1) {
	    /* category exist in table ? */
	    cex = (int *)bsearch((void *)&cat, cats, ncats, sizeof(int),
				 srch);

	    /* store height to the attribute table */
	    if (ctype == DB_C_TYPE_INT)
		sprintf(buf, "update %s set %s = %d where cat = %d",
			Fi->table, zcolumn, (int)Points->z[0], cat);
	    else		/* double */
		sprintf(buf, "update %s set %s = %.8f where cat = %d",
			Fi->table, zcolumn, Points->z[0], cat);

	    G_debug(3, "SQL: %s", buf);
	    db_set_string(&stmt, buf);

	    if (cex) {
		if (db_execute_immediate(driver, &stmt) == DB_OK) {
		    /* TODO */
		}
	    }
	    else {		/* cat does not exist in table */
		G_warning(_("Record (cat %d) does not exist (not updated)"),
			  cat);
	    }
	}

	Vect_write_line(Out, ltype, Points, Cats);
	line++;
    }

    if (G_verbose() > G_verbose_min())
	fprintf(stderr, "\r");

    if (zcolumn) {
	db_commit_transaction(driver);

	G_free(cats);

	db_close_database_shutdown_driver(driver);
	db_free_string(&stmt);
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return line - 1;
}

int srch(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;

    int *p2 = (int *)pb;

    if (*p1 < *p2)
	return -1;
    if (*p1 > *p2)
	return 1;
    return 0;
}
