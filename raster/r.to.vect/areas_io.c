/* Cell-file area extraction */
/*   Input/output and line tracing routines */

/* Jean Ezell */
/* US Army Corps of Engineers */
/* Construction Engineering Research Lab */
/* Modelling and Simulation Team */
/* Champaign, IL  61820 */
/* November 1987 - January 1988 */

/* input is a raster map found in the normal GRASS way */
/* outputs are binary digit files and a supplemental area file */
/* to be used to improve the dlg labelling process */


#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include "global.h"


/* function prototypes */
static int write_bnd(struct COOR *, struct COOR *, int);
static int write_smooth_bnd(struct COOR *, struct COOR *, int);

static int *equivs;


/* write_line - attempt to write a line to output */
/* just returns if line is not completed yet */
int write_boundary(struct COOR *seed)
{
    struct COOR *point, *line_begin, *line_end;
    int dir, line_type, n, n1;

    point = seed;
    if ((dir = at_end(point))) {	/* already have one end of line */
	line_begin = point;
	line_end = find_end(point, dir, &line_type, &n);
	if (line_type == OPEN)
	    return (-1);	/* unfinished line */
	direction = dir;
    }
    else {			/* in middle of a line */
	line_end = find_end(point, FORWARD, &line_type, &n);
	if (line_type == OPEN)	/* line not finished */
	    return (-1);

	if (line_type == END) {	/* found one end at least *//* look for other one */
	    line_begin = find_end(point, BACKWARD, &line_type, &n1);
	    if (line_type == OPEN)	/* line not finished */
		return (-1);
	    if (line_type == LOOP) {	/* this should NEVER be the case */
		return (-1);
	    }
	    direction = at_end(line_begin);	/* found both ends now; total length */
	    n += n1;		/*   is sum of distances to each end */
	}
	else {
	    /* line_type = LOOP by default */
	    /* already have correct length */
	    line_begin = line_end;	/* end and beginning are the same */
	    direction = FORWARD;	/* direction is arbitrary */
	}
    }

    if (smooth_flag == SMOOTH)
	write_smooth_bnd(line_begin, line_end, n);
    else
	write_bnd(line_begin, line_end, n);

    return (0);
}


/* write_bnd - actual writing part of write_line */
/* writes binary and ASCII digit files and supplemental file */
static int write_bnd(struct COOR *line_begin, struct COOR *line_end,	/* start and end point of line */
		     int n	/* number of points to write */
    )
{
    static struct line_pnts *points = NULL;
    double x;
    double y;
    struct COOR *p, *last;
    int i;

    if (!points)
	points = Vect_new_line_struct();
    Vect_reset_line(points);

    n++;			/* %% 6.4.88 */

    p = line_begin;
    y = cell_head.north - (double)p->row * cell_head.ns_res;
    x = cell_head.west + (double)p->col * cell_head.ew_res;

    Vect_append_point(points, x, y, 0.0);

    for (i = 1; i < n; i++) {
	last = p;

	/* this should NEVER happen */
	if ((p = move(p)) == NULPTR)
	    G_fatal_error(_("Line terminated unexpectedly\n"
			    "previous (%d) point %p (%d,%d,%d) %p %p"),
			  direction, last, last->row, last->col, last->node,
			  last->fptr, last->bptr);

	y = cell_head.north - p->row * cell_head.ns_res;
	x = cell_head.west + p->col * cell_head.ew_res;

	Vect_append_point(points, x, y, 0.0);
    }

    /* now free all the pointers */
    p = line_begin;

    for (i = 1; i < n; i++) {
	last = p;
	if ((p = move(p)) == NULPTR)
	    break;
	if (last == p)
	    break;
	if (last->fptr != NULPTR)
	    if (last->fptr->fptr == last)
		last->fptr->fptr = NULPTR;
	/* it can be NULL after the previous line, even though before it wasn't */
	if (last->fptr != NULPTR)
	    if (last->fptr->bptr == last)
		last->fptr->bptr = NULPTR;
	if (last->bptr != NULPTR)
	    if (last->bptr->fptr == last)
		last->bptr->fptr = NULPTR;
	if (last->bptr != NULPTR)
	    if (last->bptr->bptr == last)
		last->bptr->bptr = NULPTR;

	G_free(last);
    }				/* end of for i */

    if (p != NULPTR)
	G_free(p);

    Vect_write_line(&Map, GV_BOUNDARY, points, Cats);

    return 0;
}


/* write_smooth_bnd - actual writing part of write_line for smoothed lines */
/* writes binary and ASCII digit files and supplemental file */
#define SNAP_THRESH 0.00001

static int write_smooth_bnd(struct COOR *line_begin, struct COOR *line_end,	/* start and end point of line */
			    int n	/* number of points to write */
    )
{
    static struct line_pnts *points = NULL;
    double x, y;
    double dx, dy;
    int idx, idy;
    struct COOR *p, *last;
    int i, total;

    if (!points)
	points = Vect_new_line_struct();
    Vect_reset_line(points);

    n++;			/* %% 6.4.88 */

    p = line_begin;
    /* allocate the arrays and get the first point */

    y = cell_head.north - (double)p->row * cell_head.ns_res;
    x = cell_head.west + (double)p->col * cell_head.ew_res;
    Vect_append_point(points, x, y, 0.0);

    /* generate the list of smoothed points, may be duplicate points */
    total = 1;
    for (i = 1; i < n; i++) {
	if (i < 10)
	    G_debug(3, " row: %d col: %d\n", p->row, p->col);

	last = p;
	if ((p = move(p)) == NULPTR) {	/* this should NEVER happen */
	    G_debug(3, "write_line:  line terminated unexpectedly\n");
	    G_debug(3, "  previous (%d) point %p (%d,%d,%d) %p %p\n",
		    direction, last, last->row, last->col, last->node,
		    last->fptr, last->bptr);

	    exit(EXIT_FAILURE);
	}

	idy = (p->row - last->row);
	idx = (p->col - last->col);
	dy = (idy > 0) ? 0.5 : ((idy < 0) ? -0.5 : 0.0);	/* dy = 0.0, 0.5, or -0.5 */
	dx = (idx > 0) ? 0.5 : ((idx < 0) ? -0.5 : 0.0);	/* dx = 0.0, 0.5, or -0.5 */
	y = cell_head.north - (last->row + dy) * cell_head.ns_res;
	x = cell_head.west + (last->col + dx) * cell_head.ew_res;
	total++;
	Vect_append_point(points, x, y, 0.0);

	y = cell_head.north - (p->row - dy) * cell_head.ns_res;
	x = cell_head.west + (p->col - dx) * cell_head.ew_res;
	total++;
	Vect_append_point(points, x, y, 0.0);

	/* G_free (last); */
    }				/* end of for i */

    y = cell_head.north - (double)p->row * cell_head.ns_res;
    x = cell_head.west + (double)p->col * cell_head.ew_res;
    total++;
    Vect_append_point(points, x, y, 0.0);

    /* strip out the duplicate points from the list */
    y = cell_head.north - (double)p->row * cell_head.ns_res;
    x = cell_head.west + (double)p->col * cell_head.ew_res;
    total++;
    Vect_append_point(points, x, y, 0.0);

    /* write files */
    Vect_write_line(&Map, GV_BOUNDARY, points, Cats);

    /* now free all thwe pointers */
    p = line_begin;

    for (i = 1; i < n; i++) {
	if (i < 10)
	    G_debug(3, " row: %d col: %d\n", p->row, p->col);

	last = p;
	if ((p = move(p)) == NULPTR)
	    break;
	if (last == p)
	    break;
	if (last->fptr != NULPTR)
	    if (last->fptr->fptr == last)
		last->fptr->fptr = NULPTR;

	/* now it can already ne NULL */
	if (last->fptr != NULPTR)
	    if (last->fptr->bptr == last)
		last->fptr->bptr = NULPTR;
	if (last->bptr != NULPTR)
	    if (last->bptr->fptr == last)
		last->bptr->fptr = NULPTR;
	if (last->bptr != NULPTR)
	    if (last->bptr->bptr == last)
		last->bptr->bptr = NULPTR;

	G_free(last);
    }				/* end of for i */

    if (p != NULPTR)
	G_free(p);

    return 0;
}

/* write_area - make table of area equivalences and write attribute file */
int write_area(struct area_table *a_list,	/* list of areas */
	       struct equiv_table *e_list,	/* list of equivalences between areas */
	       int n_areas,	/* lengths of e_list, a_list */
	       int n_equiv)
{
    struct line_pnts *points = Vect_new_line_struct();
    int n, i;
    struct area_table *p;
    char *temp_buf;
    int cat;
    int catNum;
    double x, y;

    total_areas = 0;
    if (n_equiv < n_areas) {
	equivs = (int *)G_malloc(n_areas * sizeof(int));
	n = n_equiv;
    }
    else {
	equivs = (int *)G_malloc(n_equiv * sizeof(int));
	n = n_areas;
    }

    for (i = 0; i < n; i++) {
	if ((e_list + i)->mapped)
	    equivs[i] = (e_list + i)->where;
	else {
	    total_areas++;
	    equivs[i] = i;
	}
    }

    if (n < n_areas) {
	for (i = n; i < n_areas; i++) {
	    total_areas++;
	    equivs[i] = i;
	}
    }

    catNum = 1;

    G_important_message(_("Writing areas..."));
    for (i = 0, p = a_list; i < n_areas; i++, p++) {
	if (equivs[i] == i && p->width > 0 && !Rast_is_d_null_value(&(p->cat))) {
	    char buf[1000];

	    if (value_flag) {	/* raster value */
		cat = (int)p->cat;
	    }
	    else {		/* sequence */
		cat = catNum;
		catNum++;
	    }

            G_percent(i, n_areas, 3);
            
	    x = cell_head.west + (p->col +
				  (p->width / 2.0)) * cell_head.ew_res;
	    y = cell_head.north - (p->row + 0.5) * cell_head.ns_res;

	    switch (data_type) {
	    case CELL_TYPE:
		G_debug(3,
			"vector x = %.3f, y = %.3f, cat = %d; raster cat = %d",
			x, y, cat, (int)p->cat);
		break;
	    case FCELL_TYPE:
		G_debug(3,
			"vector x = %.3f, y = %.3f, cat = %d; raster cat = %f",
			x, y, cat, (float)p->cat);
		break;
	    case DCELL_TYPE:
		G_debug(3,
			"vector x = %.3f, y = %.3f, cat = %d; raster cat = %lf",
			x, y, cat, p->cat);
		break;
	    }

	    Vect_reset_line(points);
	    Vect_append_point(points, x, y, 0.0);

	    Vect_reset_cats(Cats);
	    Vect_cat_set(Cats, 1, cat);

	    Vect_write_line(&Map, GV_CENTROID, points, Cats);

	    if (driver != NULL && !value_flag) {
		sprintf(buf, "insert into %s values (%d, ", Fi->table, cat);
		db_set_string(&sql, buf);
		switch (data_type) {
		case CELL_TYPE:
		    sprintf(buf, "%d", (int)p->cat);
		    break;
		case FCELL_TYPE:
		case DCELL_TYPE:
		    sprintf(buf, "%f", p->cat);
		    break;
		}
		db_append_string(&sql, buf);

		if (has_cats) {
		    temp_buf = Rast_get_d_cat(&p->cat, &RastCats);

		    db_set_string(&label, temp_buf);
		    db_double_quote_string(&label);
		    sprintf(buf, ", '%s'", db_get_string(&label));
		    db_append_string(&sql, buf);
		}

		db_append_string(&sql, ")");
		G_debug(3, db_get_string(&sql));

		if (db_execute_immediate(driver, &sql) != DB_OK)
		    G_fatal_error(_("Cannot insert new row: %s"),
				  db_get_string(&sql));
	    }
	}
    }
    G_percent(1, 1, 1);
    
    return 0;
}
