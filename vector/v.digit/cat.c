/* ***************************************************************
 * *
 * * MODULE:       v.digit
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               
 * * PURPOSE:      Edit vector
 * *              
 * * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 * *
 * *               This program is free software under the 
 * *               GNU General Public License (>=v2). 
 * *               Read the file COPYING that comes with GRASS
 * *               for details.
 * *
 * **************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "global.h"
#include "proto.h"


/* Init cats */
void cat_init(void)
{
    int i, line, nlines;
    struct line_cats *Cats;

    G_debug(2, "cat_init()");
    Cats = Vect_new_cats_struct();

    /* Max cats */
    nMaxFieldCat = 0;
    aMaxFieldCat = 10;		/* allocated space */
    MaxFieldCat = (void *)G_malloc((aMaxFieldCat) * sizeof(int) * 2);

    /* Read the map and set maximum categories */
    nlines = Vect_get_num_lines(&Map);
    for (line = 1; line <= nlines; line++) {
	Vect_read_line(&Map, NULL, Cats, line);
	for (i = 0; i < Cats->n_cats; i++) {
	    if ((cat_max_get(Cats->field[i])) < Cats->cat[i]) {
		cat_max_set(Cats->field[i], Cats->cat[i]);
	    }
	}
    }
}

/* get maximum cat for field */
int cat_max_get(int field)
{
    int i;

    G_debug(2, "cat_max_get() field = %d", field);

    for (i = 0; i < nMaxFieldCat; i++) {
	if (MaxFieldCat[i][0] == field) {
	    return (MaxFieldCat[i][1]);
	}
    }

    return 0;
}

/* set maximum cat for field */
void cat_max_set(int field, int cat)
{
    int i;

    G_debug(2, "cat_max_set() field = %d cat = %d", field, cat);

    for (i = 0; i < nMaxFieldCat; i++) {
	if (MaxFieldCat[i][0] == field) {
	    MaxFieldCat[i][1] = cat;
	    return;
	}
    }
    /* Field not found -> add new */
    if (nMaxFieldCat == aMaxFieldCat) {
	aMaxFieldCat += 10;
	MaxFieldCat =
	    (void *)G_realloc(MaxFieldCat, (aMaxFieldCat) * sizeof(int) * 2);
    }
    MaxFieldCat[nMaxFieldCat][0] = field;
    MaxFieldCat[nMaxFieldCat][1] = cat;
    nMaxFieldCat++;
}
