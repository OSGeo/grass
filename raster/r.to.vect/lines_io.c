/* Cell-file line extraction */
/*   Input/output and line tracing routines */

/* Mike Baba */
/* DBA Systems */
/* Farfax, VA */
/* Jan 1990 */

/* Jean Ezell */
/* US Army Corps of Engineers */
/* Construction Engineering Research Lab */
/* Modelling and Simulation Team */
/* Champaign, IL  61820 */
/* March 1988 */

/* input is a GRASS raster map */
/* output is a binary or ascii digit file */

/*
 * Modified for the new Grass 5.0 floating point and
 * null values raster map format.
 * Pierre de Mouveaux - 20 april 2000.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __MINGW32__
#include <process.h>
#else
#include <sys/wait.h>
#endif

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include "global.h"


/* function prototypes */
static int write_ln(struct COOR *, struct COOR *, int);


/* write_line - attempt to write a line to output */
/* just returns if line is not completed yet */

int write_line(struct COOR *seed)
{
    struct COOR *point, *begin, *end;
    int dir, line_type, n, n1;

    point = seed;
    if ((dir = at_end(point))) {	/* already have one end of line */
	begin = point;
	end = find_end(point, dir, &line_type, &n);
	if (line_type == OPEN) {
	    return (-1);	/* unfinished line */
	}
	direction = dir;
    }
    else {			/* in middle of a line */
	end = find_end(point, FORWARD, &line_type, &n);
	if (line_type == OPEN) {	/* line not finished */
	    return (-1);
	}

	if (line_type == END) {	/* found one end at least *//* look for other one */
	    begin = find_end(point, BACKWARD, &line_type, &n1);
	    if (line_type == OPEN) {	/* line not finished */
		return (-1);
	    }

	    if (line_type == LOOP) {	/* this should NEVER be the case */
		G_warning(_("write_line:  found half a loop!"));
		return (-1);
	    }

	    direction = at_end(begin);	/* found both ends now; total length */
	    n += n1;		/*   is sum of distances to each end */
	}
	else {			/* line_type = LOOP by default */
	    /* already have correct length */
	    begin = end;	/* end and beginning are the same */
	    direction = FORWARD;	/* direction is arbitrary */
	}
    }

    /* if (n > 2) */
    write_ln(begin, end, n);

    return (0);
}

/* write_ln - actual writing part of write_line */
/* writes binary and supplemental file */

static int write_ln(struct COOR *begin, struct COOR *end,	/* start and end point of line */
		    int n)
{				/* number of points to write */
    static struct line_pnts *points = NULL;
    double x, y;
    struct COOR *p, *last;
    int i, cat, field;
    static int count = 1;

    if (!points)
	points = Vect_new_line_struct();
    Vect_reset_line(points);

    field = 1;
    ++n;

    Vect_reset_cats(Cats);

    p = begin;
    y = cell_head.north - ((double)p->row + 0.5) * cell_head.ns_res;
    x = cell_head.west + ((double)p->col + 0.5) * cell_head.ew_res;

    /* value_flag is used only for CELL type */
    cat = (value_flag) ? p->val : count;

    Vect_cat_set(Cats, field, cat);
    Vect_append_point(points, x, y, 0.0);

    for (i = 1; i < n; i++) {
	last = p;

	/* this should NEVER happen */
	if ((p = move(p)) == NULL)
	    G_fatal_error(_("write_line: line terminated unexpectedly\n"
			    "  previous (%d) point %p (%d,%d,%d) %p %p"),
			  direction, last, last->row, last->col, last->node,
			  last->fptr, last->bptr);

	y = cell_head.north - ((double)p->row + 0.5) * cell_head.ns_res;
	x = cell_head.west + ((double)p->col + 0.5) * cell_head.ew_res;

	if (p->val != cat && value_flag) {
	    Vect_append_point(points, x, y, 0.0);

	    if ((driver != NULL) && !value_flag) {
		insert_value(cat, p->val, p->dval);
	    }

	    Vect_write_line(&Map, GV_LINE, points, Cats);
	    Vect_reset_line(points);
	    Vect_reset_cats(Cats);
	    cat = (value_flag) ? p->val : (++count);
	    Vect_cat_set(Cats, field, cat);
	}

	Vect_append_point(points, x, y, 0.0);
    }

    if ((driver != NULL) && !value_flag) {
	insert_value(cat, p->val, p->dval);
    }

    Vect_write_line(&Map, GV_LINE, points, Cats);

    count++;

    /* now free all the pointers */
    p = begin;

    for (i = 1; i < n; i++) {
	last = p;

	if ((p = move(p)) == NULL)
	    break;
	if (last == p)
	    break;
	if (last->fptr != NULL)
	    if (last->fptr->fptr == last)
		last->fptr->fptr = NULL;

	/* now it can already ne NULL */
	if (last->fptr != NULL)
	    if (last->fptr->bptr == last)
		last->fptr->bptr = NULL;
	if (last->bptr != NULL)
	    if (last->bptr->fptr == last)
		last->bptr->fptr = NULL;
	if (last->bptr != NULL)
	    if (last->bptr->bptr == last)
		last->bptr->bptr = NULL;
	G_free(last);
    }				/* end of for i */

    if (p != NULL)
	G_free(p);

    return 0;
}
