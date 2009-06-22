/* These routines manage the list of grid-cell candidates for 
 * visiting to calculate distances to surrounding cells.
 * A binary tree (btree) approach is used.  Components are
 * sorted by distance.
 *
 * insert ()
 *   inserts a new row-col with its distance value into the btree
 *
 * delete()
 *   deletes (if possible) a row-col entry in the tree
 *
 * get_lowest()
 *   retrieves the entry with the smallest distance value
 */


#include <grass/gis.h>
#include "local_proto.h"
#include "memory.h"
#include <stdlib.h>

static struct cost *start_cell = NULL;

/*  static int show(struct cost *); */
static int do_quit(double, int, int);

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
struct cost *insert(double min_cost, int row, int col)
{
    struct cost *new_cell, *next_cell;

    /*      new_cell = (struct cost *)(G_malloc(sizeof(struct cost))); */
    new_cell = get();
    if (new_cell == NULL) {
	G_fatal_error("new_cell is NULL\n");
    }
    new_cell->min_cost = min_cost;
    new_cell->row = row;
    new_cell->col = col;
    new_cell->above = NULL;
    new_cell->higher = NULL;
    new_cell->lower = NULL;
    new_cell->nexttie = NULL;
    new_cell->previoustie = NULL;

    if (start_cell == NULL) {
	start_cell = new_cell;
	return (new_cell);
    }

    for (next_cell = start_cell;;) {
	if (min_cost < next_cell->min_cost) {
	    if (next_cell->lower != NULL) {
		next_cell = next_cell->lower;
		continue;
	    }
	    new_cell->above = next_cell;
	    next_cell->lower = new_cell;
	    return (new_cell);
	}
	if (min_cost > next_cell->min_cost) {
	    if (next_cell->higher != NULL) {
		next_cell = next_cell->higher;
		continue;
	    }
	    new_cell->above = next_cell;
	    next_cell->higher = new_cell;
	    return (new_cell);
	}

	/* If we find a value that is exactly equal to new value */
	new_cell->nexttie = next_cell->nexttie;
	next_cell->nexttie = new_cell;
	new_cell->previoustie = next_cell;
	if (new_cell->nexttie != NULL)
	    new_cell->nexttie->previoustie = new_cell;

	return (new_cell);
    }
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
struct cost *find(double min_cost, int row, int col)
{
    struct cost *next_cell;
    struct cost *next_tie_cell;

    for (next_cell = start_cell;;) {
	if (min_cost <= next_cell->min_cost) {
	    for (next_tie_cell = next_cell;
		 next_tie_cell != NULL;
		 next_tie_cell = next_tie_cell->nexttie)
		if (next_tie_cell->row == row && next_tie_cell->col == col)
		    return (next_tie_cell);
	    /*
	       if (next_cell->row == row && next_cell->col == col)
	       return(next_cell) ;
	     */

	    if (next_cell->lower != NULL) {
		next_cell = next_cell->lower;
		continue;
	    }
	    G_message("1 ");
	    return NULL;
	    do_quit(min_cost, row, col);
	}
	else {
	    if (next_cell->higher != NULL) {
		next_cell = next_cell->higher;
		continue;
	    }
	    G_message("2 ");
	    return NULL;
	    do_quit(min_cost, row, col);
	}
    }
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
static int do_quit(double min_cost, int row, int col)
{
    G_warning("Can't find %d,%d:%f\n", row, col, min_cost);
    show_all();
    exit(EXIT_FAILURE);
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
struct cost *get_lowest(void)
{
    struct cost *next_cell;

    if (start_cell == NULL)
	return (NULL);

    for (next_cell = start_cell;
	 next_cell->lower != NULL; next_cell = next_cell->lower) ;

    /* Grab my first tie instead of me */
    if (next_cell->nexttie != NULL)
	next_cell = next_cell->nexttie;

    if (next_cell->row == -1) {
	/*
	   G_message("Deleting %d\n", next_cell) ;
	   show_all() ;
	 */
	delete(next_cell);
	/*
	   G_message("Deleted %d\n", next_cell) ;
	   show_all() ;
	 */
	return (get_lowest());
    }

    return (next_cell);
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int delete(struct cost *delete_cell)
{
    if (delete_cell == NULL) {
	G_warning("Illegal delete request\n");
	return 0;
    }

    /* Simple remove if I'm a one of the "ties" */
    if (delete_cell->previoustie != NULL) {
	delete_cell->previoustie->nexttie = delete_cell->nexttie;
	if (delete_cell->nexttie != NULL)
	    delete_cell->nexttie->previoustie = delete_cell->previoustie;
	give(delete_cell);
	return 0;
    }
    /* If I have a list of ties, link replace me with the first
       in that list  */
    if (delete_cell->nexttie != NULL) {
	delete_cell->nexttie->above = delete_cell->above;
	if (delete_cell->above != NULL) {
	    if (delete_cell->above->lower == delete_cell)
		delete_cell->above->lower = delete_cell->nexttie;
	    else
		delete_cell->above->higher = delete_cell->nexttie;
	}

	delete_cell->nexttie->lower = delete_cell->lower;
	if (delete_cell->lower != NULL)
	    delete_cell->lower->above = delete_cell->nexttie;

	delete_cell->nexttie->higher = delete_cell->higher;
	if (delete_cell->higher != NULL)
	    delete_cell->higher->above = delete_cell->nexttie;
	if (start_cell == delete_cell)
	    start_cell = delete_cell->nexttie;
	delete_cell->nexttie->previoustie = NULL;

	give(delete_cell);
	return (0);
    }
    /*
       \      \      \      \   
       1  X   4  X   7  X   10 X  
       N N    / N    N \    / \ 

       /      /      /      / 
       2  X   5  X   8  X   11 X  
       N N    / N    N \    / \

       N      N      N      N  
       3  X   6  X   9  X   12 X  
       N N    / N    N \    / \
     */
    if (delete_cell->higher == NULL) {	/* 123456       */
	if (delete_cell->lower == NULL) {	/* 123          */
	    if (delete_cell->above == NULL) {	/*   3          */
		start_cell = NULL;
		give(delete_cell);
		return 0;
	    }
	    if (delete_cell->above->higher == delete_cell) {	/* 1            */
		delete_cell->above->higher = NULL;
		give(delete_cell);
		return 0;
	    }
	    else {		/*  2           */
		delete_cell->above->lower = NULL;
		give(delete_cell);
		return 0;
	    }
	}
	else {			/*    456       */

	    if (delete_cell->above == NULL) {	/*      6       */
		start_cell = delete_cell->lower;
		delete_cell->lower->above = NULL;
		give(delete_cell);
		return 0;
	    }
	    if (delete_cell->above->higher == delete_cell) {	/*    4         */
		delete_cell->above->higher = delete_cell->lower;
		delete_cell->lower->above = delete_cell->above;
		give(delete_cell);
		return 0;
	    }
	    else {		/*     5        */
		delete_cell->above->lower = delete_cell->lower;
		delete_cell->lower->above = delete_cell->above;
		give(delete_cell);
		return 0;
	    }
	}
    }
    if (delete_cell->lower == NULL) {	/*       789    */
	if (delete_cell->above == NULL) {	/*         9    */
	    start_cell = delete_cell->higher;
	    delete_cell->higher->above = NULL;
	    give(delete_cell);
	    return 0;
	}
	if (delete_cell->above->higher == delete_cell) {	/*       7      */
	    delete_cell->above->higher = delete_cell->higher;
	    delete_cell->higher->above = delete_cell->above;
	    give(delete_cell);
	    return 0;
	}
	else {			/*        8     */
	    delete_cell->above->lower = delete_cell->higher;
	    delete_cell->higher->above = delete_cell->above;
	    give(delete_cell);
	    return 0;
	}
    }
    /*
     * At this point we are left with 10,11,12 which can be expanded
     *    \   
     *  10 X         X          X   
     *    / \       / \        / \  
     *          A  O   -   B  -   O     C ALL OTHERS
     *      /     / N            N \
     *  11 X  
     *    / \
     *
     *     N  
     *  12 X  
     *    / \
     */

    if (delete_cell->lower->higher == NULL) {	/* A */
	if (delete_cell == start_cell) {	/* 12A */
	    delete_cell->lower->higher = delete_cell->higher;
	    delete_cell->higher->above = delete_cell->lower;
	    start_cell = delete_cell->lower;
	    delete_cell->lower->above = NULL;
	    give(delete_cell);
	    return 0;
	}
	if (delete_cell->above->higher == delete_cell) {	/* 10A */
	    delete_cell->lower->higher = delete_cell->higher;
	    delete_cell->higher->above = delete_cell->lower;
	    delete_cell->above->higher = delete_cell->lower;
	    delete_cell->lower->above = delete_cell->above;
	    give(delete_cell);
	    return 0;
	}
	else {			/* 11A */
	    delete_cell->lower->higher = delete_cell->higher;
	    delete_cell->higher->above = delete_cell->lower;
	    delete_cell->above->lower = delete_cell->lower;
	    delete_cell->lower->above = delete_cell->above;
	    give(delete_cell);
	    return 0;
	}
    }
    if (delete_cell->higher->lower == NULL) {	/* A */
	if (delete_cell == start_cell) {	/* 12B */
	    delete_cell->higher->lower = delete_cell->lower;
	    delete_cell->lower->above = delete_cell->higher;
	    start_cell = delete_cell->higher;
	    delete_cell->higher->above = NULL;
	    give(delete_cell);
	    return 0;
	}
	if (delete_cell->above->lower == delete_cell) {	/* 11B */
	    delete_cell->higher->lower = delete_cell->lower;
	    delete_cell->lower->above = delete_cell->higher;
	    delete_cell->above->lower = delete_cell->higher;
	    delete_cell->higher->above = delete_cell->above;
	    give(delete_cell);
	    return 0;
	}
	else {			/* 10B */
	    delete_cell->higher->lower = delete_cell->lower;
	    delete_cell->lower->above = delete_cell->higher;
	    delete_cell->above->higher = delete_cell->higher;
	    delete_cell->higher->above = delete_cell->above;
	    give(delete_cell);
	    return 0;
	}
    }
    /* If we get this far, the node cannot be safely removed.  Just leave
     * an internal mark noting that it is no longer good.
     */
    delete_cell->row = -1;
    return 0;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int show_all(void)
{
    if (start_cell == NULL) {
	G_message("Nothing to show\n");
	return 1;
    }
    show(start_cell);

    return 0;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int show(struct cost *next)
{
    struct cost *next_cell;

    if (next == NULL)
	return 0;
    for (next_cell = next; next_cell != NULL; next_cell = next_cell->nexttie)
	G_message("%p %d,%d,%f %p %p %p %p\n",
		  next_cell,
		  next_cell->row,
		  next_cell->col,
		  next_cell->min_cost,
		  next_cell->nexttie,
		  next_cell->lower, next_cell->higher, next_cell->above);
    show(next->lower);
    show(next->higher);

    return 0;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int check_all(char *str)
{
    G_message("\n");
    if (start_cell->above != NULL) {
	G_fatal_error("Bad Start Cell\n");
    }
    check(str, start_cell);

    return 0;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int check(char *str, struct cost *start)
{
    if (start == NULL)
	return 0;

    if (start->lower != NULL) {
	if (start->min_cost < start->lower->min_cost) {
	    G_warning("%s %f-%f lower cost higher or equal\n", str,
		      start->min_cost, start->lower->min_cost);
	    show_all();
	    exit(EXIT_FAILURE);
	}
	if (start->lower->above != start) {
	    G_warning("%s lower above pointer wrong\n", str);
	    show_all();
	    exit(EXIT_FAILURE);
	}
    }
    if (start->higher != NULL) {
	if (start->min_cost >= start->higher->min_cost) {
	    G_warning("%s %f-%f higher cost lower\n", str,
		      start->min_cost, start->higher->min_cost);
	    show_all();
	    exit(EXIT_FAILURE);
	}
	if (start->higher->above != start) {
	    G_warning("%s higher above pointer wrong\n", str);
	    show_all();
	    exit(EXIT_FAILURE);
	}
    }
    check(str, start->lower);
    check(str, start->higher);

    return 0;
}
