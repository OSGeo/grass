/*  @(#)xtract_lines.c    1.0  9/29/89   
 *  created by:         R.L.Glenn, SCS
 *
 * Program will read vector line records, outputting lines
 * which match the user list of names/categories.
 * The resulting map attribute is arbitarily set to first category
 * of the user list or a user selected category number (cat_new).
 */

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

static int *cats_array;
static int ncats_array;

int cmp(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    int *p2 = (int *)pb;

    if (*p1 < *p2)
	return -1;
    if (*p1 > *p2)
	return 1;
    return 0;
}

/* check if cat is in list */
static int in_list(int cat)
{
    if (bsearch(&cat, cats_array, ncats_array, sizeof(int), cmp))
	return 1;

    return 0;
}

/* output reclass cats */
static void extract_cats(struct line_cats *Cats, int type_only, int field, int new,
			 int reverse)
{
    int i, tmp;
    static struct line_cats *TCats = NULL;

    if (!TCats)
	TCats = Vect_new_cats_struct();

    /* Copy */
    Vect_reset_cats(TCats);

    for (i = 0; i < Cats->n_cats; i++) {
	Vect_cat_set(TCats, Cats->field[i], Cats->cat[i]);
    }

    if (type_only && field == -1) {	/* keep all */
	return;
    }
    else if (type_only && field > 0) {	/* keep only one field */
	Vect_reset_cats(Cats);

	if (new > -1) {
	    if (Vect_cat_get(TCats, field, &tmp))
		Vect_cat_set(Cats, field, new);
	}
	else {
	    for (i = 0; i < TCats->n_cats; i++) {
		if (TCats->field[i] == field)
		    Vect_cat_set(Cats, field, TCats->cat[i]);
	    }
	}
    }
    else {			/* keep only one field, cats in list */

	Vect_reset_cats(Cats);

	if (new > -1) {
	    int found = 0;

	    for (i = 0; i < TCats->n_cats; i++) {
		if (TCats->field[i] == field) {
		    if ((!reverse && in_list(TCats->cat[i])) ||
			(reverse && !in_list(TCats->cat[i]))) {
			found = 1;
			break;
		    }
		}
	    }
	    if (found)
		Vect_cat_set(Cats, field, new);
	}
	else {
	    for (i = 0; i < TCats->n_cats; i++) {
		if (TCats->field[i] == field) {
		    if ((!reverse && in_list(TCats->cat[i])) ||
			(reverse && !in_list(TCats->cat[i]))) {
			Vect_cat_set(Cats, field, TCats->cat[i]);
		    }
		}
	    }
	}

    }
}

/* check if output cats of left and right area match */
static int areas_new_cats_match(struct Map_info *In, int area1, int area2,
				int type_only, int field, int new, int reverse)
{
    int i, j, found;
    int centroid1, centroid2;
    static struct line_cats *Cats1 = NULL;
    static struct line_cats *Cats2 = NULL;

    G_debug(4, "areas_new_cats_match area1 = %d area2 = %d", area1, area2);

    if (!Cats1)
	Cats1 = Vect_new_cats_struct();
    if (!Cats2)
	Cats2 = Vect_new_cats_struct();

    if (area1 < 1 || area2 < 1)
	return 0;		/* at least one area is missing */

    centroid1 = Vect_get_area_centroid(In, area1);
    centroid2 = Vect_get_area_centroid(In, area2);

    if (centroid1 < 1 || centroid2 < 1)
	return 0;		/* at least one centroid is missing */

    Vect_read_line(In, NULL, Cats1, centroid1);
    Vect_read_line(In, NULL, Cats2, centroid2);

    extract_cats(Cats1, type_only, field, new, reverse);
    extract_cats(Cats2, type_only, field, new, reverse);

    if (Cats1->n_cats != Cats2->n_cats)
	return 0;

    for (i = 0; i < Cats1->n_cats; i++) {
	found = 0;
	for (j = 0; j < Cats2->n_cats; j++) {
	    G_debug(5, "%d:%d x %d:%d", Cats1->field[i], Cats1->cat[i],
		    Cats2->field[j], Cats2->cat[j]);
	    if (Cats1->field[i] == Cats2->field[j] &&
		Cats1->cat[i] == Cats2->cat[j]) {
		found = 1;
		break;
	    }
	}
	if (!found)
	    return 0;
    }

    G_debug(3, "match");
    return 1;
}

int extract_line(int num_index, int *num_array, struct Map_info *In,
		 struct Map_info *Out, int new, int select_type, int dissolve,
		 int field, int type_only, int reverse)
{
    int line, nlines;
    struct line_pnts *Points;
    struct line_cats *Line_Cats_Old, *CCats;

    int left_area, right_area;	/* left/right area */
    int left_field_match, right_field_match;	/* left/right area field match conditions */
    int left_cat_match, right_cat_match;	/* left/right area cat match conditions */
    int type_match;		/* line type match */
    int field_match;		/* category of field match */
    int cat_match;		/* category found in list */
    int centroid_in_area;	/* centroid is in area */
    int type;
    int i, tmp, write;

    /* Initialize the Point structure, ONCE */
    Points = Vect_new_line_struct();
    Line_Cats_Old = Vect_new_cats_struct();
    CCats = Vect_new_cats_struct();

    cats_array = num_array;
    ncats_array = num_index;

    /* sort list */
    qsort(cats_array, ncats_array, sizeof(int), cmp);

    /* Cycle through all lines */
    nlines = Vect_get_num_lines(In);
    for (line = 1; line <= nlines; line++) {
	/* default values */
	left_area = right_area = 0;
	left_field_match = right_field_match = 0;
	left_cat_match = right_cat_match = 0;
	type_match = field_match = cat_match = 0;
	centroid_in_area = 0;
	write = 0;

	G_percent(line, nlines, 2);
	G_debug(3, "Line = %d", line);

	/* Get data */
	type = Vect_read_line(In, Points, Line_Cats_Old, line);
	G_debug(3, "type = %d ncats = %d", type, Line_Cats_Old->n_cats);

	if (type & select_type)
	    type_match = 1;

	if (field > 0)
	    field_match = Vect_cat_get(Line_Cats_Old, field, &tmp);

	for (i = 0; i < Line_Cats_Old->n_cats; i++) {
	    G_debug(3, "field = %d cat = %d", Line_Cats_Old->field[i],
		    Line_Cats_Old->cat[i]);
	    if (Line_Cats_Old->field[i] == field) {
		if ((!reverse && in_list(Line_Cats_Old->cat[i])) ||
		    (reverse && !in_list(Line_Cats_Old->cat[i]))) {
		    cat_match = 1;
		    break;
		}
	    }
	}

	if (type == GV_BOUNDARY) {
	    int centroid;

	    Vect_get_line_areas(In, line, &left_area, &right_area);

	    if (left_area < 0)
		left_area = Vect_get_isle_area(In, abs(left_area));
	    if (left_area > 0) {
		centroid = Vect_get_area_centroid(In, left_area);
		if (centroid > 0) {
		    Vect_read_line(In, NULL, CCats, centroid);
		    left_field_match = Vect_cat_get(CCats, field, &tmp);
		    for (i = 0; i < CCats->n_cats; i++) {
			if (CCats->field[i] == field) {
			    if ((!reverse && in_list(CCats->cat[i])) ||
				(reverse && !in_list(CCats->cat[i]))) {
				left_cat_match = 1;
				break;
			    }
			}
		    }
		}
	    }

	    if (right_area < 0)
		right_area = Vect_get_isle_area(In, abs(right_area));
	    if (right_area > 0) {
		centroid = Vect_get_area_centroid(In, right_area);
		if (centroid > 0) {
		    Vect_read_line(In, NULL, CCats, centroid);
		    right_field_match = Vect_cat_get(CCats, field, &tmp);
		    for (i = 0; i < CCats->n_cats; i++) {
			if (CCats->field[i] == field) {
			    if ((!reverse && in_list(CCats->cat[i])) ||
				(reverse && !in_list(CCats->cat[i]))) {
				right_cat_match = 1;
				break;
			    }
			}
		    }
		}
	    }
	}

	if (type == GV_CENTROID) {
	    int area;

	    area = Vect_get_centroid_area(In, line);
	    if (area > 0)
		centroid_in_area = 1;
	}

	G_debug(2, "type_match = %d field_match = %d cat_match = %d",
		type_match, field_match, cat_match);
	G_debug(2, "left_area = %d left_field_match = %d left_cat_match = %d",
		left_area, left_field_match, left_cat_match);

	G_debug(2,
		"right_area = %d right_field_match = %d right_cat_match = %d",
		right_area, right_field_match, right_cat_match);

	/* Check if the line match conditions */
	if (type_only && field == -1) {	/* type only */
	    /* line */
	    if (type_match)
		write = 1;

	    /* centroid */
	    if ((type == GV_CENTROID && (select_type & GV_AREA)) &&
		!centroid_in_area)
		write = 0;

	    /* areas */
	    if (type == GV_BOUNDARY && (left_area || right_area)) {
		if (!dissolve ||
		    !areas_new_cats_match(In, left_area, right_area,
					  type_only, field, new, reverse))
		    write = 1;
	    }
	}
	else if (type_only && field > 0) {	/* type and field only */
	    /* line */
	    if (type_match && field_match)
		write = 1;

	    /* centroid */
	    if ((type == GV_CENTROID && (select_type & GV_AREA)) &&
		!(centroid_in_area && field_match))
		write = 0;

	    /* areas */
	    if (type == GV_BOUNDARY && (select_type & GV_AREA) &&
		(left_field_match || right_field_match)) {
		if (!dissolve ||
		    !areas_new_cats_match(In, left_area, right_area,
					  type_only, field, new, reverse))
		    write = 1;
	    }
	}
	else {			/* type, field and category */

	    /* line */
	    if (type_match && cat_match)
		write = 1;

	    /* centroid */
	    if ((type == GV_CENTROID && (select_type & GV_AREA)) &&
		!(centroid_in_area && field_match))
		write = 0;

	    /* areas */
	    if (type == GV_BOUNDARY && (select_type & GV_AREA) &&
		(left_cat_match || right_cat_match)) {
		if (!dissolve ||
		    !areas_new_cats_match(In, left_area, right_area,
					  type_only, field, new, reverse))
		    write = 1;
	    }
	}

	G_debug(2, "write = %d", write);

	if (write) {
	    extract_cats(Line_Cats_Old, type_only, field, new, reverse);

	    Vect_write_line(Out, type, Points, Line_Cats_Old);
	}

    }				/* end lines section */
    
    return 0;
}
