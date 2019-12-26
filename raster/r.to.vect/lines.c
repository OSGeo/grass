/* -*-c-basic-offset: 4-*-
 * Cell-file line extraction
 *   Line-tracing algorithm
 *
 * Mike Baba 
 * DBA Systems
 * Farfax, VA 
 * Jan 1990 
 *
 * Jean Ezell
 * US Army Corps of Engineers
 * Construction Engineering Research Lab
 * Modeling and Simulation Team
 * Champaign, IL  61820
 * March 1988
 *
 * Algorithm was modified by Olga Waupotitsch
 * USA CERL on nov, 1993
 * because the previous implementation was inconsistent
 * stopped in the middle of map, because it tried to continue
 * a line which was presumed to have been started earlier
 * but in fact was not started.
 * also the write_line() complained that the lines end unexpectedly
 *
 * After modification all these problems are gone
 *
 * Modified for the new Grass 5.0 floating point and
 * null values raster map format.
 * Pierre de Mouveaux - 20 april 2000.
 */
#include <stdio.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include "global.h"


static struct line_hdr *v_list;
static struct COOR *h_ptr;
static void *top, *middle, *bottom;
static int tl, tc, tr, ml, mc, mr, bl, bc, br;
static int row, col, n_cols;

extern int data_type;
extern int data_size;

/* function prototypes */
static int join_lines(struct COOR *p, struct COOR *q);
static int extend_line(struct COOR *p, struct COOR *q);

#if 0
static int stop_line(struct COOR *p, struct COOR *q);
#endif
static int read_next(void);
static int nabors(void);
static int update_list(int);
static struct COOR *end_line(struct COOR *, int);
static struct COOR *start_line(int);
static struct COOR *get_ptr(void);


int extract_lines(void)
{
    n_alloced_ptrs = 0;
    row = -3;
    read_next();
    read_next();

    G_message(_("Extracting lines..."));

    switch (data_type) {
    case CELL_TYPE:
	{
	    int rows = 1;

	    while (read_next()) {
		CELL *m = &((CELL *) middle)[1];
		CELL *t = &((CELL *) top)[1];
		CELL *b = &((CELL *) bottom)[1];

		G_percent(rows, n_rows, 2);

		for (col = 1; col < n_cols - 1; col++, t++, m++, b++) {
		    m = &((CELL *) middle)[col];
		    t = &((CELL *) top)[col];
		    b = &((CELL *) bottom)[col];

		    if ((mc = !Rast_is_c_null_value(m))) {
			tl = !Rast_is_c_null_value(t - 1);
			tc = !Rast_is_c_null_value(t);
			tr = !Rast_is_c_null_value(t + 1);
			ml = !Rast_is_c_null_value(m - 1);
			mr = !Rast_is_c_null_value(m + 1);
			bl = !Rast_is_c_null_value(b - 1);
			bc = !Rast_is_c_null_value(b);
			br = !Rast_is_c_null_value(b + 1);
			update_list(nabors());
		    }
		}

		rows++;
	    }

	    G_percent(rows, n_rows, 2);
	    break;
	}
    case FCELL_TYPE:
	{
	    int rows = 1;

	    while (read_next()) {
		FCELL *m = &((FCELL *) middle)[1];
		FCELL *t = &((FCELL *) top)[1];
		FCELL *b = &((FCELL *) bottom)[1];

		G_percent(rows, n_rows, 2);

		for (col = 1; col < n_cols - 1; col++, t++, m++, b++) {
		    m = &((FCELL *) middle)[col];
		    t = &((FCELL *) top)[col];
		    b = &((FCELL *) bottom)[col];

		    if ((mc = !Rast_is_f_null_value(m))) {
			tl = !Rast_is_f_null_value(t - 1);
			tc = !Rast_is_f_null_value(t);
			tr = !Rast_is_f_null_value(t + 1);
			ml = !Rast_is_f_null_value(m - 1);
			mr = !Rast_is_f_null_value(m + 1);
			bl = !Rast_is_f_null_value(b - 1);
			bc = !Rast_is_f_null_value(b);
			br = !Rast_is_f_null_value(b + 1);
			update_list(nabors());
		    }
		}

		rows++;
	    }

	    G_percent(rows, n_rows, 2);
	    break;
	}
    case DCELL_TYPE:
	{
	    int rows = 1;

	    while (read_next()) {
		DCELL *m = &((DCELL *) middle)[1];
		DCELL *t = &((DCELL *) top)[1];
		DCELL *b = &((DCELL *) bottom)[1];

		G_percent(rows, n_rows, 2);

		for (col = 1; col < n_cols - 1; col++, t++, m++, b++) {
		    m = &((DCELL *) middle)[col];
		    t = &((DCELL *) top)[col];
		    b = &((DCELL *) bottom)[col];
		    if ((mc = !Rast_is_d_null_value(m))) {
			tl = !Rast_is_d_null_value(t - 1);
			tc = !Rast_is_d_null_value(t);
			tr = !Rast_is_d_null_value(t + 1);
			ml = !Rast_is_d_null_value(m - 1);
			mr = !Rast_is_d_null_value(m + 1);
			bl = !Rast_is_d_null_value(b - 1);
			bc = !Rast_is_d_null_value(b);
			br = !Rast_is_d_null_value(b + 1);
			update_list(nabors());
		    }
		}

		rows++;
	    }

	    G_percent(rows, n_rows, 2);
	    break;
	}
    }

    G_free(top);
    G_free(middle);
    G_free(bottom);
    G_free(v_list);

    if (n_alloced_ptrs) {
	/* should not happen */
	G_warning("Memory leak: %d points are still in use", n_alloced_ptrs);
    }

    return 0;
}

static int nabors(void)
{
    int count = 0;

    if (tl)
	count++;
    if (tc)
	count++;
    if (tr)
	count++;
    if (mr)
	count++;
    if (br)
	count++;
    if (bc)
	count++;
    if (bl)
	count++;
    if (ml)
	count++;

    return (count);
}

static int update_list(int count)
{
    struct COOR *new_ptr1, *new_ptr2, *new_ptr3;

    G_debug(3, "update_list: count:%d row:%d col:%d", count, row, col - 1);

    switch (count) {
    case 0:
	G_debug(1, "Isolated cell (%d,%d)", row, col);
	break;
    case 1:			/* begin or end line */
	if (ml)
	    h_ptr = end_line(h_ptr, 0);
	if (tl)
	    v_list[col].left = end_line(v_list[col].left, 0);
	if (tc)
	    v_list[col].center = end_line(v_list[col].center, 0);
	if (tr)
	    v_list[col].right = end_line(v_list[col].right, 0);
	if (mr)
	    h_ptr = start_line(0);
	if (br)
	    v_list[col + 1].left = start_line(0);
	if (bc)
	    v_list[col].center = start_line(0);
	if (bl)
	    v_list[col - 1].right = start_line(0);
	break;
    case 2:			/* straight or bent line */
	if (tl != 0 && br != 0) {	/* slanted line (\) */
		if (value_flag) {
			/* only CELL supported */
			if (data_type == CELL_TYPE) {
				int mc_val = ((CELL *) middle)[col];
				int br_val = ((CELL *) bottom)[col + 1];
				int tl_val = ((CELL *) top)[col - 1];
				if (tl_val != mc_val) {
					v_list[col].left = end_line(v_list[col].left, 1);
					v_list[col].left = start_line(0);
				}
				if (mc_val != br_val) {
					v_list[col].left = end_line(v_list[col].left, 1);
					v_list[col].left = start_line(0);
				}
			}
		}
	    v_list[col + 1].left = v_list[col].left;
	    v_list[col].left = NULL;
	}
	else if (tr != 0 && bl != 0) {	/* slanted line (/) */
		if (value_flag) {
			/* only CELL supported */
			if (data_type == CELL_TYPE) {
				int mc_val = ((CELL *) middle)[col];
				int bl_val = ((CELL *) bottom)[col - 1];
				int tr_val = ((CELL *) top)[col + 1];
				if (tr_val != mc_val) {
					v_list[col].right = end_line(v_list[col].right, 1);
					v_list[col].right = start_line(0);
				}
				if (mc_val != bl_val) {
					v_list[col].right = end_line(v_list[col].right, 1);
					v_list[col].right = start_line(0);
				}
			}
		}
	    v_list[col - 1].right = v_list[col].right;
	    v_list[col].right = NULL;
	}

	/* first take care of the cases where both non-zero
	   neighbours are in a upper-left corner (cw from ml to tr) */
	else if (ml != 0 && tc != 0) {	/* bend (_|) */
	    join_lines(h_ptr, v_list[col].center);
	    h_ptr = v_list[col].center = NULL;
	}
	else if (ml != 0 && tr != 0) {	/* bend (_/) */
	    join_lines(h_ptr, v_list[col].right);
	    h_ptr = v_list[col].left = NULL;
	}
	else if (tl != 0 && tr != 0) {	/* bend (\/) */
	    join_lines(v_list[col].left, v_list[col].right);
	    v_list[col].left = v_list[col].right = NULL;
	}
	else if (tl != 0 && tc != 0)	/* bend (\|) */
	    v_list[col].center = end_line(v_list[col].center, 1);
	else if (tr != 0 && tc != 0)	/* bend |/ */
	    v_list[col].center = end_line(v_list[col].center, 1);
	else if (tl != 0 && ml != 0)
	    h_ptr = end_line(h_ptr, 1);

	/* now take care of the cases when non-zero neighbours
	   are next to nonzero neighbours in a top-left corner */
	else if (bl != 0 && ml != 0)
	    v_list[col].center = start_line(1);
	else if (tr != 0 && mr != 0)
	    h_ptr = start_line(1);
	else if (!((tc != 0 && bc != 0) || (ml != 0 && mr != 0)))
	    /* if not horiz or vertical line */
	{
	    /* one of the non zero neighbours is in the top left corner,
	       and the other one is one of bl - mr, not next to the first one */
	    if (ml || tl || tc || tr) {	/* old line bends toward *//*   new area */
		new_ptr1 = get_ptr();

		if (ml) {	/* join new to where came from */
		    if (h_ptr == NULL)
			G_debug(1, "h_ptr is NULL!");

		    /* this should never happen by the logic of algorithm */
		    extend_line(h_ptr, new_ptr1);
		    h_ptr = NULL;
		}
		else if (tl) {
		    if (v_list[col].left == NULL)
			G_debug(1, "v_list[col].left is NULL!");

		    /* this should never happen by the logic of algorithm */
		    extend_line(v_list[col].left, new_ptr1);
		    v_list[col].left = NULL;
		}
		else if (tc) {
		    if (v_list[col].center == NULL)
			G_debug(1, "v_list[col].center is NULL!");

		    /* this should never happen by the logic of algorithm */
		    extend_line(v_list[col].center, new_ptr1);
		    v_list[col].center = NULL;
		}
		else {		/* tr */

		    if (v_list[col].right == NULL)
			G_debug(1, "v_list[col].right is NULL!");

		    /* this should never happen by the logic of algorithm */
		    extend_line(v_list[col].right, new_ptr1);
		    v_list[col].right = NULL;
		}

		if (mr)
		    /* find out where going */
		    /* tr is 0 here */
		    h_ptr = new_ptr1;
		else if (br)
		    v_list[col + 1].left = new_ptr1;
		else if (bc)
		    v_list[col].center = new_ptr1;
		else		/* bl, ml is 0 here */
		    v_list[col - 1].right = new_ptr1;
	    }
	    else {		/* lower-left */
		/* if the non-zero neighbors are adjacent */
		if (mr && br)
		    h_ptr = start_line(1);
		else if ((br && bc) || (bl && bc))
		    v_list[col].center = start_line(1);
		else
		    /* the non-zero neighbors are not adjacent */
		{		/* starting in middle of line */
		    new_ptr1 = get_ptr();
		    new_ptr2 = get_ptr();
		    new_ptr3 = get_ptr();
		    new_ptr1->fptr = new_ptr2;
		    new_ptr1->bptr = new_ptr3;
		    new_ptr3->bptr = new_ptr2->bptr = new_ptr1;

		    if (mr && bc) {
			h_ptr = new_ptr2;
			v_list[col].center = new_ptr3;
		    }
		    else if (mr && bl) {
			h_ptr = new_ptr2;
			v_list[col - 1].right = new_ptr3;
		    }
		    else if (bl && br) {
			v_list[col - 1].right = new_ptr3;
			v_list[col + 1].left = new_ptr2;
		    }
		}		/* starting in the middle of the line */
	    }
	}
	else if (value_flag) {	/* horizontal or vertical line */
	    int ml_val, mc_val, mr_val;	/* horiz */
	    int tc_val, bc_val;	/* vert */

	    ml_val = mc_val = mr_val = tc_val = bc_val = 0;
	    /* only CELL supported */
	    if (data_type == CELL_TYPE) {
		ml_val = ((CELL *) middle)[col - 1];
		mc_val = ((CELL *) middle)[col];
		mr_val = ((CELL *) middle)[col + 1];

		tc_val = ((CELL *) top)[col];
		bc_val = ((CELL *) bottom)[col];
	    }

	    if ((mc && mr) && mc_val != mr_val) {	/* break the horizontal line */
		h_ptr = end_line(h_ptr, 1);
		h_ptr = start_line(1);
	    }
	    else if ((mc && bc) && mc_val != bc_val) {	/* break the vertical line */
		v_list[col].center = end_line(v_list[col].center, 1);
		v_list[col].center = start_line(1);
	    }

	    if ((mc && ml) && mc_val != ml_val) {
		h_ptr->bptr->val = mc_val;
	    }
	    else if ((mc && tc) && mc_val != tc_val) {
		v_list[col].center->bptr->val = mc_val;
	    }
	}
	break;
    case 3:
	if (ml || tl || tc || (tr && !mr)) {
	    if (ml)		/* stop horz. and vert. lines */
		h_ptr = end_line(h_ptr, 1);

	    if (tc)
		v_list[col].center = end_line(v_list[col].center, 1);

	    /* stop diag lines if no horz,vert */
	    if ((tl) && (!ml) && (!tc))
		v_list[col].left = end_line(v_list[col].left, 1);
	    if ((tr) && (!mr) && (!tc))
		v_list[col].right = end_line(v_list[col].right, 1);
	}

	if (mr)			/* start horz. and vert */
	    h_ptr = start_line(1);
	if (bc)
	    v_list[col].center = start_line(1);

	/* start diag if no horz,vert */
	if ((br) && (!mr) && (!bc))
	    v_list[col + 1].left = start_line(1);
	if ((bl) && (!ml) && (!bc))
	    v_list[col - 1].right = start_line(1);
	break;
    case 4:
	if (ml)			/* end horz. and vert lines */
	    h_ptr = end_line(h_ptr, 1);

	if (tc)
	    v_list[col].center = end_line(v_list[col].center, 1);

	/* end diag lines only if no horz,vert */
	if ((tl) && (!ml) && (!tc))
	    v_list[col].left = end_line(v_list[col].left, 1);
	if ((tr) && (!mr) && (!tc))
	    v_list[col].right = end_line(v_list[col].right, 1);

	if (mr)			/* start horz. and vert */
	    h_ptr = start_line(1);
	if (bc)
	    v_list[col].center = start_line(1);

	/* start diag if no horz,vert */
	if ((br) && (!mr) && (!bc))
	    v_list[col + 1].left = start_line(1);
	if ((bl) && (!ml) && (!bc))
	    if (bl)
		v_list[col - 1].right = start_line(1);
	break;
    case 5:
	/* G_message(_("crowded cell %xH (%d,%d) -continuing"),count,row,col); */
	/* I think 5 neighbours is nor crowded, so we shouldn't worry the user
	   Olga */
	if (ml)			/* end horz. and vert lines */
	    h_ptr = end_line(h_ptr, 1);

	if (tc)
	    v_list[col].center = end_line(v_list[col].center, 1);

	/* end diag lines only if no horz,vert */
	if ((tl) && (!ml) && (!tc))
	    v_list[col].left = end_line(v_list[col].left, 1);
	if ((tr) && (!mr) && (!tc))
	    v_list[col].right = end_line(v_list[col].right, 1);

	if (mr)			/* start horz. and vert */
	    h_ptr = start_line(1);
	if (bc)
	    v_list[col].center = start_line(1);

	/* start diag if no horz,vert */
	if ((br) && (!mr) && (!bc))
	    v_list[col + 1].left = start_line(1);
	if ((bl) && (!ml) && (!bc))
	    v_list[col - 1].right = start_line(1);
	break;
    case 6:
	/* the same as case 5 */
	G_debug(1, "Crowded cell %xH (%d,%d), continuing", count, row, col);

	if (ml)			/* end horz. and vert lines */
	    h_ptr = end_line(h_ptr, 1);

	if (tc)
	    v_list[col].center = end_line(v_list[col].center, 1);

	/* end diag lines only if no horz,vert */
	if ((tl) && (!ml) && (!tc))
	    v_list[col].left = end_line(v_list[col].left, 1);
	if ((tr) && (!mr) && (!tc))
	    v_list[col].right = end_line(v_list[col].right, 1);

	if (mr)			/* start horz. and vert */
	    h_ptr = start_line(1);
	if (bc)
	    v_list[col].center = start_line(1);

	/* start diag if no horz,vert */
	if ((br) && (!mr) && (!bc))
	    v_list[col + 1].left = start_line(1);
	if ((bl) && (!ml) && (!bc))
	    v_list[col - 1].right = start_line(1);
	break;
    default:
	G_message(_("Crowded cell at (%f, %f): row %d, col %d, count %d"),
		  Rast_col_to_easting((double)col - .5, &cell_head),
		  Rast_row_to_northing((double)row + .5, &cell_head),
		  row, col - 1, count);
	G_fatal_error(_("Raster map is not thinned properly.\nPlease run r.thin."));
    }				/* switch count */

    return 0;
}

static struct COOR *end_line(struct COOR *ptr, int node)
{
    ptr->row = row;
    ptr->col = col - 1;
    ptr->node = node;

    switch (data_type) {
    case CELL_TYPE:
	ptr->val = ((CELL *) middle)[col];
	break;
    case FCELL_TYPE:
	ptr->dval = ((FCELL *) middle)[col];
	break;
    case DCELL_TYPE:
	ptr->dval = ((DCELL *) middle)[col];
	break;
    default:
	break;
    }

    G_debug(3, "end_line: node: %d; p: row:%d, col:%d",
	    node, ptr->row, ptr->col);

    ptr->fptr = ptr;
    write_line(ptr);

    return (NULL);
}

static struct COOR *start_line(int node)
{
    struct COOR *new_ptr1, *new_ptr2;

    G_debug(3, "start_line: node %d", node);

    new_ptr1 = get_ptr();
    new_ptr2 = get_ptr();
    new_ptr1->bptr = new_ptr1;
    new_ptr1->fptr = new_ptr2;
    new_ptr1->node = node;
    new_ptr2->bptr = new_ptr1;

    return (new_ptr2);
}

static int join_lines(struct COOR *p, struct COOR *q)
{
    p->row = row;
    p->col = col - 1;

    switch (data_type) {
    case CELL_TYPE:
	p->val = ((CELL *) middle)[col];
	break;
    case FCELL_TYPE:
	p->dval = ((FCELL *) middle)[col];
	break;
    case DCELL_TYPE:
	p->dval = ((DCELL *) middle)[col];
	break;
    default:
	break;
    }

    G_debug(3, "join_lines: p: row:%d, col:%d; q: row:%d, col:%d",
	    p->row, p->col, q->row, q->col);

    if (p->fptr != NULL) {
	G_warning(_("join_lines: p front pointer not NULL!"));
	/* exit(EXIT_FAILURE); */
    }

    p->fptr = q->bptr;
    if (q->fptr != NULL) {
	G_warning(_("join_lines: q front pointer not NULL!"));
	/* exit(EXIT_FAILURE); */
    }

    if (q->bptr->fptr == q)
	q->bptr->fptr = p;
    else
	q->bptr->bptr = p;

    free_ptr(q);
    write_line(p);

    return 0;
}

static int extend_line(struct COOR *p, struct COOR *q)
{
    while (p == NULL) {
	G_warning(_("extend line:  p is NULL"));
	/* should never happen by the logic of algorithm */
	p = start_line(1);
    }

    G_debug(3, "extend_line: p: row:%d, col:%d; q: row:%d, col:%d",
	    p->row, p->col, q->row, q->col);

    p->row = row;
    p->col = col - 1;

    switch (data_type) {
    case CELL_TYPE:
	p->val = ((CELL *) middle)[col];
	break;
    case FCELL_TYPE:
	p->dval = ((FCELL *) middle)[col];
	break;
    case DCELL_TYPE:
	p->dval = ((DCELL *) middle)[col];
	break;
    default:
	break;
    }

    if (p->fptr != NULL) {
	G_warning(_("extend_lines: p front pointer not NULL!"));
	/* should never happen by the logic of algorithm */
	/* exit(EXIT_FAILURE); */
    }

    p->fptr = q;
    if (q->bptr != NULL) {
	G_warning(_("extend_lines: q back pointer not NULL!"));
	/* should never happen by the logic of algorithm */
	/* exit(EXIT_FAILURE); */
    }
    q->bptr = p;

    return 0;
}

#if 0
static int stop_line(struct COOR *p, struct COOR *q)
{
    p->row = row;
    p->col = col - 1;

    switch (data_type) {
    case CELL_TYPE:
	ptr->val = ((CELL *) middle)[col];
	break;
    case FCELL_TYPE:
	ptr->dval = ((FCELL *) middle)[col];
	break;
    case DCELL_TYPE:
	ptr->dval = ((DCELL *) middle)[col];
	break;
    default:
	break;
    }

    if (p->fptr != NULL) {
	G_debug(1, "stop_line: p front pointer not NULL!");
	/* should never happen by the logic of algorithm */
	/* exit(EXIT_FAILURE); */
    }

    p->fptr = q;
    if (q->bptr != NULL) {
	G_debug(1, "stop_line: q back pointer not NULL!");
	/* should never happen by the logic of algorithm */
	/* exit(EXIT_FAILURE); */
    }
    q->bptr = p;
    q->fptr = q;

    return 0;
}
#endif

static struct COOR *get_ptr(void)
{
    struct COOR *p;

    p = (struct COOR *)G_malloc(sizeof(struct COOR));
    p->row = row;
    p->col = col - 1;
    p->node = 0;

    switch (data_type) {
    case CELL_TYPE:
	p->val = ((CELL *) middle)[col];
	break;
    case FCELL_TYPE:
	p->dval = ((FCELL *) middle)[col];
	break;
    case DCELL_TYPE:
	p->dval = ((DCELL *) middle)[col];
	break;
    default:
	break;
    }

    G_debug(3, "get_ptr: row:%d, col:%d", p->row, p->col);

    p->bptr = p->fptr = NULL;

    n_alloced_ptrs++;

    return (p);
}

int alloc_lines_bufs(int size)
{
    int i;

    top = (void *)G_malloc(size * data_size);
    middle = (void *)G_malloc(size * data_size);
    bottom = (void *)G_malloc(size * data_size);
    v_list = (struct line_hdr *)G_malloc(size * sizeof(struct line_hdr));

    for (i = 0; i < size; i++)
	v_list[i].left = v_list[i].center = v_list[i].right = NULL;

    n_cols = size;

    return 0;
}

static int read_next(void)
{
    void *p;

    row++;
    p = top;
    top = middle;
    middle = bottom;
    bottom = p;

    return (read_row(bottom));
}
