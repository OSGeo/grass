/*
 *   r3.stats support functions
 *
 *   Copyright (C) 2004-2014 by the GRASS Development Team
 *   Author(s): Soeren Gebbert
 *
 *      This program is free software under the GNU General Public
 *      License (>=v2). Read the file COPYING that comes with GRASS
 *      for details.
 *
 */

#include <grass/gis.h>
#include "local_proto.h"


/* *************************************************************** */
/* ***** allocate equal_val_array structure ********************* */
/* *************************************************************** */
equal_val_array *alloc_equal_val_array(int count)
{
    equal_val_array *p;
    int i;


    p = (equal_val_array *) G_calloc(1, sizeof(equal_val_array));
    p->count = count;

    p->values = (equal_val **) G_calloc(p->count, sizeof(equal_val *));

    for (i = 0; i < p->count; i++)
	p->values[i] = (equal_val *) G_calloc(1, sizeof(equal_val));


    return p;
}

/* *************************************************************** */
/* ***** add an equal_val to the equal_val_array structure ******* */
/* *************************************************************** */
equal_val_array *add_equal_val_to_array(equal_val_array *array, double val)
{
    equal_val_array *p = array;
    int count;

    if (p == NULL) {
	p = alloc_equal_val_array(1);
	p->values[0]->val = val;
	p->values[0]->count = 1;
	G_debug(5, "Create new equal_array with value %g\n", val);
    }
    else {
	/*increase the count */
	count = array->count;
	count++;

	/*new memory */
	p->values =
	    (equal_val **) G_realloc(p->values, count * sizeof(equal_val *));
	p->values[count - 1] = (equal_val *) G_calloc(1, sizeof(equal_val));
	/*set the new value */
	p->values[count - 1]->val = val;
	p->values[count - 1]->count = 1;
	/*set the new counter */
	p->count = count;
	G_debug(5, "Add new value %g at position %i\n", val, p->count);
    }

    return p;
}

/* *************************************************************** */
/* ***** check if a value exists in the equal_val_array ********* */
/* *************************************************************** */
int check_equal_value(equal_val_array *array, double val)
{
    int i;

    /*search if the new value exists and increase the counter */
    if (array != NULL)
	for (i = 0; i < array->count; i++) {
	    if (array->values[i]->val == val) {
		array->values[i]->count++;
		G_debug(5, "found value %g with count %i at pos %i\n", val,
			array->values[i]->count, i);
		return 1;
	    }
	}

    /*if it does not exists, add it to the array */
    add_equal_val_to_array(array, val);

    /*return 1 if found, 0 otherwise */
    return 0;
}


/* *************************************************************** */
/* ***** Release the memory of a equal_val_array structure ****** */
/* *************************************************************** */
void free_equal_val_array(equal_val_array *uvals)
{
    int i;

    for (i = 0; i < uvals->count; i++) {
	G_free(uvals->values[i]);
    }

    G_free(uvals->values);
    G_free(uvals);
    uvals = NULL;

    return;
}

/* *************************************************************** */
/* **** Create the structure which manages the statistical ******* */
/* **** values for a value range or equal values **************** */
/* *************************************************************** */
stat_table *create_stat_table(int nsteps, equal_val_array *eqvals,
			      double min, double max)
{
    stat_table *p;
    int i;
    double step;

    /* Memory */
    p = (stat_table *) G_calloc(1, sizeof(stat_table));

    p->null = (stat_row *) G_calloc(nsteps, sizeof(stat_row));
    p->table = (stat_row **) G_calloc(nsteps, sizeof(stat_row *));

    for (i = 0; i < nsteps; i++)
	p->table[i] = (stat_row *) G_calloc(1, sizeof(stat_row));

    /* Some value initializing */
    p->null->min = 0;
    p->null->max = 0;
    p->null->vol = 0;
    p->null->perc = 0;
    p->null->count = 0;
    p->null->num = nsteps + 1;

    p->nsteps = nsteps;
    p->sum_count = 0;
    p->sum_vol = 0;
    p->sum_perc = 0;
    p->equal = 0;

    /*if no equal values are provided, calculate the range and the length of each step */
    if (!eqvals) {

	/*calculate the steplength */
	step = (max - min) / (nsteps);
	p->equal = 0;

	p->table[0]->min = min;
	p->table[0]->max = min + step;
	p->table[0]->num = 1;
	p->table[0]->count = 0;
	p->table[0]->vol = 0;
	p->table[0]->perc = 0;

	G_debug(3, "Step %i range min %.11lf max %.11lf\n", p->table[0]->num,
		p->table[0]->min, p->table[0]->max);

	for (i = 1; i < nsteps; i++) {
	    p->table[i]->min = p->table[i - 1]->max;
	    p->table[i]->max = p->table[i - 1]->max + step;
	    p->table[i]->num = i + 1;
	    p->table[i]->count = 0;
	    p->table[i]->vol = 0;
	    p->table[i]->perc = 0;
	    G_debug(5, "Step %i range min %.11lf max %.11lf\n",
		    p->table[i]->num, p->table[i]->min, p->table[i]->max);
	}
	/* the last value must be a bit larger */
	p->table[nsteps - 1]->max += COMPARE_PRECISION;
    }
    else {			/* Create equal value statistic */
	p->equal = 1;
	for (i = 0; i < eqvals->count; i++) {
	    p->table[i]->min = eqvals->values[i]->val;	/* equal values have no range, set min and max to the same value */
	    p->table[i]->max = eqvals->values[i]->val;
	    p->table[i]->num = i + 1;
	    p->table[i]->count = eqvals->values[i]->count;	/* the appearance count for each equal value */
	    p->table[i]->vol = 0;
	    p->table[i]->perc = 0;
	    G_debug(5, "Unique value %i = %g count %i\n", p->table[i]->num,
		    p->table[i]->min, p->table[i]->count);
	}

    }

    return p;
}

/* *************************************************************** */
/* ***** Release the memory of a stat_table structure ************ */
/* *************************************************************** */
void free_stat_table(stat_table *stats)
{
    int i;

    for (i = 0; i < stats->nsteps; i++) {
	G_free(stats->table[i]);
    }

    G_free(stats->table);
    G_free(stats->null);

    G_free(stats);
    stats = NULL;

    return;
}

/* *************************************************************** */
/* **** Compute the volume, percentage and sums ****************** */
/* *************************************************************** */
void update_stat_table(stat_table *stats, RASTER3D_Region *region)
{
    int i;
    double vol = region->ns_res * region->ew_res * region->tb_res;
    int cellnum = region->rows * region->cols * region->depths;

    /*Calculate volume and percentage for each range or equal value and the sum stats */
    for (i = 0; i < stats->nsteps; i++) {
	stats->table[i]->vol = stats->table[i]->count * vol;
	stats->table[i]->perc =
	    (double)(100.0 * (double)stats->table[i]->count /
		     (double)cellnum);
	stats->sum_count += stats->table[i]->count;
	stats->sum_vol += stats->table[i]->vol;
	stats->sum_perc += stats->table[i]->perc;
    }

    /*calculate the null value stats */
    stats->null->vol = stats->null->count * vol;
    stats->null->perc =
	(double)(100.0 * (double)stats->null->count / (double)cellnum);

    return;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
void print_stat_table(stat_table *stats, int counts_only)
{
    int i;

    if (stats->equal) {
	/*       1234567   012345678901234567   0123456789012   0123456   0123456789 */
	fprintf(stdout,
		"  num   |        value       |     volume    |   perc  | cell count\n");

	for (i = 0; i < stats->nsteps; i++) {
	    fprintf(stdout, "%7i   %18.6lf   %13.3lf   %7.5lf   %10i\n",
		    stats->table[i]->num, stats->table[i]->min,
		    stats->table[i]->vol, stats->table[i]->perc,
		    stats->table[i]->count);
	}
	fprintf(stdout,
		"%7i                    *   %13.3lf   %7.5lf   %10i\n",
		stats->null->num, stats->null->vol, stats->null->perc,
		stats->null->count);

	fprintf(stdout, "\nNumber of groups with equal values: %i",
		stats->nsteps);
    }
    else if (counts_only) {
	for (i = 0; i < stats->nsteps; i++) {
	    fprintf(stdout, "%d %ld\n",
		    stats->table[i]->num, stats->table[i]->count);
	}
	fprintf(stdout, "* %ld\n", stats->null->count);
    }
    else {
	/*       1234567   012345678901234567   012345678901234567   0123456789012   0123456   0123456789 */
	fprintf(stdout,
		"  num   | minimum <= value   | value < maximum    |     volume    |   perc  | cell count\n");

	for (i = 0; i < stats->nsteps; i++) {
	    fprintf(stdout,
		    "%7i   %18.9lf   %18.9lf   %13.3lf   %7.5lf   %10i\n",
		    stats->table[i]->num, stats->table[i]->min,
		    stats->table[i]->max, stats->table[i]->vol,
		    stats->table[i]->perc, stats->table[i]->count);

	}
	fprintf(stdout,
		"%7i                    *                    *   %13.3lf   %7.5lf   %10i\n",
		stats->null->num, stats->null->vol, stats->null->perc,
		stats->null->count);
    }

    if (!counts_only) {
	fprintf(stdout,
		"\nSum of non Null cells: \n\tVolume = %13.3lf \n\tPercentage = %7.3lf  \n\tCell count = %i\n",
		stats->sum_vol, stats->sum_perc, stats->sum_count);

	fprintf(stdout,
		"\nSum of all cells: \n\tVolume = %13.3lf \n\tPercentage = %7.3lf  \n\tCell count = %i\n",
		stats->sum_vol + stats->null->vol,
		stats->sum_perc + stats->null->perc,
		stats->sum_count + stats->null->count);
    }

    return;
}

/* *************************************************************** */
/*  Make an entry in the statistic table based on range value check */
/* *************************************************************** */
void check_range_value(stat_table *stats, double value)
{

    /* this is very expensive for large range arrays! */
    /*
     * int i;
     * for (i = 0; i < stats->nsteps; i++) {
     * if (value >= stats->table[i]->min && value < stats->table[i]->max) {
     * stats->table[i]->count++;
     * }
     * }
     */

    /* the much faster tree search is used */
    tree_search_range(stats, 0, stats->nsteps - 1, value);

    return;
}

/* *************************************************************** */
/*  tree search method developed by Soeren Gebbert *************** */
/* *************************************************************** */
/*
 * This is a divide and conquer tree search algorithm
 *
 * The algorithm counts how many values are located in each range.
 * It divides therefor the range array in smaller pices.
 *
 * e.g:
 *    0     1     2     3     4     5     6     7     8 
 * [[0,1],[1,2],[2,3],[3,4],[4,5],[5,6],[6,7],[7,8],[8,9]]
 *                         /   \
 * [[0,1],[1,2],[2,3],[3,4]]   [[4,5],[5,6],[6,7],[7,8],[8,9]]
 *             /   \                           /   \
 * [[0,1],[1,2]]   [[2,3],[3,4]]   [[4,5],[5,6]]   [[6,7],[7,8],[8,9]]
 *       /  \             /  \               / \               /  \
 * [[0,1]]  [[1,2]]  [[2,3]]  [[3,4]]  [[4,5]]  [[5,6]]  [[6,7]]  [[7,8],[8,9]]
 *                                                                     / \ 
 *                                                               [[7,8]]  [[8,9]]
 *
 * Searching the value 5.5 will result in the following steps:
 *
 * value 5.5 in range of
 * 1.) left array index = 0 - right array index = 8 -> range [0 - 9]
 * 2.) left array index = 4 - right array index = 8 -> range [4 - 9]
 * 3.) left array index = 4 - right array index = 5 -> range [4 - 6]
 * 4.) the value is located in range array 5 with a range of [5 - 6]
 * 5.) now increase the value range counter of range array with index 5
 *
 * */
void tree_search_range(stat_table *stats, int left, int right, double value)
{
    int size = right - left;

    G_debug(5,
	    "Search value %g in array size %i left border index %i right border index %i\n",
	    value, size, left, right);

    /*if left and right are equal */
    if (size == 0) {
	stats->table[left]->count++;
	return;
    }
    else if (size == 1) {	/* if the size is one, check directly */

	if (value >= stats->table[left]->min &&
	    value < stats->table[left]->max) {
	    stats->table[left]->count++;
	}
	else if (value >= stats->table[right]->min &&
		 value < stats->table[right]->max) {
	    stats->table[right]->count++;
	}
	return;
    }
    else {
	if ((size % 2 == 0)) {	/*even */
	    /*left side */
	    right = left + (size) / 2;

	    if (value >= stats->table[left]->min &&
		value < stats->table[right]->max) {
		tree_search_range(stats, left, right, value);
		return;
	    }

	    /*right side */
	    left += (size) / 2;
	    right = left + (size) / 2;

	    if (value >= stats->table[left]->min &&
		value < stats->table[right]->max) {
		tree_search_range(stats, left, right, value);
		return;
	    }
	}
	else {			/*odd */
	    /*left side */
	    right = left + (size - 1) / 2;

	    if (value >= stats->table[left]->min &&
		value < stats->table[right]->max) {
		tree_search_range(stats, left, right, value);
		return;
	    }

	    /*right side */
	    left += (size - 1) / 2;
	    right = left + (size - 1) / 2 + 1;	/*the right array is one col larger */

	    if (value >= stats->table[left]->min &&
		value < stats->table[right]->max) {
		tree_search_range(stats, left, right, value);
		return;
	    }
	}
    }

    return;
}

/* *************************************************************** */
/* ****** heapsort for equal value arrays of size n ************** */
/* ** Code based on heapsort from Sebastian Cyris **************** */
/* *************************************************************** */
void heapsort_eqvals(equal_val_array *e, int n)
{
    int k;
    double t;
    int count = 0;

    --n;

    for (k = n / 2; k >= 0; k--)
	downheap_eqvals(e, n, k);

    while (n > 0) {
	t = e->values[0]->val;
	count = e->values[0]->count;
	e->values[0]->val = e->values[n]->val;
	e->values[0]->count = e->values[n]->count;
	e->values[n]->val = t;
	e->values[n]->count = count;
	downheap_eqvals(e, --n, 0);
    }
    return;
}


/* *************************************************************** */
/* ** Code based on heapsort from Sebastian Cyris **************** */
/* ** ************************************************************ */
void downheap_eqvals(equal_val_array *e, int n, int k)
{
    int j;
    double v;
    int count;

    v = e->values[k]->val;
    count = e->values[k]->count;
    while (k <= n / 2) {
	j = k + k;

	if (j < n && e->values[j]->val < e->values[j + 1]->val)
	    j++;
	if (v >= e->values[j]->val)
	    break;

	e->values[k]->val = e->values[j]->val;
	e->values[k]->count = e->values[j]->count;
	k = j;
    }

    e->values[k]->val = v;
    e->values[k]->count = count;

    return;
}
