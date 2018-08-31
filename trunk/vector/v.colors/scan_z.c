#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

void scan_z(struct Map_info *Map, int field,
            const char *style, const char *rules,
            const struct FPRange *range, struct Colors *colors)
{
    int ltype, line, cat, i;
    int items_alloc;
    double zmin, zmax;
    struct line_pnts *Points;
    struct line_cats *Cats;
    
    struct Colors vcolors;
    
    dbCatValArray cvarr;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    
    items_alloc = 0;
    db_CatValArray_init(&cvarr);
    cvarr.ctype = DB_C_TYPE_DOUBLE;

    Vect_set_constraint_field(Map, field);
    Vect_set_constraint_type(Map, GV_POINTS); /* points, centroids or kernels only) */
        
    G_message(_("Reading features..."));
    line = i = 0;
    while(TRUE) {
	ltype = Vect_read_next_line(Map, Points, Cats);
	if (ltype == -1)
	    G_fatal_error(_("Unable to read vector map"));
	if (ltype == -2)
	    break; /* EOF */

	G_progress(++line, 1e4);
        
        if (Vect_cat_get(Cats, field, &cat) == -1)
            continue; /* skip features without category */
        
        /* add item into cat-value array */
        if (i >= items_alloc) {
            items_alloc += 1000;
            db_CatValArray_realloc(&cvarr, items_alloc);
        }
        cvarr.n_values++;
        cvarr.value[i].cat = cat;
        cvarr.value[i++].val.d = Points->z[0];

	if (line == 1 || Points->z[0] < zmin)
	    zmin = Points->z[0];
	if (line == 1 || Points->z[0] > zmax)
	    zmax = Points->z[0];
    }
    G_progress(1, 1);
    
    /* sort array by z-coordinate */
    db_CatValArray_sort_by_value(&cvarr);
    
    if (range) {
	if (range->min >= zmin && range->min <= zmax)
	    zmin = range->min;
	else
	    G_warning(_("Min value (%f) is out of range %f,%f"),
		      range->min, zmin, zmax);
	
	if (range->max <= zmax && range->max >= zmin)
	    zmax = range->max;
	else
	    G_warning(_("Max value (%f) is out of range %f,%f"),
		      range->max, zmin, zmax);
    }

    if (style)
	make_colors(&vcolors, style, (DCELL) zmin, (DCELL) zmax, TRUE);
    else if (rules) {
	load_colors(&vcolors, rules, (DCELL) zmin, (DCELL) zmax, TRUE);
    }

    /* color table for categories */
    color_rules_to_cats(&cvarr, TRUE, &vcolors, colors);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    db_CatValArray_free(&cvarr);

}
