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

struct symb
{
    char *name;
    int code;
    int r, g, b;		/* default colors */
};

struct symb default_symb_table[] = {
    {"background", SYMB_BACKGROUND, 255, 255, 255},	/* white */
    {"highlight", SYMB_HIGHLIGHT, 255, 255, 0},	/* yellow */
    {"point", SYMB_POINT, 0, 0, 0},	/* black */
    {"line", SYMB_LINE, 0, 0, 0},	/* black */
    {"boundary_0", SYMB_BOUNDARY_0, 153, 153, 153},	/* grey */
    {"boundary_1", SYMB_BOUNDARY_1, 255, 125, 0},	/* orange */
    {"boundary_2", SYMB_BOUNDARY_2, 0, 255, 0},	/* green */
    {"centroid_in", SYMB_CENTROID_IN, 0, 0, 153},	/* dark blue */
    {"centroid_out", SYMB_CENTROID_OUT, 153, 153, 0},	/* mustard */
    {"centroid_dupl", SYMB_CENTROID_DUPL, 255, 0, 255},	/* magenta */
    {"node_1", SYMB_NODE_1, 255, 0, 0},	/* red */
    {"node_2", SYMB_NODE_2, 0, 153, 0},	/* dark green */
    {"", 0, 0, 0, 0}
};

/* Convert symbology  name to code */
int get_symb_code(char *name)
{
    int i;

    G_debug(2, "get_symb_code(): name = %s", name);

    for (i = 0; default_symb_table[i].name[0]; i++) {
	if (strcmp(name, default_symb_table[i].name) == 0) {
	    G_debug(2, "-> code = %d", default_symb_table[i].code);
	    return (default_symb_table[i].code);
	}
    }

    G_warning("get_symb_code(): symbol name %s does not exist", name);
    return (-1);
}

/* Returns pointer to symbology name */
char *get_symb_name(int code)
{
    G_debug(2, "get_symb_name(): code = %d", code);

    G_debug(2, "-> name = %s", default_symb_table[code].name);

    return (default_symb_table[code].name);
}

/* Init symbology: set defaults */
void symb_init(void)
{
    int i, code;

    for (i = 0; default_symb_table[i].name[0]; i++) {
	code = default_symb_table[i].code;
	Symb[code].on = 1;
	Symb[code].r = default_symb_table[i].r;
	Symb[code].g = default_symb_table[i].g;
	Symb[code].b = default_symb_table[i].b;
    }
}

/* Synchronize GUI */
void symb_init_gui(void)
{
    int i, code;

    for (i = 0; default_symb_table[i].name[0]; i++) {
	code = get_symb_code(default_symb_table[i].name);
	i_set_color(default_symb_table[i].name, Symb[code].r, Symb[code].g,
		    Symb[code].b);
	i_set_on(default_symb_table[i].name, Symb[code].on);
    }
}

/* Set driver color */
void symb_set_driver_color(int code)
{
    G_debug(2, "set color to symb %d: %d %d %d", code, Symb[code].r,
	    Symb[code].g, Symb[code].b);

    driver_rgb_color(Symb[code].r, Symb[code].g, Symb[code].b);
}


/* --- LINES --- */
/* Get line symbology from map */
int symb_line_from_map(int line)
{
    int type, area1, area2, nareas;

    G_debug(2, "line_symb_from_map(): line = %d", line);

    type = Vect_read_line(&Map, NULL, NULL, line);

    switch (type) {
    case GV_POINT:
	return SYMB_POINT;
    case GV_LINE:
	return SYMB_LINE;
    case GV_BOUNDARY:
	nareas = 0;
	Vect_get_line_areas(&Map, line, &area1, &area2);
	/* Count areas/isles on both sides */
	if (area1 != 0)
	    nareas++;
	if (area2 != 0)
	    nareas++;
	G_debug(2, "  boundary = %d nareas = %d", line, nareas);
	if (nareas == 0)
	    return SYMB_BOUNDARY_0;
	else if (nareas == 1)
	    return SYMB_BOUNDARY_1;
	else
	    return SYMB_BOUNDARY_2;
    case GV_CENTROID:
	area1 = Vect_get_centroid_area(&Map, line);
	G_debug(2, "  centroid = %d area = %d", line, area1);
	if (area1 == 0)
	    return SYMB_CENTROID_OUT;
	else if (area1 > 0)
	    return SYMB_CENTROID_IN;
	else
	    return SYMB_CENTROID_DUPL;	/* area1 < 0 */
    }

    /* Should not be reached */
    return SYMB_HIGHLIGHT;
}

/* Init line symbology table */
void symb_lines_init(void)
{
    int i;

    G_debug(2, "symb_line_init()");

    /* Lines */
    aLineSymb = Vect_get_num_lines(&Map) + 1000;	/* allocated space */
    LineSymb = (int *)G_malloc((aLineSymb + 1) * sizeof(int));
    for (i = 1; i <= Vect_get_num_lines(&Map); i++)
	LineSymb[i] = symb_line_from_map(i);
}

/* Set symbology for existing or new line */
void symb_line_set_from_map(int line)
{
    G_debug(2, "line_symb_refresh()");

    if (line > aLineSymb) {
	aLineSymb = line + 1000;
	LineSymb = (int *)G_realloc(LineSymb, (aLineSymb + 1) * sizeof(int));
    }
    LineSymb[line] = symb_line_from_map(line);
}

/* Update the symbology of lines changed by last write access to the map */
void symb_updated_lines_set_from_map(void)
{
    int i, line;

    G_debug(2, "symb_updated_lines_set_from_map();");

    for (i = 0; i < Vect_get_num_updated_lines(&Map); i++) {
	line = Vect_get_updated_line(&Map, i);
	if (!Vect_line_alive(&Map, line))
	    continue;
	symb_line_set_from_map(line);
    }
}

/* --- NODES --- */
/* Get node symbology from map */
int symb_node_from_map(int node)
{
    int i, nl, nlines, line, type;

    nlines = 0;
    nl = Vect_get_node_n_lines(&Map, node);

    G_debug(2, "node = %d nl = %d", node, nl);
    for (i = 0; i < nl; i++) {
	line = abs(Vect_get_node_line(&Map, node, i));
	G_debug(2, "i = %d line = %d", i, line);
	if (!Vect_line_alive(&Map, line))
	    continue;
	type = Vect_read_line(&Map, NULL, NULL, line);
	if (type & GV_LINES)
	    nlines++;
    }

    G_debug(2, "node = %d nlines = %d", node, nlines);
    if (nlines == 0)
	return SYMB_NODE_0;
    else if (nlines == 1)
	return SYMB_NODE_1;
    else
	return SYMB_NODE_2;
}

/* Add node symbology for new lines if necessary */
void symb_node_set_from_map(int node)
{
    G_debug(2, "line_symb_refresh()");

    if (node > aNodeSymb) {
	aNodeSymb = node + 1000;
	NodeSymb = (int *)G_realloc(NodeSymb, (aNodeSymb + 1) * sizeof(int));
    }
    NodeSymb[node] = symb_node_from_map(node);
}

/* Init node symbology table */
void symb_nodes_init(void)
{
    int i;

    G_debug(2, "symb_node_init()");

    /* Nodes */
    aNodeSymb = Vect_get_num_nodes(&Map) + 1000;	/* allocated space */
    NodeSymb = (int *)G_malloc((aNodeSymb + 1) * sizeof(int));
    for (i = 1; i <= Vect_get_num_nodes(&Map); i++)
	NodeSymb[i] = symb_node_from_map(i);
}

/* Update the symbology of nodes changed by last write access to the map */
void symb_updated_nodes_set_from_map(void)
{
    int i, node;

    G_debug(2, "node_update();");

    for (i = 0; i < Vect_get_num_updated_nodes(&Map); i++) {
	node = Vect_get_updated_node(&Map, i);
	if (!Vect_node_alive(&Map, node))
	    continue;
	symb_node_set_from_map(node);
    }
}

void updated_lines_and_nodes_erase_refresh_display(void)
{
    /* Note: this is a problem if new line is point, and there is a raster on the background, its displays
     *        (erase) node over the raster, which is a bit confusing */
    display_updated_nodes(SYMB_BACKGROUND);	/* because the size/shape may decrease for new symbol */

    symb_updated_lines_set_from_map();
    symb_updated_nodes_set_from_map();
    display_updated_nodes(SYMB_DEFAULT);
    display_updated_lines(SYMB_DEFAULT);
}
