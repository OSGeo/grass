
/****************************************************************************
 *
 * MODULE:       r.describe
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Prints terse list of category values found in a raster
 *               map layer.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>

static int show(CELL, CELL, int *, DCELL, DCELL, RASTER_MAP_TYPE, int);


int long_list(struct Cell_stats *statf,
	      DCELL dmin, DCELL dmax,
	      char *no_data_str, int skip_nulls,
	      RASTER_MAP_TYPE map_type, int nsteps)
{
    CELL cat;
    long count;			/* not used, but required by cell stats call */

    Rast_get_stats_for_null_value(&count, statf);
    if (count != 0 && !skip_nulls)
	fprintf(stdout, "%s\n", no_data_str);

    while (Rast_next_cell_stat(&cat, &count, statf)) {
	if (map_type != CELL_TYPE)
	    fprintf(stdout, "%f-%f\n",
		    dmin + (double)(cat - 1) * (dmax - dmin) / nsteps,
		    dmin + (double)cat * (dmax - dmin) / nsteps);
	else
	    fprintf(stdout, "%ld\n", (long)cat);
    }
    return (0);
}


int compact_list(struct Cell_stats *statf,
		 DCELL dmin, DCELL dmax,
		 char *no_data_str, int skip_nulls,
		 RASTER_MAP_TYPE map_type, int nsteps)
{
    CELL cat1, cat2, temp;
    int len;
    long count;			/* not used, but required by cell stats call */

    len = 0;
    Rast_get_stats_for_null_value(&count, statf);
    if (count != 0 && !skip_nulls)
	fprintf(stdout, "%s ", no_data_str);

    if (!Rast_next_cell_stat(&cat1, &count, statf))
	/* map doesn't contain any non-null data */
	return 1;

    cat2 = cat1;
    while (Rast_next_cell_stat(&temp, &count, statf)) {
	if (temp != cat2 + (CELL) 1) {
	    show(cat1, cat2, &len, dmin, dmax, map_type, nsteps);
	    cat1 = temp;
	}
	cat2 = temp;
    }
    show(cat1, cat2, &len, dmin, dmax, map_type, nsteps);
    fprintf(stdout, "\n");
    return (0);
}


static int show(CELL low, CELL high, int *len,
		DCELL dmin, DCELL dmax, RASTER_MAP_TYPE map_type, int nsteps)
{
    char text[100];
    char xlen;

    if (low + 1 == high) {
	show(low, low, len, dmin, dmax, map_type, nsteps);
	show(high, high, len, dmin, dmax, map_type, nsteps);
	return 0;
    }

    if (map_type != CELL_TYPE) {
	sprintf(text, "%f%s%f ", dmin + (low - 1) * (dmax - dmin) / nsteps,
		dmin < 0 ? " thru " : "-",
		dmin + high * (dmax - dmin) / nsteps);
    }
    else {
	if (low == high)
	    sprintf(text, "%ld ", (long)low);
	else
	    sprintf(text, "%ld%s%ld ", (long)low, low < 0 ? " thru " : "-",
		    (long)high);
    }

    xlen = strlen(text);
    if (xlen + *len > 78) {
	fprintf(stdout, "\n");
	*len = 0;
    }
    fprintf(stdout, "%s", text);
    *len += xlen;
    return (0);
}


int compact_range_list(CELL negmin, CELL negmax, CELL zero, CELL posmin,
		       CELL posmax, CELL null, char *no_data_str,
		       int skip_nulls)
{
    if (negmin) {
	fprintf(stdout, "%ld", (long)negmin);
	if (negmin != negmax)
	    fprintf(stdout, " thru %ld", (long)negmax);
	fprintf(stdout, "\n");
    }
    if (zero)
	fprintf(stdout, "0\n");
    if (posmin) {
	fprintf(stdout, "%ld", (long)posmin);
	if (posmin != posmax)
	    fprintf(stdout, " thru %ld", (long)posmax);
	fprintf(stdout, "\n");
    }

    if (null && !skip_nulls)
	fprintf(stdout, "%s\n", no_data_str);

    return (0);
}


int range_list(CELL negmin, CELL negmax, CELL zero, CELL posmin, CELL posmax,
	       CELL null, char *no_data_str, int skip_nulls)
{
    if (negmin) {
	fprintf(stdout, "%ld\n", (long)negmin);
	if (negmin != negmax)
	    fprintf(stdout, "%ld\n", (long)negmax);
    }
    if (zero)
	fprintf(stdout, "0\n");
    if (posmin) {
	fprintf(stdout, "%ld\n", (long)posmin);
	if (posmin != posmax)
	    fprintf(stdout, "%ld\n", (long)posmax);
    }

    if (null && !skip_nulls)
	fprintf(stdout, "%s\n", no_data_str);

    return (0);
}
