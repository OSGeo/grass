/*  From preliminary work (pull.vect) by Dave Gerdes - CERL
 *  created by:         R.L.Glenn, SCS
 *  modified to function by RB 5/2000
 *
 * Function reclass() reads input vector map and writes reclassed elements to output map.
 *
 * Arguments:
 * in_name - name of input vector map
 * out_name - name of output vector map
 * new - reclass table
 * type - elements type 
 * optiond - do not output boundaries between areas with the same cat 
 *
 * Returns:
 * number of elements created or -1 on error
 */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>

int reclass(struct Map_info *In, struct Map_info *Out, int type, int field,
	    dbCatValArray * cvarr, int dissolve)
{
    int i, nlines, line, ltype, old_cat, new_cat;
    int nocat = 0, rclelem = 0, negative = 0;
    struct line_pnts *Points;
    struct line_cats *Cats, *NewCats;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    NewCats = Vect_new_cats_struct();

    /* --------------------- Lines Section ------------------------------- */
    /* Cycle through all lines */
    nlines = Vect_get_num_lines(In);
    for (line = 1; line <= nlines; line++) {
	G_percent(line, nlines, 1);

	ltype = Vect_read_line(In, Points, Cats, line);

	Vect_reset_cats(NewCats);

	for (i = 0; i < Cats->n_cats; i++) {
	    if ((ltype & type) && Cats->field[i] == field) {	/* reclass */
		old_cat = Cats->cat[i];
		G_debug(3, "  old_cat = %d", old_cat);

		if (db_CatValArray_get_value_int(cvarr, old_cat, &new_cat) !=
		    DB_OK) {
		    nocat++;
		}
		else {
		    G_debug(3, "  new_cat = %d", new_cat);

		    if (new_cat < 0) {
			negative++;
		    }
		    else {
			Vect_cat_set(NewCats, field, new_cat);
		    }
		    rclelem++;
		}
	    }
	    else {		/* copy */
		Vect_cat_set(NewCats, Cats->field[i], Cats->cat[i]);
	    }
	}
	Vect_write_line(Out, ltype, Points, NewCats);
    }

    if (nocat > 0)
	G_warning("For %d elements no new category was defined", nocat);

    if (negative > 0)
	G_warning
	    ("For %d elements requested negative category (ignored, no category in output)",
	     negative);

    return (rclelem);
}
