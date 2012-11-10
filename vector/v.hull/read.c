#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "hull.h"

int loadSiteCoordinates(struct Map_info *Map, struct Point **points, int region,
			struct Cell_head *window, int field, struct cat_list *cat_list)
{
    int i, pointIdx;
    struct line_pnts *sites;
    struct line_cats *cats;
    struct bound_box box;
    int cat, type;

    sites = Vect_new_line_struct();
    cats = Vect_new_cats_struct();

    *points = NULL;
    pointIdx = 0;
    
    /* copy window to box */
    Vect_region_box(window, &box);

    while ((type = Vect_read_next_line(Map, sites, cats)) > -1) {

	if (type != GV_POINT && !(type & GV_LINES))
	    continue;

	if (cat_list) {
	    int found = 0;

	    for (i = 0; i < cats->n_cats; i++) {
		if (cats->field[i] == field &&
		    Vect_cat_in_cat_list(cats->cat[i], cat_list)) {
		    
		    found = 1;
		    break;
		}
	    }
	    if (!found)
		continue;
	}
	else if (field > 0 && Vect_cat_get(cats, field, &cat) == 0)
	    continue;
	
	for (i = 0; i < sites->n_points; i++) {
	    G_debug(4, "Point: %f|%f|%f|#%d", sites->x[i], sites->y[i],
		    sites->z[i], cat);
	    
	    if (region && !Vect_point_in_box(sites->x[i], sites->y[i], sites->z[i], &box))
		continue;
	    
	    G_debug(4, "Point in the box");

	    if ((pointIdx % ALLOC_CHUNK) == 0)
		*points = (struct Point *) G_realloc(*points,
						     (pointIdx + ALLOC_CHUNK) * sizeof(struct Point));
	    
	    (*points)[pointIdx].x = sites->x[i];
	    (*points)[pointIdx].y = sites->y[i];
	    (*points)[pointIdx].z = sites->z[i];
	    pointIdx++;
	}
    }

    if (pointIdx > 0)
	*points = (struct Point *)G_realloc(*points,
					    (pointIdx + 1) * sizeof(struct Point));
    
    return pointIdx;
}

static int cmp_int(const void *a, const void *b)
{
    int ai = *(int *)a;
    int bi = *(int *)b;
    
    return (ai < bi ? -1 : (ai > bi));
}

/* parse filter options */
/* return cat list or NULL */
struct cat_list *parse_filter_options(struct Map_info *Map, int layer,
                      char *where, char *catstr)
{
    struct cat_list *list = NULL;

    if (where) {
	struct field_info *Fi = NULL;
	dbDriver *driver = NULL;
	int ncats, *cats = NULL;
	int i, j;
	
	if (layer < 1)
	    G_fatal_error(_("'%s' must be > 0 for '%s'"), "layer", "where");
	if (catstr)
	    G_warning(_("'where' and 'cats' parameters were supplied, cat will be ignored"));

	Fi = Vect_get_field(Map, layer);
	if (!Fi) {
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  layer);
	}

	G_verbose_message(_("Loading categories from table <%s>..."), Fi->table);

	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
	
	ncats = db_select_int(driver, Fi->table, Fi->key, where,
			      &cats);
	if (ncats == -1)
		G_fatal_error(_("Unable select records from table <%s>"),
			      Fi->table);
	G_verbose_message(_("%d categories loaded"), ncats);
	    
	db_close_database_shutdown_driver(driver);

	/* sort */
	qsort(cats, ncats, sizeof(int), cmp_int);
	
	/* remove duplicates */
	j = 1;
	for (i = 1; i < ncats; i++) {
	    if (cats[i] != cats[j - 1]) {
		cats[j] = cats[i];
		j++;
	    }
	}
	ncats = j;
	
	/* convert to cat list */
	list = Vect_new_cat_list();
	
	Vect_array_to_cat_list(cats, ncats, list);
	
	if (cats)
	    G_free(cats);
    }
    else if (catstr) {
	if (layer < 1)
	    G_fatal_error(_("'%s' must be > 0 for '%s'"), "layer", GV_KEY_COLUMN);
	list = Vect_new_cat_list();

	if (Vect_str_to_cat_list(catstr, list) > 0) {
	    G_warning(_("Problem loading category values"));
	}
    }
    
    if (list) {
	if (list->n_ranges < 1) {
	    Vect_destroy_cat_list(list);
	    list = NULL;
	}
    }
	
    return list;
}
