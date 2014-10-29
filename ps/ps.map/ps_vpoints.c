/* Functions: PS_vector_plot
 **
 ** Modified by: Janne Soimasuo August 1994 line_cat added
 ** Author: Paul W. Carlson     March 1992
 ** modified to use G_plot_line() by Olga Waupotitsch on dec,93
 */

#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/symbol.h>
#include <grass/glocale.h>

#include "clr.h"
#include "local_proto.h"
#include "vector.h"

int PS_vpoints_plot(struct Map_info *P_map, int vec)
{
    struct line_pnts *Points;
    int line, ltype, cat, nlines, ret;
    struct line_cats *Cats;

    char eps[50], epsfile[1024], sname[100];
    double nn, ee;
    double size, x, y, xt, yt;
    double llx, lly, urx, ury;
    int x_int, y_int, eps_exist;
    SYMBOL *Symb;
    struct varray *Varray = NULL;

    /* Attributes if sizecol is used */
    dbCatValArray cvarr_size;
    int size_val_int;
    double size_val;

    /* rotation column */
    dbCatValArray cvarr_rot;
    double rotate;
    int rot_val_int;

    /* rgbcol */
    dbCatValArray cvarr_rgb;
    dbCatVal *cv_rgb;
    int red, grn, blu;
    char *rgbstring = NULL;
    PSCOLOR color;

    cv_rgb = NULL;
    Symb = NULL;

    /* Create vector array if required */
    if (vector.layer[vec].cats != NULL || vector.layer[vec].where != NULL) {
	Varray = Vect_new_varray(Vect_get_num_lines(P_map));
	if (vector.layer[vec].cats != NULL) {
	    ret =
		Vect_set_varray_from_cat_string(P_map,
						vector.layer[vec].field,
						vector.layer[vec].cats,
						vector.layer[vec].ltype, 1,
						Varray);
	}
	else {
	    ret = Vect_set_varray_from_db(P_map, vector.layer[vec].field,
					  vector.layer[vec].where,
					  vector.layer[vec].ltype, 1, Varray);
	}
	G_debug(3, "%d items selected for vector %d", ret, vec);
	if (ret == -1)
	    G_fatal_error(_("Cannot load data from table"));
    }

    /* allocate memory for coordinates */
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* process only vectors in current window */
    Vect_set_constraint_region(P_map, PS.w.north, PS.w.south, PS.w.east,
			       PS.w.west, PORT_DOUBLE_MAX, -PORT_DOUBLE_MAX);

    /* Read symbol */
    if (vector.layer[vec].symbol != NULL) {
	sprintf(sname, "SITESYMBOL%d", vec);
	Symb = S_read(vector.layer[vec].symbol);
	if (Symb == NULL) {
	    G_warning(_("Cannot read symbol, using default icon"));
	}
	symbol_save(Symb, &(vector.layer[vec].color),
		    &(vector.layer[vec].fcolor), sname);
	vector.layer[vec].symbol_ps = G_store(sname);
    }


    /* if eps file is specified as common for all points then
       read bbox and save eps to PS file */
    if (vector.layer[vec].epstype == 1) {
	if (!eps_bbox(vector.layer[vec].epspre, &llx, &lly, &urx, &ury)) {
	    vector.layer[vec].epstype = 0;	/* eps file can't be read */
	}
	else {			/* save to PS */
	    sprintf(eps, "SITEEPSF%d", vec);
	    eps_save(PS.fp, vector.layer[vec].epspre, eps);
	}
    }


    /* Load attributes if sizecol used */
    if (vector.layer[vec].sizecol != NULL)
	load_catval_array_size(P_map, vec, &cvarr_size);

    /* load attributes if rgbcol used */
    if (vector.layer[vec].rgbcol != NULL)
	load_catval_array_rgb(P_map, vec, &cvarr_rgb);

    /* Load attributes if rotatecolumn used */
    if (vector.layer[vec].rotcol != NULL)
	load_catval_array_rot(P_map, vec, &cvarr_rot);


    /* read and plot vectors */
    nlines = Vect_get_num_lines(P_map);
    for (line = 1; line <= nlines; line++) {
	if (!Vect_line_alive(P_map, line))
	    continue;
	ltype = Vect_read_line(P_map, Points, Cats, line);

	if (!(ltype & GV_POINTS))
	    continue;
	if (!(ltype & vector.layer[vec].ltype))
	    continue;

	if (Varray != NULL && Varray->c[line] == 0)
	    continue;		/* is not in array */

	Vect_cat_get(Cats, 1, &cat);

	nn = Points->y[0];
	ee = Points->x[0];

	if (nn > PS.w.north || nn < PS.w.south)
	    continue;
	if (ee > PS.w.east || ee < PS.w.west)
	    continue;

	G_plot_where_xy(ee, nn, &x_int, &y_int);
	x = (double)x_int / 10.;
	y = (double)y_int / 10.;

	/* symbol size */
	if (vector.layer[vec].sizecol == NULL)
	    size = vector.layer[vec].size;
	else {			/* get value from sizecol column */

	    if (cvarr_size.ctype == DB_C_TYPE_INT) {
		ret =
		    db_CatValArray_get_value_int(&cvarr_size, cat,
						 &size_val_int);
		if (ret != DB_OK) {
		    G_warning(_("No record for category [%d]"), cat);
		    continue;
		}
		size_val = (double)size_val_int;
	    }

	    if (cvarr_size.ctype == DB_C_TYPE_DOUBLE) {
		ret =
		    db_CatValArray_get_value_double(&cvarr_size, cat,
						    &size_val);
		if (ret != DB_OK) {
		    G_warning(_("No record for category [%d]"), cat);
		    continue;
		}
	    }

	    if (size_val < 0.0) {
		G_warning(_("Attribute is of invalid size [%.3f] for category [%d]"),
			  size_val, cat);
		continue;
	    }

	    if (size_val == 0.0)
		continue;

	    size = size_val * vector.layer[vec].scale;
	    G_debug(3, "    dynamic symbol size = %.2f", size);
	}

	/* symbol color */
	if (vector.layer[vec].rgbcol != NULL) {
	    rgbstring = NULL;
	    ret = db_CatValArray_get_value(&cvarr_rgb, cat, &cv_rgb);

	    if (ret != DB_OK) {
		G_warning(_("No record for category [%d]"), cat);
	    }
	    else {
		rgbstring = db_get_string(cv_rgb->val.s);
		if (rgbstring == NULL ||
		    G_str_to_color(rgbstring, &red, &grn, &blu) != 1) {
		    G_warning(_("Invalid RGB color definition in column <%s> for category [%d]"),
			      vector.layer[vec].rgbcol, cat);
		    rgbstring = NULL;
		}
	    }

	    if (rgbstring) {
		/* TODO: do not duplicate save symbol */

		G_debug(3, "    dynamic symbol rgb color = %s", rgbstring);
		set_color(&color, red, grn, blu);

		sprintf(sname, "SITESYMBOL%d_%d", vec, line);

		symbol_save(Symb, &(vector.layer[vec].color), &color, sname);
	    }
	    else {		/* use default symbol */
		G_debug(3, "    static symbol rgb color = %d:%d:%d",
			vector.layer[vec].color.r,
			vector.layer[vec].color.g, vector.layer[vec].color.b);

		sprintf(sname, "SITESYMBOL%d", vec);
	    }
	}

	/* symbol rotation */
	if (vector.layer[vec].rotcol == NULL)
	    rotate = vector.layer[vec].rotate;
	else {			/* get value from rotcol column */

	    if (cvarr_rot.ctype == DB_C_TYPE_INT) {
		ret =
		    db_CatValArray_get_value_int(&cvarr_rot, cat,
						 &rot_val_int);
		if (ret != DB_OK) {
		    G_warning(_("No record for category [%d]"), cat);
		    continue;
		}
		rotate = (double)rot_val_int;
	    }

	    if (cvarr_rot.ctype == DB_C_TYPE_DOUBLE) {
		ret =
		    db_CatValArray_get_value_double(&cvarr_rot, cat, &rotate);
		if (ret != DB_OK) {
		    G_warning(_("No record for category [%d]"), cat);
		    continue;
		}
	    }

	    G_debug(3, "    dynamic rotation value = %.2f", rotate);
	}


	if (vector.layer[vec].epstype == 1) {	/* draw common eps */
	    /* calculate translation */
	    eps_trans(llx, lly, urx, ury, x, y, size, rotate, &xt, &yt);
	    eps_draw_saved(eps, xt, yt, size, rotate);
	}
	else if (vector.layer[vec].epstype == 2) {	/* draw epses */
	    sprintf(epsfile, "%s%d%s", vector.layer[vec].epspre, cat,
		    vector.layer[vec].epssuf);
	    if ((eps_exist = eps_bbox(epsfile, &llx, &lly, &urx, &ury))) {
		eps_trans(llx, lly, urx, ury, x, y, size, rotate, &xt, &yt);

		eps_draw(PS.fp, epsfile, xt, yt, size, rotate);
	    }
	}

	/* draw the icon */
	if ((vector.layer[vec].epstype == 0) ||
	    (vector.layer[vec].epstype == 2 && !eps_exist)) {
	    if (Symb != NULL) {
		symbol_draw(sname, x, y, size, rotate,
			    vector.layer[vec].width);
	    }
	}
    }				/* for (line) */

    fprintf(PS.fp, "\n");
    return 0;
}
