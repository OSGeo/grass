#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "local.h"


static struct list
{
    double size;
    int index;
    CELL cat;
} *list;

static int nareas;

/* function prototypes */
static int compare(const void *, const void *);


int do_areas(struct Map_info *Map, struct line_pnts *Points,
	     dbCatValArray * Cvarr, int ctype, int use,
	     double value, int value_type)
{
    int i;
    CELL cval, cat;
    DCELL dval;

    if (nareas <= 0)
	return 0;

    G_message(_("Reading areas..."));
    for (i = 0; i < nareas; i++) {
	/* Note: in old version (grass5.0) there was a check here if the current area 
	 *        is identical to previous one. I don't see any reason for this in topological vectors */
	G_percent(i, nareas, 2);
	cat = list[i].cat;
	G_debug(3, "Area cat = %d", cat);

	if (ISNULL(&cat)) {	/* No centroid or no category */
	    set_cat(cat);
	}
	else {
	    if (use == USE_ATTR) {
		if (ctype == DB_C_TYPE_INT) {
		    if ((db_CatValArray_get_value_int(Cvarr, cat, &cval)) !=
			DB_OK) {
			G_warning(_("No record for area (cat = %d)"), cat);
			SETNULL(&cval);
		    }
		    set_cat(cval);
		}
		else if (ctype == DB_C_TYPE_DOUBLE) {
		    if ((db_CatValArray_get_value_double(Cvarr, cat, &dval))
			!= DB_OK) {
			G_warning(_("No record for area (cat = %d)"), cat);
			SETDNULL(&dval);
		    }
		    set_dcat(dval);
		}
		else {
		    G_fatal_error(_("Unable to use column specified"));
		}
	    }
	    else if (use == USE_CAT) {
		set_cat(cat);
	    }
	    else {
		if (value_type == USE_CELL)
		    set_cat((int)value);
		else
		    set_dcat(value);
	    }
	}

	if (Vect_get_area_points(Map, list[i].index, Points) <= 0) {
	    G_warning(_("Get area %d failed"), list[i].index);
	    return -1;
	}

	G_plot_polygon(Points->x, Points->y, Points->n_points);
    }
    G_percent(1, 1, 1);
    
    return nareas;
}


int sort_areas(struct Map_info *Map, struct line_pnts *Points,
               int field, struct cat_list *cat_list)
{
    int i, centroid, nareas_selected;
    struct line_cats *Cats;
    CELL cat;

    G_begin_polygon_area_calculations();
    Cats = Vect_new_cats_struct();

    /* first count valid areas */
    nareas = Vect_get_num_areas(Map);
    if (nareas == 0)
	return 0;

    /* allocate list to hold valid area info */
    list =
	(struct list *)G_calloc(nareas * sizeof(char), sizeof(struct list));

    /* store area size,cat,index in list */
    nareas_selected = 0;
    for (i = 0; i < nareas; i++) {

	centroid = Vect_get_area_centroid(Map, i + 1);
	SETNULL(&cat);
	if (centroid <= 0) {
	    G_debug(2,_("Area without centroid (OK for island)"));
	}
	else {
	    Vect_read_line(Map, NULL, Cats, centroid);
	    if (field > 0) {
		if (Vect_cats_in_constraint(Cats, field, cat_list)) {
		    Vect_cat_get(Cats, field, &cat);
		    nareas_selected++;
		}
		else {
		    G_debug(2, _("Area centroid without category"));
		}
	    }
	    else {
		/* field < 1, process all areas with centroid */
		cat = 0;
		nareas_selected++;
	    }
	}

	list[i].index = i + 1;
	Vect_get_area_points(Map, i + 1, Points);
	list[i].size =
	    G_area_of_polygon(Points->x, Points->y, Points->n_points);

	list[i].cat = cat;
    }
    if (nareas_selected > 0) {
	/* sort the list by size */
	qsort(list, nareas * sizeof(char), sizeof(struct list), compare);
    }

    return nareas_selected;
}


static int compare(const void *aa, const void *bb)
{
    const struct list *a = aa, *b = bb;

    if (a->size < b->size)
	return 1;
    if (a->size > b->size)
	return -1;

    return 0;
}
