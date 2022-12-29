#include <stdlib.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

int option_to_display(const struct Option *opt)
{
    int i, display;
    
    i = display = 0;
    while (opt->answers[i]) {
	switch (opt->answers[i][0]) {
	case 's':
	    display |= DISP_SHAPE;
	    break;
	case 'c':
	    display |= DISP_CAT;
	    break;
	case 'v':
	    display |= DISP_VERT;
	    break;
	case 't':
	    display |= DISP_TOPO;
	    break;
	case 'd':
	    display |= DISP_DIR;
	    break;
	case 'z':
	    display |= DISP_ZCOOR;
	    break;
	}
	i++;
    }

    return display;
}

void options_to_lattr(LATTR *lattr, const char *layer,
		      const char *color, const char *bgcolor, const char *bcolor,
		      int size, const char *font, const char *encoding,
		      const char *xref, const char *yref)
{
    int r, g, b;
    
    if (layer)
	lattr->field = atoi(layer);
    else
	lattr->field = 1;

    lattr->color.R = lattr->color.G = lattr->color.B = 255;
    if (G_str_to_color(color, &r, &g, &b)) {
	lattr->color.R = r;
	lattr->color.G = g;
	lattr->color.B = b;
    }
    lattr->has_bgcolor = 0;
    if (G_str_to_color(bgcolor, &r, &g, &b) == 1) {
	lattr->has_bgcolor = 1;
	lattr->bgcolor.R = r;
	lattr->bgcolor.G = g;
	lattr->bgcolor.B = b;
    }
    lattr->has_bcolor = 0;
    if (G_str_to_color(bcolor, &r, &g, &b) == 1) {
	lattr->has_bcolor = 1;
	lattr->bcolor.R = r;
	lattr->bcolor.G = g;
	lattr->bcolor.B = b;
    }

    lattr->size = size;
    lattr->font = font;
    lattr->enc = encoding;
    if (xref) {
	switch (xref[0]) {
	case 'l':
	    lattr->xref = LLEFT;
	    break;
	case 'c':
	    lattr->xref = LCENTER;
	    break;
	case 'r':
	    lattr->xref = LRIGHT;
	    break;
	}
    }
    else
	lattr->xref = LCENTER;
    
    if (yref) {
	switch (yref[0]) {
	case 't':
	    lattr->yref = LTOP;
	    break;
	case 'c':
	    lattr->yref = LCENTER;
	    break;
	case 'b':
	    lattr->yref = LBOTTOM;
	    break;
	}
    }
    else
	lattr->yref = LCENTER;
}

int option_to_color(struct color_rgb *color, const char *color_val)
{
    int has_color, ret;
    int r, g, b;
    
    ret = G_str_to_color(color_val, &r, &g, &b);
    if (ret == 1) {
	has_color = 1;
	color->r = r;
	color->g = g;
	color->b = b;
    }
    else if (ret == 2) {	/* none */
	has_color = 0;
    }
    else if (ret == 0) {	/* error */
	G_fatal_error(_("Unknown color: '%s'"), color_val);
    }
    
    return has_color;
}

void option_to_where(struct Map_info *Map, struct cat_list *Clist,
		     const char *where)
{
    int ncat;
    int *cats;
    struct field_info *fi;
    dbDriver *driver;
    dbHandle handle;
    
    fi = Vect_get_field(Map, Clist->field);
    if (!fi)
	G_fatal_error(_("Database connection not defined"));
    
    driver = db_start_driver(fi->driver);
    if (!driver)
	G_fatal_error(_("Unable to start driver <%s>"), fi->driver);
    
    db_init_handle(&handle);
    db_set_handle(&handle, fi->database, NULL);
    if (db_open_database(driver, &handle) != DB_OK)
	G_fatal_error(_("Unable to open database <%s>"),
		      fi->database);
    
    ncat = db_select_int(driver, fi->table, fi->key, where,
			 &cats);
    
    db_close_database(driver);
    db_shutdown_driver(driver);
    
    Vect_array_to_cat_list(cats, ncat, Clist);
}	
	
