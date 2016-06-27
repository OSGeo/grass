/*
 * Cell-file area extraction
 *   Boundary tracing algorithm
 * 
 * Jean Ezell
 * US Army Corps of Engineers
 * Construction Engineering Research Lab
 * Modelling and Simulation Team
 * Champaign, IL  61820
 * January 1988
 *
 * Fixed 3/2001 by Andrea Aime <aaime@comune.modena.it>
 *   fixed area label problem with r.poly (it was creating more        
 *   labels than required if there were islands)
 *
 *************************
 * The algorithm allocates COOR structures as necessary to mark endpoints of
 * and bends in boundaries between areas.  Each COOR structure contains the
 * row and column coordinates of a point which is either an endpoint or a
 * bend.  If the point represents a bend, fptr and bptr point to the adjacent
 * endpoint(s) or bend(s); if an endpoint, one of fptr or bptr points to the
 * point itself and the other to the adjacent bend or endpoint.  While a
 * boundary is under construction, fptr of the "free" end is NULL.
 * The right and left fields contain the area numbers to the right and
 * left of the line under construction.  When two lines are joined, it
 * will be necessary to make the area numbers agree.  This is done by mapping
 * one of the area numbers to another.  The mapping is done in such a way
 * that the smallest number in each equivalence class is chosen to represent
 * the class.  We also keep track of the longest horizontal strip in each
 * area.  The left end of this strip will be used as the position of the
 * label in the dlg label file.
 * 
 * Global variables:
 *   v_list          pointer to allocated array of pointers to endpoints of
 *                   vertical lines currently under construction
 *   h_ptr           pointer to endpoint of horizontal line currently
 *                   under construction
 *   col, row        column and row of current position in file
 *   buffer[]        pointers to the two allocated areas which hold the
 *                   two rows currently being processed
 *   top, bottom     which row of buffer[] is "top" line from file and
 *                   which is "bottom"
 *   scan_length     length of a row from the data file
 *   tl, tr, bl, br  top left and right, bottom left and right elements
 *                   in current 2 by 2 data window; one-time calculation
 *                   prevents multiple indexing and indirection
 *                   computations
 *   a_list          pointer to allocated array of correspondences between
 *                   areas and categories
 *   n_areas         current length of a_list
 *   area_num        lowest area number available for use
 *   a_list_new      pointer to next a_list entry to be used
 *   a_list_old      pointer to a_list entry just filled
 *   e_list          pointer to allocated array which is used to form
 *                   mapping of all equivalent area numbers onto one
 *                   number from the class
 *   n_equiv         current length of e_list
 * 
 * Entry points:
 *   extract_areas   driver for boundary extraction, area labelling
 *                   algorithm
 *   alloc_bufs      allocate buffers for raster map data and storage of
 *                   information obtained in the extraction process (v_list,
 *                   a_list, e_list, buffer[0], buffer[1])
 *
 ********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include "global.h"

static int col, row, top, bottom;
static double tl, tr, bl, br;
static struct COOR **v_list;
static struct COOR *h_ptr;
static void *buffer[2];
static int scan_length;
static int n_areas, area_num, n_equiv, tl_area;
static struct area_table *a_list, *a_list_new, *a_list_old;
static struct equiv_table *e_list;

/* function prototypes */
static int update_list(int);
static int end_vline();
static int end_hline();
static int start_vline();
static int start_hline();
static struct COOR *get_ptr();
static int read_next();
static int equiv_areas(int, int);
static int map_area(int, int);
static int add_to_list(int, int);
static int assign_area(double, int);
static int more_areas();
static int update_width(struct area_table *, int);
static int nabors(void);

#define get_raster_value(ptr, col) \
	Rast_get_d_value(G_incr_void_ptr(ptr, (col)*data_size), data_type)

/* extract_areas - trace boundaries of polygons in file */

int extract_areas(void)
{
    double nullVal;
    int i;

    row = col = top = 0;	/* get started for read of first */
    bottom = 1;			/* line from raster map */
    area_num = 0;
    tl_area = 0;
    n_alloced_ptrs = 0;

    Rast_set_d_null_value(&nullVal, 1);
    /* represents the "outside", the external null values */
    assign_area(nullVal, 0);

    G_message(_("Extracting areas..."));

    scan_length = read_next();
    while (read_next()) {	/* read rest of file, one row at *//*   a time */
	G_percent(row, n_rows + 1, 2);

	for (col = 0; col < scan_length - 1; col++) {
	    tl = get_raster_value(buffer[top], col);	/* top left in window */
	    tr = get_raster_value(buffer[top], col + 1);	/* top right */
	    bl = get_raster_value(buffer[bottom], col);	/* bottom left */
	    br = get_raster_value(buffer[bottom], col + 1);	/* bottom right */
	    update_list(nabors());
	}

	if (h_ptr != NULPTR)	/* if we have a loose end, */
	    end_hline();	/*   tie it down */

	row++;
    }
    G_percent(1, 1, 1);

    write_area(a_list, e_list, area_num, n_equiv);

    G_free(a_list);
    for (i = 0; i < n_equiv; i++) {
	if (e_list[i].ptr)
	    G_free(e_list[i].ptr);
    }
    G_free(e_list);
    G_free(v_list);
    G_free(buffer[0]);
    G_free(buffer[1]);

    if (n_alloced_ptrs) {
	/* should not happen */
	G_warning("Memory leak: %d points are still in use", n_alloced_ptrs);
    }

    return 0;
}				/* extract_areas */

/* update_list - maintains linked list of COOR structures which resprsent */
/* bends in and endpoints of lines separating areas in input file; */
/* compiles a list of area to category number correspondences; */
/* for pictures of what each case in the switch represents, see comments */
/* before nabors() */

static int update_list(int i)
{
    struct COOR *new_ptr, *new_ptr1, *new_ptr2, *new_ptr3;
    double right, left;

    switch (i) {
    case 0:
	/* Vertical line - Just update width information */
	/* update_width(a_list + v_list[col]->left,0); */
	tl_area = v_list[col]->left;
	break;
    case 1:
	/* Bottom right corner - Point in middle of new line */
	/* (growing) <- ptr2 -><- ptr1 -><- ptr3 -> (growing) */
	/*            (?, col) (row, col) (row, ?) */
	new_ptr1 = get_ptr();	/* corner point */
	new_ptr2 = get_ptr();	/* downward-growing point */
	new_ptr3 = get_ptr();	/* right-growing point */
	new_ptr1->bptr = new_ptr2;
	new_ptr1->fptr = new_ptr3;
	new_ptr2->bptr = new_ptr3->bptr = new_ptr1;

	/* if(Rast_is_c_null_value(&tl_area)) {
	   new_ptr1->left = new_ptr2->right = new_ptr3->left = 0;
	   assign_area(tl,1);
	   } else {
	 */
	new_ptr1->left = new_ptr2->right = new_ptr3->left = tl_area;
	new_ptr1->right = new_ptr2->left = new_ptr3->right = area_num;

	assign_area(br, 1);
	update_width(a_list_old, 1);
	v_list[col] = new_ptr2;
	h_ptr = new_ptr3;
	break;
    case 3:
	/* Bottom left corner - Add point to line already under construction */
	/* (fixed) -><- original h_ptr -><- new_ptr -> (growing) */
	/*                (row, col)       (?, col) */
	tl_area = h_ptr->left;
	new_ptr = get_ptr();	/* downward-growing point */
	h_ptr->col = col;
	h_ptr->fptr = new_ptr;
	new_ptr->bptr = h_ptr;
	new_ptr->left = h_ptr->left;
	new_ptr->right = h_ptr->right;

	/* update_width(a_list + new_ptr->left,3); */
	v_list[col] = new_ptr;
	h_ptr = NULPTR;
	break;
    case 4:
	/* Top left corner - Join two lines already under construction */
	/* (fixed) -><- original v_list -><- (fixed) */
	/*                 (row, col) */
	tl_area = v_list[col]->left;
	equiv_areas(h_ptr->left, v_list[col]->right);
	equiv_areas(h_ptr->right, v_list[col]->left);
	v_list[col]->row = row;	/* keep downward-growing point */
	v_list[col]->fptr = h_ptr->bptr;	/*   and join it to predecessor */
	h_ptr->bptr->fptr = v_list[col];	/*   of right-growing point */
	free_ptr(h_ptr);		/* right-growing point disappears */
	h_ptr = NULPTR;		/* turn loose of pointers */
	write_boundary(v_list[col]);	/* try to write line */
	v_list[col] = NULPTR;	/* turn loose of pointers */
	break;
    case 5:
	/* Top right corner - Add point to line already under construction */
	/* (fixed) -><- original v_list -><- new_ptr -> (growing) */
	/*                 (row, col)        (row, ?) */
	new_ptr = get_ptr();	/* right-growing point */
	v_list[col]->row = row;
	new_ptr->bptr = v_list[col];
	new_ptr->left = v_list[col]->left;
	new_ptr->right = v_list[col]->right;
	v_list[col]->fptr = new_ptr;
	h_ptr = new_ptr;
	v_list[col] = NULPTR;
	break;
    case 6:
	/* T upward - End one vertical and one horizontal line */
	/*            Start horizontal line */
	v_list[col]->node = h_ptr->node = 1;
	left = v_list[col]->left;
	right = h_ptr->right;
	end_vline();
	end_hline();
	start_hline();
	h_ptr->bptr->node = 1;	/* where we came from is a node */
	h_ptr->left = h_ptr->bptr->left = left;
	h_ptr->right = h_ptr->bptr->right = right;
	break;
    case 7:
	/* T downward - End horizontal line */
	/*              Start one vertical and one horizontal line */
	h_ptr->node = 1;
	right = h_ptr->right;
	left = h_ptr->left;
	end_hline();
	start_hline();
	start_vline();
	h_ptr->bptr->node = v_list[col]->bptr->node = 1;
	h_ptr->left = h_ptr->bptr->left = left;
	h_ptr->right = h_ptr->bptr->right = v_list[col]->left =
	    v_list[col]->bptr->left = area_num;
	assign_area(br, 7);
	update_width(a_list_old, 7);
	v_list[col]->right = v_list[col]->bptr->right = right;
	break;
    case 8:
	/* T left - End one vertical and one horizontal line */
	/*          Start one vertical line */
	tl_area = v_list[col]->left;
	h_ptr->node = v_list[col]->node = 1;
	right = h_ptr->right;
	left = v_list[col]->left;
	end_vline();
	end_hline();
	start_vline();
	v_list[col]->bptr->node = 1;	/* where we came from is a node */
	v_list[col]->left = v_list[col]->bptr->left = left;
	v_list[col]->right = v_list[col]->bptr->right = right;
	/* update_width(a_list + v_list[col]->left,8); */
	break;
    case 9:
	/* T right - End one vertical line */
	/*           Start one vertical and one horizontal line */
	v_list[col]->node = 1;
	right = v_list[col]->right;
	left = v_list[col]->left;
	end_vline();
	start_vline();
	start_hline();
	v_list[col]->bptr->node = h_ptr->bptr->node = 1;
	h_ptr->left = h_ptr->bptr->left = left;
	h_ptr->right = h_ptr->bptr->right = v_list[col]->left =
	    v_list[col]->bptr->left = area_num;
	assign_area(br, 9);
	update_width(a_list_old, 9);
	v_list[col]->right = v_list[col]->bptr->right = right;
	break;
    case 10:
	/* Cross - End one vertical and one horizontal line */
	/*         Start one vertical and one horizontal line */
	v_list[col]->node = h_ptr->node = 1;
	left = v_list[col]->left;
	right = h_ptr->right;
	end_vline();
	end_hline();
	start_vline();
	start_hline();
	v_list[col]->bptr->node = h_ptr->bptr->node = 1;
	h_ptr->left = h_ptr->bptr->left = left;
	v_list[col]->left = v_list[col]->bptr->left = h_ptr->right =
	    h_ptr->bptr->right = area_num;
	assign_area(br, 10);
	update_width(a_list_old, 10);
	v_list[col]->right = v_list[col]->bptr->right = right;
	break;
    }				/* switch */

    return 0;
}				/* update_list */

/* end_vline, end_hline - end vertical or horizontal line and try */
/* to write it out */

static int end_vline(void)
{
    v_list[col]->row = row;
    v_list[col]->fptr = v_list[col];
    write_boundary(v_list[col]);
    v_list[col] = NULPTR;

    return 0;
}

static int end_hline(void)
{
    h_ptr->col = col;
    h_ptr->fptr = h_ptr;
    write_boundary(h_ptr);
    h_ptr = NULPTR;

    return 0;
}

/* start_vline, start_hline - begin line in vertical or horizontal */
/* direction */

static int start_vline(void)
{
    struct COOR *new_ptr1, *new_ptr2;

    new_ptr1 = get_ptr();
    new_ptr2 = get_ptr();
    new_ptr1->fptr = new_ptr2;
    new_ptr2->bptr = new_ptr1->bptr = new_ptr1;
    new_ptr2->fptr = NULPTR;
    v_list[col] = new_ptr2;

    return 0;
}

static int start_hline(void)
{
    struct COOR *new_ptr1, *new_ptr2;

    new_ptr1 = get_ptr();
    new_ptr2 = get_ptr();
    new_ptr1->bptr = new_ptr2->bptr = new_ptr1;
    new_ptr1->fptr = new_ptr2;
    new_ptr2->fptr = NULPTR;
    h_ptr = new_ptr2;

    return 0;
}

/* get_ptr - allocate storage for yet another COOR structure */

static struct COOR *get_ptr(void)
{
    static struct COOR *ptr;

    ptr = (struct COOR *)G_malloc(sizeof(struct COOR));
    ptr->row = row;
    ptr->col = col;
    ptr->fptr = ptr->bptr = NULPTR;
    ptr->node = ptr->left = ptr->right = 0;
    
    n_alloced_ptrs++;

    return (ptr);
}

/* nabors - check 2 x 2 matrix and return case from table below */

/*    *--*--*      *--*--*      *--*--*      *--*--*    */
/*    |  |  |      |     |      |     |      |     |    */
/*    *  |  *      *  *--*      *-----*      *--*  *    */
/*    |  |  |      |  |  |      |     |      |  |  |    */
/*    *--*--*      *--*--*      *--*--*      *--*--*    */

/*       0            1            2            3       */

/*    *--*--*      *--*--*      *--*--*      *--*--*    */
/*    |  |  |      |  |  |      |  |  |      |     |    */
/*    *--*  *      *  *--*      *--*--*      *--*--*    */
/*    |     |      |     |      |     |      |  |  |    */
/*    *--*--*      *--*--*      *--*--*      *--*--*    */

/*       4            5            6            7       */

/*    *--*--*      *--*--*      *--*--*      *--*--*    */
/*    |  |  |      |  |  |      |  |  |      |     |    */
/*    *--*  *      *  *--*      *--*--*      *     *    */
/*    |  |  |      |  |  |      |  |  |      |     |    */
/*    *--*--*      *--*--*      *--*--*      *--*--*    */
/*                                                      */
/*       8            9            10           11      */

static int nabors(void)
{
    int tl_null = Rast_is_d_null_value(&tl);
    int tr_null = Rast_is_d_null_value(&tr);
    int bl_null = Rast_is_d_null_value(&bl);
    int br_null = Rast_is_d_null_value(&br);

    /* if both a and b are NULLs, thery are equal */
#define cmp(a, b) (a##_null+b##_null==1 || (a##_null+b##_null==0 && a != b))

    if (cmp(tl, tr) != 0) {	/* 0, 4, 5, 6, 8, 9, 10 */
	if (cmp(tl, bl) != 0) {	/* 4, 6, 8, 10 */
	    if (cmp(bl, br) != 0) {	/* 8, 10 */
		if (cmp(tr, br) != 0)
		    return (10);
		else
		    return (8);
	    }
	    else {		/* 4, 6 */

		if (cmp(tr, br) != 0)
		    return (6);
		else
		    return (4);
	    }
	}
	else {			/* 0, 5, 9 */

	    if (cmp(bl, br) != 0) {	/* 0, 9 */
		if (cmp(tr, br) != 0)
		    return (9);
		else
		    return (0);
	    }
	    else
		return (5);
	}
    }
    else {			/* 1, 2, 3, 7, 11 */

	if (cmp(tl, bl) != 0) {	/* 2, 3, 7 */
	    if (cmp(bl, br) != 0) {	/* 3, 7 */
		if (cmp(tr, br) != 0)
		    return (7);
		else
		    return (3);
	    }
	    else
		return (2);
	}
	else {			/* 1, 11 */

	    if (cmp(bl, br) != 0)
		return (1);
	    else
		return (11);
	}
    }

    return 0;
}

/* read_next - read another line from input file */

static int read_next(void)
{
    int n;

    top = bottom++;		/* switch top and bottom, */
    bottom = 1 & bottom;	/*   which are always 0 or 1 */
    n = read_row(buffer[bottom]);

    return (n);
}

/* alloc_bufs - allocate buffers we will need for storing raster map */
/* data, pointers to extracted lines, area number information */

int alloc_areas_bufs(int size)
{
    int i;

    buffer[0] = (void *)G_malloc(size * data_size);
    buffer[1] = (void *)G_malloc(size * data_size);
    v_list = (struct COOR **)G_malloc(size * sizeof(*v_list));
    n_areas = n_equiv = 500;	/* guess at number of areas, equivs */
    a_list =
	(struct area_table *)G_malloc(n_areas * sizeof(struct area_table));

    for (i = 0; i < n_areas; i++) {
	(a_list + i)->width = (a_list + i)->row = (a_list + i)->col = 0;
	(a_list + i)->free = 1;
    }
    a_list_new = a_list_old = a_list;

    e_list =
	(struct equiv_table *)G_malloc(n_equiv * sizeof(struct equiv_table));
    for (i = 0; i < n_equiv; i++) {
	(e_list + i)->mapped = (e_list + i)->count = 0;
	(e_list + i)->ptr = NULL;
    }

    return 0;
}

/* equiv_areas - force two areas to be equivalent and generate */
/* mapping information */

static int equiv_areas(int a1, int a2)
{
    int small, large, small_obj, large_obj;

    if (a1 == -1 || a2 == -2)
	return 0;

    if (a1 == a2)
	return (0);
    if (a1 < a2) {
	small = a1;
	large = a2;
    }
    else {
	small = a2;
	large = a1;
    }

    while (large >= n_equiv)	/* make sure our equivalence tables */
	more_equivs();		/*   are large enough */

    if ((e_list + large)->mapped) {
	if ((e_list + small)->mapped) {	/* small mapped, large mapped */
	    large_obj = (e_list + large)->where;
	    small_obj = (e_list + small)->where;
	    if (large_obj == small_obj)	/* both mapped to same place */
		return (0);
	    if (small_obj < large_obj)	/* map where large goes to where */
		map_area(large_obj, small_obj);	/*   small goes */
	    else		/* map where small goes to where */
		map_area(small_obj, large_obj);	/*   large goes */
	}
	else {			/* small not mapped, large mapped */

	    large_obj = (e_list + large)->where;
	    if (small == large_obj)	/* large already mapped to small */
		return (0);
	    if (small < large_obj)	/* map where large goes to small */
		map_area(large_obj, small);
	    else		/* map small to where large goes */
		map_area(small, large_obj);
	}
    }
    else {
	if ((e_list + small)->mapped)	/* small mapped, large not mapped */
	    map_area(large, (e_list + small)->where);
	else			/* small not mapped, large not mapped */
	    map_area(large, small);
    }

    return (0);
}

/* map_area - establish a mapping from one area to another */
/* the area number mapping looks like the following: */
/*   area numbers index into e_list to get equiv_table structures */
/*   for each of these structures, */
/*   .mapped indicates whether area number is mapped */
/*   if area number is mapped */
/*     .where tells to what other area number */
/*   if area number is not mapped */
/*     .count tells how many areas are mapped to this one */
/*     if count != 0 */
/*       .ptr gives a pointer to the list of area numbers */

static int map_area(int x, int y	/* map x to y */
    )
{
    int n, i, *p;

    (e_list + x)->mapped = 1;
    (e_list + x)->where = y;

    if ((a_list + x)->width > (a_list + y)->width) {
	(a_list + y)->width = (a_list + x)->width;
	(a_list + y)->row = (a_list + x)->row;
	(a_list + y)->col = (a_list + x)->col;
    }

    if (add_to_list(x, y)) {	/* if x is not already in y's list */
	n = (e_list + x)->count;
	p = (e_list + x)->ptr;
	for (i = 0; i < n; i++) {	/* map everything that is currently *//*   mapped onto x onto y; because */
	    (e_list + *p)->where = y;	/*   of this reshuffle, only one */
	    add_to_list(*p++, y);	/*   level of mapping is ever needed */
	}
    }

    return 0;
}

/* add_to_list - add another area number to an equivalence list */

static int add_to_list(int x, int y)
{
    int n, i;
    struct equiv_table *e_list_y;

    e_list_y = e_list + y;
    n = e_list_y->count;
    if (n == 0) {		/* first time through--start list */
	e_list_y->length = 20;	/* initial guess at storage needed */
	e_list_y->ptr = (int *)G_malloc(e_list_y->length * sizeof(int));
	*(e_list_y->ptr) = x;
	(e_list_y->count)++;
    }
    else {			/* list already started */

	for (i = 0; i < n; i++) {	/* check whether x is already *//*   in list */
	    if (*(e_list_y->ptr + i) == x)
		return (0);	/* if so, we don't need to add it */
	}

	if (n + 1 >= e_list_y->length) {	/* add more space for storage *//*   if necessary */
	    e_list_y->length += 10;
	    e_list_y->ptr =
		(int *)G_realloc(e_list_y->ptr,
				 e_list_y->length * sizeof(int));
	}

	*(e_list_y->ptr + n) = x;	/* add x to list */
	(e_list_y->count)++;
    }

    return (1);			/* indicate addition made */
}

/* assign_area - make current area number correspond to the passed */
/* category number and allocate more space to store areas if necessary */

static int assign_area(double cat, int kase)
{
    a_list_new->free = 0;
    a_list_new->cat = cat;
    area_num++;

    if (area_num >= n_areas)
	more_areas();

    a_list_old = a_list + area_num - 1;
    a_list_new = a_list + area_num;

    return 0;
}

/* more_areas - allocate larger space to store area correspondences */

static int more_areas(void)
{
    int old_n, i;

    old_n = n_areas;
    n_areas += 250;

    a_list =
	(struct area_table *)G_realloc(a_list,
				       n_areas * sizeof(struct area_table));
    for (i = old_n; i < n_areas; i++) {
	(a_list + i)->width = -1;
	(a_list + i)->free = 1;
    }

    return 0;
}

/* more_equivs - allocate more space to construct equivalence information */

int more_equivs(void)
{
    int old_n, i;

    old_n = n_equiv;
    n_equiv += 250;

    e_list =
	(struct equiv_table *)G_realloc(e_list,
					n_equiv * sizeof(struct equiv_table));
    for (i = old_n; i < n_equiv; i++) {
	(e_list + i)->mapped = (e_list + i)->count = 0;
	(e_list + i)->ptr = NULL;
    }

    return 0;
}

/* update_width - update position of longest horizontal strip in an area */
static int update_width(struct area_table *ptr, int kase)
{
    int w, j, a;
    struct equiv_table *ep;

    a = (ptr - a_list);
    for (j = col + 1, w = 0; j < scan_length &&
	 get_raster_value(buffer[bottom], j) == br; j++, w++) ;

    if (a == 0)
	G_debug(1, "Area 0, %d \t%d \t%d \t%d \t%d", kase, row, col,
		ptr->width, w);

    if (a < n_equiv) {
	ep = e_list + a;
	if (ep->mapped)
	    ptr = a_list + ep->where;
    }

    if (w > ptr->width) {
	ptr->width = w;
	ptr->row = row;
	ptr->col = col;
    }

    return 0;
}
