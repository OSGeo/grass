/* Functions: PS_area_plot
 **
 ** Author: Radim Blazek Jan 2000
 ** modified copy of ps_vector.c by Paul W. Carlson March 1992
 */

#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#include "vector.h"
#include "clr.h"
#include "local_proto.h"

/* constuct subpath with moveto and repeated lineto from Points */
int construct_path(struct line_pnts *Points, double shift, int t)
{
    int i, np, k = 1;
    double *xarray, *yarray, x, y;

    np = Points->n_points;
    xarray = Points->x;
    yarray = Points->y;

    for (i = 0; i < np; i++) {
	x = XCONV(xarray[0] + shift);
	y = YCONV(yarray[0]);
	fprintf(PS.fp, "%.1f %.1f ", x, y);
	if (i == 0 && (t == START_PATH || t == WHOLE_PATH)) {
	    fprintf(PS.fp, "M ");
	}
	else {
	    fprintf(PS.fp, "LN ");
	}
	if (k == 2) {
	    fprintf(PS.fp, "\n");
	    k = 0;
	}
	else {
	    fprintf(PS.fp, " ");
	    k++;
	}
	xarray++;
	yarray++;
    }
    if (t == CLOSE_PATH || t == WHOLE_PATH) {
	fprintf(PS.fp, "CP\n");
    }
    return 1;
}

/* create paths for area and islands */
static int plot_area(struct Map_info *P_map, int area, double shift)
{
    struct line_pnts *Points;
    int j, ret, ni, island;

    /* allocate memory for coordinates */
    Points = Vect_new_line_struct();

    /* plot areas */
    if (0 > (ret = Vect_get_area_points(P_map, area, Points))) {
	if (ret == -1)
	    G_warning(_("Read error in vector map"));
	return 0;
    }
    construct_path(Points, shift, WHOLE_PATH);

    /* plot islands */
    ni = Vect_get_area_num_isles(P_map, area);
    for (j = 0; j < ni; j++) {
	island = Vect_get_area_isle(P_map, area, j);
	if (0 > (ret = Vect_get_isle_points(P_map, island, Points))) {
	    if (ret == -1)
		G_warning(_("Read error in vector map"));
	    return -1;
	}
	construct_path(Points, shift, WHOLE_PATH);
    }
    return 1;
}


/* get pscolor based on rgb color definition stored in rgbcolumn */
/*   sets *color and returns 1 on success, -1 if no color found */
int get_ps_color_rgbcol_varea(struct Map_info *map, int vec, int area,
			       dbCatValArray * cvarr_rgb, PSCOLOR *color)
{
    int cat, ret;
    dbCatVal *cv_rgb;
    int red, grn, blu;
    char *rgbstring = NULL;

    cat = Vect_get_area_cat(map, area, vector.layer[vec].field);

    ret = db_CatValArray_get_value(cvarr_rgb, cat, &cv_rgb);

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
	G_debug(3, "    dynamic varea fill rgb color = %s", rgbstring);
	set_color(color, red, grn, blu);
    }
    else {
	set_color(color, 0, 0, 0);
	return -1;
    }

    return 1;
}


/* plot areas */
int PS_vareas_plot(struct Map_info *P_map, int vec)
{
    int na, area, ret;
    double e, w, n, s, aw, shift;
    double llx, lly, urx, ury, sc;
    char pat[50];
    struct bound_box box;
    struct varray *Varray = NULL;
    PSCOLOR color;
    int centroid;

    /* rgbcol */
    dbCatValArray cvarr_rgb;

    fprintf(PS.fp, "1 setlinejoin\n");	/* set line join to round */

    /* Create vector array if required */
    if (vector.layer[vec].cats != NULL || vector.layer[vec].where != NULL) {
	Varray = Vect_new_varray(Vect_get_num_areas(P_map));
	if (vector.layer[vec].cats != NULL) {
	    ret =
		Vect_set_varray_from_cat_string(P_map,
						vector.layer[vec].field,
						vector.layer[vec].cats,
						GV_AREA, 1, Varray);
	}
	else {
	    ret = Vect_set_varray_from_db(P_map, vector.layer[vec].field,
					  vector.layer[vec].where, GV_AREA, 1,
					  Varray);
	}
	G_debug(3, "%d items selected for vector %d", ret, vec);
	if (ret == -1)
	    G_fatal_error(_("Cannot load data from table"));
    }

    /* load attributes if rgbcol used */
    if (vector.layer[vec].rgbcol != NULL) {
	load_catval_array_rgb(P_map, vec, &cvarr_rgb);
    }

    shift = 0;
    /* read and plot areas */
    na = Vect_get_num_areas(P_map);
    for (area = 1; area <= na; area++) {
	G_debug(4, "area = %d", area);

	if (Varray != NULL && Varray->c[area] == 0)
	    continue;		/* is not in array */

	if (!Vect_area_alive(P_map, area))
	    continue;

	centroid = Vect_get_area_centroid(P_map, area);
	G_debug(4, "centroid = %d", centroid);
	if (centroid < 1)  /* area is an island */
	    continue;

	Vect_get_area_box(P_map, area, &box);
	n = box.N;
	s = box.S;
	e = box.E;
	w = box.W;
	if (PS.w.proj == PROJECTION_LL) {
	    aw = G_adjust_easting(w, &PS.w);
	    if (aw > PS.w.east)
		aw -= 360.0;
	    shift = aw - w;
	    e += shift;
	    w += shift;
	}
	/* check if in window */
	if (n < PS.w.south || s > PS.w.north || e < PS.w.west ||
	    w > PS.w.east)
	    continue;

	fprintf(PS.fp, "NP\n");
	if (PS.w.proj == PROJECTION_LL) {
	    /* plot area while in window */
	    while (e > PS.w.west) {
		ret = plot_area(P_map, area, shift);
		if (ret != 1)
		    return 0;
		shift -= 360.0;
		e -= 360.0;
	    }
	}
	else {
	    ret = plot_area(P_map, area, shift);
	    if (ret != 1)
		return 0;
	}

	if (vector.layer[vec].pat != NULL ||
	    (!color_none(&vector.layer[vec].fcolor) ||
	     vector.layer[vec].rgbcol != NULL)) {

	    if (vector.layer[vec].rgbcol != NULL) {
		/* load fill color from rgbcol */
		/* if data column is empty or cat is missing don't fill */
		if (get_ps_color_rgbcol_varea(P_map, vec, area, &cvarr_rgb, &color) != 1)
		    return 0;
	    }
	    else {
		color = vector.layer[vec].fcolor;
	    }

	    if (vector.layer[vec].pat != NULL) {	/* use pattern */
		sc = vector.layer[vec].scale;
		/* DEBUG */
		/*
		   printf("\n eps pattern = %s\n", vector.layer[vec].eps);
		   printf("       scale = %f\n", vector.layer[vec].scale);
		 */
		/* load pattern */
		eps_bbox(vector.layer[vec].pat, &llx, &lly, &urx, &ury);
		sprintf(pat, "APATTEPS%d", vec);
		pat_save(PS.fp, vector.layer[vec].pat, pat);
		fprintf(PS.fp,
			"<<  /PatternType 1\n    /PaintType 1\n    /TilingType 1\n");
		fprintf(PS.fp, "    /BBox [%f %f %f %f]\n", llx * sc,
			lly * sc, urx * sc, ury * sc);
		fprintf(PS.fp, "    /XStep %f\n    /YStep %f\n",
			(urx - llx) * sc, (ury - lly) * sc);
		fprintf(PS.fp, "    /PaintProc\n      { begin\n");
		fprintf(PS.fp, "        %f %f scale\n", sc, sc);

		set_ps_color(&color);

		fprintf(PS.fp, "        %.8f W\n", vector.layer[vec].pwidth);
		fprintf(PS.fp, "        %s\n", pat);
		fprintf(PS.fp, "        end\n");
		fprintf(PS.fp, "      } bind\n>>\n");
		sprintf(pat, "APATT%d", vec);
		fprintf(PS.fp, " matrix\n makepattern /%s exch def\n", pat);
		fprintf(PS.fp, "/Pattern setcolorspace\n %s setcolor\n", pat);
	    }
	    else {
		set_ps_color(&color);
	    }

	    fprintf(PS.fp, "F\n");
	}
	if (vector.layer[vec].width > 0 &&
	    !(color_none(&vector.layer[vec].color))) {
	    fprintf(PS.fp, "%.8f W\n", vector.layer[vec].width);
	    set_ps_color(&(vector.layer[vec].color));
	    fprintf(PS.fp, "stroke\n");
	}
    }
    fprintf(PS.fp, "\n");
    fprintf(PS.fp, "0 setlinejoin\n");	/* reset line join to miter */
    return 1;
}
