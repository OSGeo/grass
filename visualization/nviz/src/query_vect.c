#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/form.h>
#include <grass/dbmi.h>
#include <grass/display.h>
#include <grass/Vect.h>

char *query_vect(char *name, double x, double y)
{
    int level;
    char buf[2000];
    struct Map_info Map;
    double maxdist;
    int line, area;
    static struct line_cats *Cats = NULL;
    char *mapset, *form_buf;
    dbString form;
    struct field_info *Fi;

    G_debug(3, "query_vect() name = %s x,y = %f, %f", name, x, y);

    if (!name || strlen(name) == 0)
	return "";

    if (!Cats)
	Cats = Vect_new_cats_struct();
    else
	Vect_reset_cats(Cats);

    db_init_string(&form);

    if ((mapset = G_find_vector2(name, "")) == NULL) {
	return ("Could not find input map\n");
    }

    Vect_set_open_level(2);
    Vect_set_fatal_error(GV_FATAL_PRINT);
    level = Vect_open_old(&Map, name, mapset);
    if (level < 2) {
	return ("Could not open map on level 2.\n");
    }

    /* TODO: set maxdist correctly */
    maxdist = 10000;

    line =
	Vect_find_line(&Map, x, y, 0.0,
		       GV_POINT | GV_LINE | GV_BOUNDARY | GV_CENTROID,
		       maxdist, 0, 0);
    area = Vect_find_area(&Map, x, y);

    if (line + area == 0) {
	return ("Nothing found.\n");
    }

    if (line > 0) {
	Vect_read_line(&Map, NULL, Cats, line);
    }
    else if (area > 0) {
	Vect_get_area_cats(&Map, area, Cats);
    }

    if (Cats->n_cats > 0) {
	int i;

	for (i = 0; i < Cats->n_cats; i++) {
	    sprintf(buf, "\nlayer: %d\ncategory: %d\n", Cats->field[i],
		    Cats->cat[i]);
	    db_append_string(&form, buf);

	    Fi = Vect_get_field(&Map, Cats->field[i]);
	    if (Fi != NULL) {
		F_generate(Fi->driver, Fi->database, Fi->table, Fi->key,
			   Cats->cat[i], NULL, NULL, F_VIEW, F_TXT,
			   &form_buf);
		db_append_string(&form, form_buf);
	    }
	}
    }
    else {
	return ("No category\n");
    }

    Vect_close(&Map);

    G_debug(3, "form = %s\n", db_get_string(&form));
    return (db_get_string(&form));
}
