#include <math.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

static struct line_cats *PCats;
static struct line_pnts *PPoints;
static int point_cat = 1;

void write_point(struct Map_info *Out, double x, double y, double z,
		 int line_cat, double along, dbDriver *driver, struct field_info *Fi)
{
    G_debug(3, "write_point()");

    if (!PPoints) {
        PCats = Vect_new_cats_struct();
        PPoints = Vect_new_line_struct();
    }
    else {
        Vect_reset_line(PPoints);
        Vect_reset_cats(PCats);
    }
    
    /* Write point */
    Vect_append_point(PPoints, x, y, z);
    if (line_cat > 0) {
        Vect_cat_set(PCats, 1, line_cat);
        Vect_cat_set(PCats, 2, point_cat);
    }
    else {
        Vect_cat_set(PCats, 2, point_cat);
    }
    Vect_write_line(Out, GV_POINT, PPoints, PCats);

    /* Attributes */
    if (Fi) {
        char buf[DB_SQL_MAX];
        dbString stmt;
        
	db_init_string(&stmt);
        if (line_cat > 0)
            sprintf(buf, "insert into %s values ( %d, %d, %.15g )", Fi->table,
                point_cat, line_cat, along);
        else
            sprintf(buf, "insert into %s values ( %d, %.15g )", Fi->table,
                point_cat, along);
	db_append_string(&stmt, buf);

	if (db_execute_immediate(driver, &stmt) != DB_OK) {
	    G_warning(_("Unable to insert new record: '%s'"),
		      db_get_string(&stmt));
	}
    }
    point_cat++;
}

void write_line(struct Map_info *Out, struct line_pnts *LPoints, int cat,
		int vertex, int interpolate, double dmax,
                dbDriver *driver, struct field_info *Fi)
{
    if (vertex == GV_VERTEX || vertex == GV_NODE) {	/* use line vertices */
	double along;
	int vert;

	along = 0;
	for (vert = 0; vert < LPoints->n_points; vert++) {
	    G_debug(3, "vert = %d", vert);

	    if (vertex == GV_VERTEX ||
		(vertex == GV_NODE &&
		 (vert == 0 || vert == LPoints->n_points - 1))) {
		write_point(Out, LPoints->x[vert], LPoints->y[vert],
			    LPoints->z[vert], cat, along, driver, Fi);
	    }

	    if (vert < LPoints->n_points - 1) {
		double dx, dy, dz, len;

		dx = LPoints->x[vert + 1] - LPoints->x[vert];
		dy = LPoints->y[vert + 1] - LPoints->y[vert];
		dz = LPoints->z[vert + 1] - LPoints->z[vert];
		len = hypot(hypot(dx, dy), dz);

		/* interpolate segment */
		if (interpolate && vert < (LPoints->n_points - 1)) {
		    int i, n;
		    double x, y, z, dlen;

		    if (len > dmax) {
			n = len / dmax + 1;	/* number of segments */
			dx /= n;
			dy /= n;
			dz /= n;
			dlen = len / n;

			for (i = 1; i < n; i++) {
			    x = LPoints->x[vert] + i * dx;
			    y = LPoints->y[vert] + i * dy;
			    z = LPoints->z[vert] + i * dz;

			    write_point(Out, x, y, z, cat, along + i * dlen,
					driver, Fi);
			}
		    }
		}
		along += len;
	    }
	}
    }
    else {			/* do not use vertices */
	int i, n;
	double len, dlen, along, x, y, z;

	len = Vect_line_length(LPoints);
	n = len / dmax + 1;	/* number of segments */
	dlen = len / n;		/* length of segment */

	G_debug(3, "n = %d len = %f dlen = %f", n, len, dlen);

	for (i = 0; i <= n; i++) {
	    if (i > 0 && i < n) {
		along = i * dlen;
		Vect_point_on_line(LPoints, along, &x, &y, &z, NULL, NULL);
	    }
	    else {		/* first and last vertex */
		if (i == 0) {
		    along = 0;
		    x = LPoints->x[0];
		    y = LPoints->y[0];
		    z = LPoints->z[0];
		}
		else {		/* last */
		    along = len;
		    x = LPoints->x[LPoints->n_points - 1];
		    y = LPoints->y[LPoints->n_points - 1];
		    z = LPoints->z[LPoints->n_points - 1];
		}
	    }
	    G_debug(3, "  i = %d along = %f", i, along);
	    write_point(Out, x, y, z, cat, along, driver, Fi);
	}
    }
}
