/****************************************************************************
*
* MODULE:       Symbol library 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      Read symbol from a file to internal structure
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <grass/gis.h>
#include <grass/symbol.h>
#include <grass/glocale.h>

static char key[100], data[500];

/* Define currently processed part */
#define OBJ_NONE    0
#define OBJ_STRING  1
#define OBJ_POLYGON 2
#define OBJ_RING    3

/* stores input to key an data */
void get_key_data(char *buf)
{
    char *p;

    G_debug(3, "  get_key_data(): %s", buf);

    data[0] = '\0';

    strcpy(key, buf);
    p = strchr(key, ' ');
    if (p == NULL)
	return;

    p[0] = '\0';

    p++;
    if (strlen(p) > 0) {
	strcpy(data, p);
	G_chop(data);
    }
    G_debug(3, "  key = %s data = %s", key, data);
}

/* --- SYMBOL --- */
/* create new empty symbol */
SYMBOL *new_symbol(void)
{
    SYMBOL *p;

    p = (SYMBOL *) G_malloc(sizeof(SYMBOL));
    p->scale = 1.0;
    p->count = 0;
    p->alloc = 0;
    p->part = NULL;
    return p;
}

/* add part to symbol */
void add_part(SYMBOL * s, SYMBPART * p)
{
    if (s->count == s->alloc) {
	s->alloc += 10;
	s->part =
	    (SYMBPART **) G_realloc(s->part, s->alloc * sizeof(SYMBPART *));
    }
    s->part[s->count] = p;
    s->count++;
}

/* --- PART --- */
/* create new empty part */
SYMBPART *new_part(int type)
{
    SYMBPART *p;

    p = (SYMBPART *) G_malloc(sizeof(SYMBPART));
    p->type = type;
    p->count = 0;
    p->alloc = 0;
    p->chain = NULL;
    p->color.color = S_COL_DEFAULT;
    p->fcolor.color = S_COL_DEFAULT;
    return p;
}

/* add chain to part */
void add_chain(SYMBPART * p, SYMBCHAIN * s)
{
    if (p->count == p->alloc) {
	p->alloc += 10;
	p->chain =
	    (SYMBCHAIN **) G_realloc(p->chain,
				     p->alloc * sizeof(SYMBCHAIN *));
    }
    p->chain[p->count] = s;
    p->count++;
}


/* --- CHAIN --- */
/* create new empty chain */
SYMBCHAIN *new_chain(void)
{
    SYMBCHAIN *p;

    p = (SYMBCHAIN *) G_malloc(sizeof(SYMBCHAIN));
    p->count = 0;
    p->alloc = 0;
    p->elem = NULL;
    p->scount = 0;
    p->salloc = 0;
    p->sx = NULL;
    p->sy = NULL;
    return p;
}

/* add element to chain */
void add_element(SYMBCHAIN * s, SYMBEL * e)
{
    if (s->count == s->alloc) {
	s->alloc += 10;
	s->elem = (SYMBEL **) G_realloc(s->elem, s->alloc * sizeof(SYMBEL *));
    }
    s->elem[s->count] = e;
    s->count++;
}

/* --- ELEMENT --- */
/* create new empty line */
SYMBEL *new_line(void)
{
    SYMBEL *p;

    p = (SYMBEL *) G_malloc(sizeof(SYMBEL));
    p->type = S_LINE;
    p->coor.line.count = 0;
    p->coor.line.alloc = 0;
    p->coor.line.x = NULL;
    p->coor.line.y = NULL;
    return p;
}

/* add point to line */
void add_point(SYMBEL * el, double x, double y)
{
    if (el->coor.line.count == el->coor.line.alloc) {
	el->coor.line.alloc += 10;
	el->coor.line.x =
	    (double *)G_realloc(el->coor.line.x,
				el->coor.line.alloc * sizeof(double));
	el->coor.line.y =
	    (double *)G_realloc(el->coor.line.y,
				el->coor.line.alloc * sizeof(double));
    }
    el->coor.line.x[el->coor.line.count] = x;
    el->coor.line.y[el->coor.line.count] = y;
    el->coor.line.count++;
}

/* create new arc */
SYMBEL *new_arc(double x, double y, double r, double a1, double a2, int c)
{
    SYMBEL *p;

    p = (SYMBEL *) G_malloc(sizeof(SYMBEL));
    p->type = S_ARC;
    p->coor.arc.clock = c;
    p->coor.arc.x = x;
    p->coor.arc.y = y;
    p->coor.arc.r = r;
    p->coor.arc.a1 = a1;
    p->coor.arc.a2 = a2;
    return p;
}

/* read line coordinates */
void read_coor(FILE * fp, SYMBEL * e)
{
    char buf[501];
    double x, y;

    G_debug(5, "    read_coor()");

    while (G_getl2(buf, 500, fp) != 0) {
	G_chop(buf);

	/* skip empty and comment lines */
	if ((buf[0] == '#') || (buf[0] == '\0'))
	    continue;

	get_key_data(buf);

	if (strcmp(key, "END") == 0) {
	    G_debug(5, "    LINE END");
	    return;
	}

	if (sscanf(buf, "%lf %lf", &x, &y) != 2) {
	    G_warning(_("Cannot read symbol line coordinates: %s"), buf);
	    return;
	}
	G_debug(5, "      x = %f y = %f", x, y);
	add_point(e, x, y);
    }
}

/* close file free symbol, print message, return NULL */
SYMBOL *err(FILE * fp, SYMBOL * s, char *msg)
{
    fclose(fp);
    G_free(s);			/* TODO: free all */
    G_warning(msg);
    return NULL;
}

/* 
 *  Read symbol specified by name.
 *  Name: group/name | group/name@mapset 
 *        (later add syntax to prefer symbol from GISBASE)
 *  S_read() searches first in mapsets (standard GRASS search) and
 *   then in GISBASE/etc/symbol/ 
 */
SYMBOL *S_read(const char *sname)
{
    int i, j, k, l;
    FILE *fp;
    char group[500], name[500], buf[2001];
    const char *ms;
    char *c;
    double x, y, x2, y2, rad, ang1, ang2;
    int r, g, b;
    double fr, fg, fb;
    int ret;
    char clock;
    SYMBOL *symb;
    int current;		/* current part_type */
    SYMBPART *part;		/* current part */
    SYMBCHAIN *chain;		/* current chain */
    SYMBEL *elem;		/* current element */

    G_debug(3, "S_read(): sname = %s", sname);

    /* Find file */
    /* Get group and name */
    strcpy(group, sname);
    c = strchr(group, '/');
    if (c == NULL) {
	G_warning(_("Incorrect symbol name: '%s' (should be: group/name or group/name@mapset)"),
		  sname);
	return NULL;
    }
    c[0] = '\0';

    c++;
    strcpy(name, c);

    G_debug(3, "  group: '%s' name: '%s'", group, name);

    /* Search in mapsets */
    sprintf(buf, "symbol/%s", group);
    ms = G_find_file(buf, name, NULL);

    if (ms != NULL) {		/* Found in mapsets */
	fp = G_fopen_old(buf, name, ms);
    }
    else {			/* Search in GISBASE */
	sprintf(buf, "%s/etc/symbol/%s", G_gisbase(), sname);
	fp = fopen(buf, "r");
    }

    if (fp == NULL) {
	G_warning(_("Cannot find/open symbol: '%s'"), sname);
	return NULL;
    }

    /* create new symbol */
    symb = new_symbol();

    current = OBJ_NONE;		/* no part */

    /* read file */
    while (G_getl2(buf, 2000, fp) != 0) {
	G_chop(buf);
	G_debug(3, "  BUF: [%s]", buf);

	/* skip empty and comment lines */
	if ((buf[0] == '#') || (buf[0] == '\0'))
	    continue;

	get_key_data(buf);

	if (strcmp(key, "VERSION") == 0) {
	    if (strcmp(data, "1.0") != 0) {
		sprintf(buf, "Wrong symbol version: '%s'", data);
		return (err(fp, symb, buf));
	    }
	}
	else if (strcmp(key, "BOX") == 0) {
	    if (sscanf(data, "%lf %lf %lf %lf", &x, &y, &x2, &y2) != 4) {
		sprintf(buf, "Incorrect box definition: '%s'", data);
		return (err(fp, symb, buf));
	    }
	    if (x2 - x > y2 - y)
		symb->scale = 1 / (x2 - x);
	    else
		symb->scale = 1 / (y2 - y);
	}
	else if (strcmp(key, "STRING") == 0) {
	    G_debug(4, "  STRING >");
	    current = OBJ_STRING;
	    part = new_part(S_STRING);
	    add_part(symb, part);

	    chain = new_chain();
	    add_chain(part, chain);
	}
	else if (strcmp(key, "POLYGON") == 0) {
	    G_debug(4, "  POLYGON >");
	    current = OBJ_POLYGON;
	    part = new_part(S_POLYGON);
	    add_part(symb, part);

	}
	else if (strcmp(key, "RING") == 0) {
	    G_debug(4, "  RING >");
	    current = OBJ_RING;
	    chain = new_chain();
	    add_chain(part, chain);

	}
	else if (strcmp(key, "LINE") == 0) {
	    G_debug(4, "    LINE >");
	    elem = new_line();
	    add_element(chain, elem);
	    read_coor(fp, elem);

	}
	else if (strcmp(key, "ARC") == 0) {
	    G_debug(4, "    ARC");
	    ret =
		sscanf(data, "%lf %lf %lf %lf %lf %c", &x, &y, &rad, &ang1,
		       &ang2, &clock);
	    if (ret < 5) {
		sprintf(buf, "Incorrect arc definition: '%s'", buf);
		return (err(fp, symb, buf));
	    }
	    if (ret == 6 && (clock == 'c' || clock == 'C'))
		i = 1;
	    else
		i = 0;
	    elem = new_arc(x, y, rad, ang1, ang2, i);
	    add_element(chain, elem);

	}
	else if (strcmp(key, "END") == 0) {
	    switch (current) {
	    case OBJ_STRING:
		G_debug(4, "  STRING END");
		current = OBJ_NONE;
		break;
	    case OBJ_POLYGON:
		G_debug(4, "  POLYGON END");
		current = OBJ_NONE;
		break;
	    case OBJ_RING:
		G_debug(4, "  RING END");
		current = OBJ_POLYGON;
		break;
	    }
	}
	else if (strcmp(key, "COLOR") == 0) {
	    if (G_strcasecmp(data, "NONE") == 0) {
		part->color.color = S_COL_NONE;
	    }
	    else if (sscanf(data, "%d %d %d", &r, &g, &b) == 3) {
		if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
		    G_warning(_("Incorrect symbol color: '%s', using default."),
			      buf);
		else {
		    fr = r / 255.0;
		    fg = g / 255.0;
		    fb = b / 255.0;
		    part->color.color = S_COL_DEFINED;
		    part->color.r = r;
		    part->color.g = g;
		    part->color.b = b;
		    part->color.fr = fr;
		    part->color.fg = fg;
		    part->color.fb = fb;
		    G_debug(4, "  color [%d %d %d] = [%.3f %.3f %.3f]", r, g,
			    b, fr, fg, fb);
		}
	    }
	    else {
		G_warning(_("Incorrect symbol color: '%s', using default."),
			  buf);
	    }
	}
	else if (strcmp(key, "FCOLOR") == 0) {
	    if (G_strcasecmp(data, "NONE") == 0) {
		part->fcolor.color = S_COL_NONE;
	    }
	    else if (sscanf(data, "%d %d %d", &r, &g, &b) == 3) {
		if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
		    G_warning(_("Incorrect symbol color: '%s', using default."),
			      buf);
		else {
		    fr = r / 255.0;
		    fg = g / 255.0;
		    fb = b / 255.0;
		    part->fcolor.color = S_COL_DEFINED;
		    part->fcolor.r = r;
		    part->fcolor.g = g;
		    part->fcolor.b = b;
		    part->fcolor.fr = fr;
		    part->fcolor.fg = fg;
		    part->fcolor.fb = fb;
		    G_debug(4, "  color [%d %d %d] = [%.3f %.3f %.3f]", r, g,
			    b, fr, fg, fb);
		}
	    }
	    else {
		G_warning(_("Incorrect symbol color: '%s', using default."),
			  buf);
	    }
	}
	else {
	    sprintf(buf, "Unknown keyword in symbol: '%s'", buf);
	    return (err(fp, symb, buf));
	    break;
	}
    }

    /* Debug output */

    G_debug(3, "Number of parts: %d", symb->count);
    for (i = 0; i < symb->count; i++) {
	part = symb->part[i];
	G_debug(4, "  Part %d: type: %d number of chains: %d", i, part->type,
		part->count);
	G_debug(4, "           color: %d: fcolor: %d", part->color.color,
		part->fcolor.color);
	for (j = 0; j < part->count; j++) {
	    chain = part->chain[j];
	    G_debug(4, "    Chain %d: number of elements: %d", j,
		    chain->count);
	    for (k = 0; k < chain->count; k++) {
		elem = chain->elem[k];
		G_debug(4, "      Element %d: type: %d", k, elem->type);
		if (elem->type == S_LINE) {
		    G_debug(4, "        Number of points %d",
			    elem->coor.line.count);
		    for (l = 0; l < elem->coor.line.count; l++) {
			G_debug(4, "        x, y: %f %f",
				elem->coor.line.x[l], elem->coor.line.y[l]);
		    }
		}
		else {
		    G_debug(4, "        arc r = %f", elem->coor.arc.r);
		}
	    }
	}
    }

    fclose(fp);

    return symb;
}
