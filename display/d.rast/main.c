
/****************************************************************************
 *
 * MODULE:       d.rast
 * AUTHOR(S):    Jim Westervelt (CERL) (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>, 
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:      display raster maps in active graphics display
 * COPYRIGHT:    (C) 1999-2006, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>

#include "mask.h"
#include "local_proto.h"
#include <grass/glocale.h>

static int parse_catlist(char **, Mask *);
static int parse_vallist(char **, d_Mask *);

d_Mask d_mask;
Mask mask;

int main(int argc, char **argv)
{
    char *name;
    int overlay;
    int invert, fp;
    struct GModule *module;
    struct Option *map;
    struct Option *vallist;
    struct Option *bg;
    struct Flag *flag_n;
    struct Flag *flag_i;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("raster"));
    module->description = _("Displays user-specified raster map in the active "
			    "graphics frame.");
    
    /* set up command line */
    map = G_define_standard_option(G_OPT_R_MAP);
    map->description = _("Name of raster map to be displayed");

    vallist = G_define_option();
    vallist->key = "values";
    vallist->key_desc = "value[-value]";
    vallist->type = TYPE_STRING;
    vallist->required = NO;
    vallist->multiple = YES;
    vallist->description = _("List of categories or values to be displayed");
    vallist->guisection = _("Selection");

    bg = G_define_standard_option(G_OPT_C);
    bg->key = "bgcolor";
    bg->key_desc = "color";
    bg->answer = DEFAULT_BG_COLOR;
    bg->label = _("Background color (for null)");
    bg->guisection = _("Null cells");

    flag_n = G_define_flag();
    flag_n->key = 'n';
    flag_n->description = _("Make null cells opaque");
    flag_n->guisection = _("Null cells");

    flag_i = G_define_flag();
    flag_i->key = 'i';
    flag_i->description = _("Invert value list");
    flag_i->guisection = _("Selection");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = map->answer;
    overlay = !flag_n->answer;
    invert = flag_i->answer;

    D_open_driver();

    fp = Rast_map_is_fp(name, "");
    if (vallist->answer) {
	if (fp)
	    parse_vallist(vallist->answers, &d_mask);
	else
	    parse_catlist(vallist->answers, &mask);
    }

    /* use DCELL even if the map is FCELL */
    display(name, overlay, bg->answer, fp ? DCELL_TYPE : CELL_TYPE, invert);
    
    D_save_command(G_recreate_command());
    D_close_driver();
    
    exit(EXIT_SUCCESS);
}

static int parse_catlist(char **catlist, Mask * mask)
{
    char buf[1024];
    char x[2];
    FILE *fd;

    init_mask_rules(mask);
    if (catlist == NULL)
	return 0;

    for (; *catlist; catlist++) {
	if (*catlist[0] == '/') {
	    fd = fopen(*catlist, "r");
	    if (fd == NULL) {
		perror(*catlist);
		G_usage();
		exit(EXIT_FAILURE);
	    }
	    while (fgets(buf, sizeof buf, fd)) {
		if (sscanf(buf, "%1s", x) != 1 || *x == '#')
		    continue;
		parse_mask_rule(buf, mask, *catlist);
	    }
	    fclose(fd);
	}
	else
	    parse_mask_rule(*catlist, mask, (char *)NULL);
    }

    return 0;
}

static int parse_vallist(char **vallist, d_Mask * d_mask)
{
    char buf[1024];
    char x[2];
    FILE *fd;

    init_d_mask_rules(d_mask);
    if (vallist == NULL)
	return -1;

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
		parse_d_mask_rule(buf, d_mask, *vallist);
	    }
	    fclose(fd);
	}
	else
	    parse_d_mask_rule(*vallist, d_mask, (char *)NULL);
    }

    return 0;
}

int parse_mask_rule(char *catlist, Mask * mask, char *where)
{
    long a, b;
    char junk[128];

    /* #-# */
    if (sscanf(catlist, "%ld-%ld", &a, &b) == 2)
	add_mask_rule(mask, a, b, 0);

    /* inf-# */
    else if (sscanf(catlist, "%[^ -\t]-%ld", junk, &a) == 2)
	add_mask_rule(mask, a, a, -1);

    /* #-inf */
    else if (sscanf(catlist, "%ld-%[^ \t]", &a, junk) == 2)
	add_mask_rule(mask, a, a, 1);

    /* # */
    else if (sscanf(catlist, "%ld", &a) == 1)
	add_mask_rule(mask, a, a, 0);

    else {
	if (where)
	    fprintf(stderr, "%s: ", where);
	G_usage();
	G_fatal_error(_("[%s]: illegal category specified"), catlist);
    }

    return 0;
}

int parse_d_mask_rule(char *vallist, d_Mask * d_mask, char *where)
{
    double a, b;
    char junk[128];

    /* #-# */
    if (sscanf(vallist, "%lf-%lf", &a, &b) == 2)
	add_d_mask_rule(d_mask, a, b, 0);

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
	    fprintf(stderr, "%s: ", where);
	G_usage();
	G_fatal_error(_("[%s]: illegal value specified"), vallist);
    }

    return 0;
}
