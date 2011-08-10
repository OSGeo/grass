
/***************************************************************************
* MODULE:       this structs/functions are used by r3.mask and r3.null
*
* AUTHOR(S):    Roman Waupotitsch, Michael Shapiro, Helena Mitasova,
*		Bill Brown, Lubos Mitas, Jaro Hofierka
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/
/*Helperfunctions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

/*local prototypes */
static void add_d_mask_rule(d_Mask * d_mask, double a, double b, int inf);
static void parse_d_mask_rule(char *vallist, d_Mask * d_mask, char *where);
static void init_d_mask_rules(d_Mask * d_mask);

void init_d_mask_rules(d_Mask * d_mask)
{
    d_mask->list = NULL;
}

void add_d_mask_rule(d_Mask * d_mask, double a, double b, int inf)
{
    d_Interval *I;

    I = (d_Interval *) G_malloc(sizeof(d_Interval));
    I->low = a <= b ? a : b;
    I->high = a >= b ? a : b;
    I->inf = inf;
    I->next = d_mask->list;
    d_mask->list = I;
}

int Rast3d_mask_d_select(DCELL * x, d_Mask * mask)
{
    d_Interval *I;

    if (mask->list == NULL)
	return 0;
    for (I = mask->list; I; I = I->next) {
	if (Rast3d_mask_match_d_interval(*x, I))
	    return 1;
    }
    return 0;
}

DCELL Rast3d_mask_match_d_interval(DCELL x, d_Interval * I)
{
    if (I->inf < 0)
	return x <= I->low;

    if (I->inf > 0)
	return x >= I->high;

    return x >= I->low && x <= I->high;
}

void parse_d_mask_rule(char *vallist, d_Mask * d_mask, char *where)
{
    double a, b;
    char junk[128];

    /* #-# */
    if (sscanf(vallist, "%lf-%lf", &a, &b) == 2) {
	G_message(_("Adding rule: %lf - %lf"), a, b);
	add_d_mask_rule(d_mask, a, b, 0);
    }
    /* inf-# */
    else if (sscanf(vallist, "%[^ -\t]-%lf", junk, &a) == 2)
	add_d_mask_rule(d_mask, a, a, -1);

    /* #-inf */
    else if (sscanf(vallist, "%lf-%[^ \t]", &a, junk) == 2)
	add_d_mask_rule(d_mask, a, a, 1);

    /* # */
    else if (sscanf(vallist, "%lf", &a) == 1)
	add_d_mask_rule(d_mask, a, a, 0);

    else {
	if (where)
	    G_message("%s: ", where);
	G_warning(_("%s: illegal value spec"), vallist);
	G_usage();
	exit(EXIT_FAILURE);
    }
}

void Rast3d_parse_vallist(char **vallist, d_Mask ** d_mask)
{
    char buf[1024];
    char x[2];
    FILE *fd;

    *d_mask = (d_Mask *) G_malloc(sizeof(d_Mask));

    init_d_mask_rules(*d_mask);
    if (vallist == NULL)
	return;

    for (; *vallist; vallist++) {
	if (*vallist[0] == '/') {
	    fd = fopen(*vallist, "r");
	    if (fd == NULL) {
		perror(*vallist);
		G_usage();
		exit(EXIT_FAILURE);
	    }
	    while (fgets(buf, sizeof buf, fd)) {
		if (sscanf(buf, "%1s", x) != 1 || *x == '#')
		    continue;
		parse_d_mask_rule(buf, *d_mask, *vallist);
	    }
	    fclose(fd);
	}
	else
	    parse_d_mask_rule(*vallist, *d_mask, (char *)NULL);
    }
}
