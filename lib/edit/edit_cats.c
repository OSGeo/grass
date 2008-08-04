
/****************************************************************************
 *
 * MODULE:       edit library functions
 * AUTHOR(S):    Originally part of gis lib dir in CERL GRASS code
 *               Subsequent (post-CVS) contributors:
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Eric G. Miller <egm2 jps.net>,
 *               Markus Neteler <neteler itc.it>,
 *               Brad Douglas <rez touchofmadness.com>,
 *               Bernhard Reiter <bernhard intevation.de>
 * PURPOSE:      libraries for interactively editing raster support data
 * COPYRIGHT:    (C) 1996-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/**********************************************************************
 *
 *   E_edit_cats (name, cats, option)
 *      char *name
 *      struct Categories *cats 
 *
 *   Interactively prompts the user for category names for
 *   cats->ncats categories.  Uses screen oriented prompting through
 *   the visual_ask library.  Compile with $(VASKLIB) and $(CURSES)
 *
 *   name is used for informatin on the screen only.
 *   No files are read or written
 *
 *   If option is 0, user can change number of cats
 *                1, user must initialize number of cats
 *               -1, user may not change number of cats
 *
 *
 *   Returns:
 *            -1 if user canceled the edit
 *             1 if ok
 *
 *   note:
 *      at present, this routine pretends to know nothing about the
 *      category label generation capabilities using the cats.fmt
 *      string. If it is necessary to let the user edit this
 *      a separate interface must be used
 **********************************************************************/
#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vask.h>
#include <grass/edit.h>


#define NLINES 10


int E_edit_cats(const char *name, struct Categories *cats, int option)
{
    long incr;
    long atnum;
    long startcat;
    long endcat;
    long first_cat, last_cat;
    long catnum[NLINES];
    char buff[NLINES][80];
    char next[20];
    char next_line[80];
    char title[80];
    char msg1[80];
    char msg2[80];
    int line;
    CELL max_ind, max_cat, min_ind;
    DCELL max_val, min_val;

    if (G_quant_get_limits
	(&cats->q, &min_val, &max_val, &min_ind, &max_ind) < 0)
	max_cat = 0;
    else
	max_cat = (CELL) max_val;

    if (max_cat < 0)
	option = 1;

    last_cat = (long)max_cat;
    if (option >= 0) {
	V_clear();
	V_line(1, msg1);
	V_line(2, msg2);

	if (option == 0) {
	    strcpy(msg1, "The current value for the highest category");
	    sprintf(msg2, "in [%s] is shown below", name);
	    V_line(3, "If you need to change it, enter another value");
	}
	else {
	    last_cat = 0;
	    strcpy(msg1, "Please enter the highest category value");
	    sprintf(msg2, "for [%s]", name);
	}

	V_line(10, "         Highest Category:");
	V_ques(&last_cat, 'l', 10, 28, 5);
	V_line(16, next_line);

	*next_line = 0;
	while (1) {
	    V_intrpt_ok();
	    if (!V_call())
		return -1;

	    if (last_cat >= 0)
		break;

	    sprintf(next_line, "** Negative values not allowed **");
	}
    }

    max_cat = last_cat;

    first_cat = 0;
    if (cats->ncats > 0 && min_val < 0)
	first_cat = (CELL) min_val;

    *title = 0;
    if (cats->title != NULL)
	strcpy(title, cats->title);

    startcat = first_cat;
    sprintf(msg1, "[%s] ENTER NEW CATEGORY NAMES FOR THESE CATEGORIES", name);

    while (1) {
	V_clear();
	V_line(0, msg1);
	V_line(2, "TITLE: ");
	V_line(3, "CAT   NEW CATEGORY NAME");
	V_line(4, "NUM");

	V_ques(title, 's', 2, 8, 60);

	endcat =
	    startcat + NLINES <=
	    last_cat + 1 ? startcat + NLINES : last_cat + 1;

	line = 5;
	for (incr = startcat; incr < endcat; incr++) {
	    atnum = incr - startcat;
	    strcpy(buff[atnum], G_get_cat((CELL) incr, cats));
	    catnum[atnum] = incr;
	    V_const(&catnum[atnum], 'l', line, 1, 3);
	    V_ques(buff[atnum], 's', line, 5, 60);
	    line++;
	}

	line += 2;
	*next = 0;
	if (endcat > last_cat)
	    strcpy(next, "end");
	else
	    sprintf(next, "%ld", endcat);
	sprintf(next_line, "%*s%*s  (of %ld)", 41,
		"Next category ('end' to end): ", 5, "", last_cat);
	V_line(line, next_line);
	V_ques(next, 's', line, 41, 5);

	V_intrpt_ok();
	if (!V_call())
	    return -1;

	/* store new category name in structure */
	for (incr = startcat; incr < endcat; incr++) {
	    atnum = incr - startcat;
	    G_strip(buff[atnum]);
	    if (strcmp(buff[atnum], G_get_cat((CELL) incr, cats)) != 0)
		G_set_cat((CELL) incr, buff[atnum], cats);
	}

	if (*next == 0)
	    break;
	if (strcmp(next, "end") == 0)
	    break;
	if (sscanf(next, "%ld", &endcat) != 1)
	    continue;

	if (endcat < first_cat)
	    endcat = first_cat;

	if (endcat > last_cat) {
	    endcat = last_cat - NLINES + 1;
	    if (endcat < 0)
		endcat = 0;
	}
	startcat = endcat;
    }
    if (cats->title)
	G_free(cats->title);

    G_strip(title);
    cats->title = G_store(title);

    return (1);
}


/**********************************************************************
 *
 *   E_edit_fp_cats (name, cats)
 *      char *name
 *      struct Categories *cats 
 *
 *   Interactively prompts the user for category names for
 *   fp ranges of data.  Uses screen oriented prompting through
 *   the visual_ask library.  Compile with $(VASKLIB) and $(CURSES)
 *
 *   name is used for informatin on the screen only.
 *   No files are read or written
 *
 *   Returns:
 *            -1 if user canceled the edit
 *             1 if ok
 *
 *   note:
 *      at present, this routine pretends to know nothing about the
 *      category label generation capabilities using the cats.fmt
 *      string. If it is necessary to let the user edit this
 *      a separate interface must be used
 **********************************************************************/

#define NLINES 10

int E_edit_fp_cats(const char *name, struct Categories *cats)
{
    long incr;
    long atnum;
    long startcat;
    long endcat;
    char buff[NLINES][60];
    char next[20];
    char next_line[80];
    char title[80];
    char msg1[80];
    char msg2[80];
    int line, ncats;
    size_t lab_len;
    DCELL max_val[NLINES], min_val[NLINES];
    DCELL dmin, dmax;
    CELL tmp_cell;
    struct Categories old_cats;
    struct FPRange fp_range;

    if (G_read_fp_range(name, G_mapset(), &fp_range) < 0)
	G_fatal_error("can't read the floating point range for %s", name);

    G_get_fp_range_min_max(&fp_range, &dmin, &dmax);
    /* first save old cats */
    G_copy_raster_cats(&old_cats, cats);

    G_init_raster_cats(old_cats.title, cats);
    G_free_raster_cats(cats);

    ncats = old_cats.ncats;
    V_clear();

    if (!ncats)
	sprintf(msg1, "There are no predefined fp ranges to label");
    else
	sprintf(msg1, "There are %d predefined fp ranges to label", ncats);
    sprintf(msg2, "Enter the number of fp ranges you want to label");

    V_line(1, msg1);
    V_line(2, msg2);
    V_ques(&ncats, 'l', 2, 48, 5);
    V_line(16, next_line);
    *next_line = 0;
    V_intrpt_ok();

    if (!V_call())
	return -1;

    *title = 0;
    if (old_cats.title != NULL)
	strcpy(title, old_cats.title);

    startcat = 0;
    sprintf(msg1, "The fp data in map %s ranges from %f to %f", name, dmin,
	    dmax);
    sprintf(msg2, "[%s] ENTER NEW CATEGORY NAMES FOR THESE CATEGORIES", name);

    while (1) {
	V_clear();
	V_line(0, msg1);
	V_line(1, msg2);
	V_line(3, "TITLE: ");
	V_line(4, "FP RANGE           NEW CATEGORY NAME");

	V_ques(title, 's', 2, 8, 60);

	endcat = startcat + NLINES <= ncats ? startcat + NLINES : ncats;

	line = 6;
	for (incr = startcat; incr < endcat; incr++) {
	    atnum = incr - startcat;
	    if (incr < old_cats.ncats) {
		/* if editing existing range label */
		lab_len = strlen(old_cats.labels[incr]);
		if (lab_len > 58)
		    lab_len = 58;
		strncpy(buff[atnum], old_cats.labels[incr], lab_len);
		buff[atnum][lab_len] = 0;
		G_quant_get_ith_rule(&old_cats.q, incr, &min_val[atnum],
				     &max_val[atnum], &tmp_cell, &tmp_cell);
	    }
	    else {
		/* adding new range label */
		strcpy(buff[atnum], "");
		max_val[atnum] = min_val[atnum] = 0;
	    }

	    V_ques(&min_val[atnum], 'd', line, 1, 8);
	    V_const("-", 's', line, 9, 1);
	    V_ques(&max_val[atnum], 'd', line, 10, 8);
	    V_ques(buff[atnum], 's', line, 19, 58);
	    line++;
	}

	line += 2;
	*next = 0;
	if (endcat >= ncats)
	    strcpy(next, "end");
	else
	    sprintf(next, "%ld", endcat);
	sprintf(next_line, "%*s%*s  (of %d)", 41,
		"Next range number ('end' to end): ", 5, "", ncats);
	V_line(line, next_line);
	V_ques(next, 's', line, 41, 5);

	V_intrpt_ok();
	if (!V_call())
	    return -1;

	/* store new category name in structure */
	for (incr = startcat; incr < endcat; incr++) {
	    atnum = incr - startcat;
	    G_strip(buff[atnum]);

	    /* adding new range label */
	    if (!(strcmp(buff[atnum], "") == 0 &&
		  min_val[atnum] == 0. && max_val[atnum] == 0.))
		G_set_d_raster_cat(&min_val[atnum], &max_val[atnum],
				   buff[atnum], cats);
	}

	if (*next == 0)
	    break;
	if (strcmp(next, "end") == 0)
	    break;
	if (sscanf(next, "%ld", &endcat) != 1)
	    continue;

	if (endcat < 0)
	    endcat = 0;

	if (endcat > ncats) {
	    endcat = ncats - NLINES + 1;
	    if (endcat < 0)
		endcat = 0;
	}
	startcat = endcat;
    }

    G_strip(title);
    cats->title = G_store(title);
    /* since label pointers in old_cats point to the old allocated strings,
       and cats now has all the newly allocated strings, it never reuses
       old ones, we need to free them */

    return (1);
}
