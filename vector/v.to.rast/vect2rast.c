#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local.h"


int vect_to_rast(const char *vector_map, const char *raster_map, const char *field_name,
		 const char *column, int cache_mb, int use, double value,
		 int value_type, const char *rgbcolumn, const char *labelcolumn,
		 int ftype, char *where, char *cats, int dense)
{
    struct Map_info Map;
    struct line_pnts *Points;
    int i, j, field;
    struct cat_list *cat_list = NULL;
    int fd;			/* for raster map */
    int nareas, nlines;		/* number of converted features */
    int nareas_all, nplines_all;	/* number of all areas, points/lines */
    int stat;
    int format;
    int pass, npasses;

    /* Attributes */
    int nrec;
    int ctype = 0;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;
    int is_fp = 0;

    nareas = 0;

    G_verbose_message(_("Loading data..."));
    Vect_set_open_level(2);
    if (Vect_open_old2(&Map, vector_map, "", field_name) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), vector_map);

    field = Vect_get_field_number(&Map, field_name);

    if (field > 0)
	cat_list = Vect_cats_set_constraint(&Map, field, where, cats);


    if ((use == USE_Z) && !(Vect_is_3d(&Map)))
	G_fatal_error(_("Vector map <%s> is not 3D"),
		      Vect_get_full_name(&Map));

    switch (use) {
    case USE_ATTR:
	db_CatValArray_init(&cvarr);
	if (!(Fi = Vect_get_field(&Map, field)))
	    G_fatal_error(_("Database connection not defined for layer <%s>"),
			  field_name);

	if ((Driver =
	     db_start_driver_open_database(Fi->driver, Fi->database)) == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
        db_set_error_handler_driver(Driver);

	/* Note do not check if the column exists in the table because it may be expression */

	if ((nrec =
	     db_select_CatValArray(Driver, Fi->table, Fi->key, column, where,
				   &cvarr)) == -1)
	    G_fatal_error(_("Column <%s> not found"), column);
	G_debug(3, "nrec = %d", nrec);

	ctype = cvarr.ctype;
	if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Column type (%s) not supported (did you mean 'labelcolumn'?)"),
			  db_sqltype_name(ctype));

	if (nrec < 0)
	    G_fatal_error(_("No records selected from table <%s>"),
			  Fi->table);

	G_debug(1, "%d records selected from table", nrec);

	db_close_database_shutdown_driver(Driver);

	j = 0;
	for (i = 0; i < cvarr.n_values; i++) {
	    if (ctype == DB_C_TYPE_INT) {
		G_debug(3, "cat = %d val = %d", cvarr.value[i].cat,
			cvarr.value[i].val.i);
	    }
	    else if (ctype == DB_C_TYPE_DOUBLE) {
		G_debug(3, "cat = %d val = %f", cvarr.value[i].cat,
			cvarr.value[i].val.d);
	    }
	    /* check for null values */
	    if (!cat_list || Vect_cat_in_cat_list(cvarr.value[i].cat, cat_list)) {
		if (cvarr.value[i].isNull) {
		    j++;
		}
	    }
	}
	if (j) {
	    G_important_message(_("%d of %d records in column <%s> are empty and replaced with 0 (zero)"),
	                          j, nrec, column);
	}
	
	switch (ctype) {
	case DB_C_TYPE_INT:
	    format = CELL_TYPE;
	    break;
	case DB_C_TYPE_DOUBLE:
	    format = DCELL_TYPE;
	    break;
	default:
	    G_fatal_error(_("Unable to use column <%s>"), column);
	    break;
	}
	break;
    case USE_CAT:
	format = CELL_TYPE;
	break;
    case USE_VAL:
	format = value_type;
	break;
    case USE_Z:
	format = DCELL_TYPE;
	is_fp = 1;
	break;
    case USE_D:
	format = DCELL_TYPE;
	break;
    default:
	G_fatal_error(_("Unknown use type: %d"), use);
    }

    fd = Rast_open_new(raster_map, format);

    Points = Vect_new_line_struct();

    if (use != USE_Z && use != USE_D && (ftype & GV_AREA)) {
	if ((nareas = sort_areas(&Map, Points, field, cat_list)) == 0)
	    G_warning(_("No areas selected from vector map <%s>"),
			  vector_map);

	G_debug(1, "%d areas sorted", nareas);
    }
    if (nareas > 0 && dense) {
	G_warning(_("Area conversion and line densification are mutually exclusive, disabling line densification."));
	dense = 0;
    }

    nlines = Vect_get_num_primitives(&Map, ftype);
    nplines_all = nlines;
    npasses = begin_rasterization(cache_mb, format, dense);
    pass = 0;

    nareas_all = Vect_get_num_areas(&Map);

    do {
	pass++;

	if (npasses > 1)
	    G_message(_("Pass %d of %d:"), pass, npasses);

	stat = 0;

	if ((use != USE_Z && use != USE_D) && nareas) {
	    if (do_areas
		(&Map, Points, &cvarr, ctype, use, value,
		 value_type) < 0) {
		G_warning(_("Problem processing areas from vector map <%s>, continuing..."),
			  vector_map);
		stat = -1;
		break;
	    }
	}

	if (nlines) {
	    if ((nlines =
		 do_lines(&Map, Points, &cvarr, ctype, field, cat_list, 
		          use, value, value_type, ftype,
			  &nplines_all, dense)) < 0) {
		G_warning(_("Problem processing lines from vector map <%s>, continuing..."),
			  vector_map);
		stat = -1;
		break;
	    }
	}

	G_important_message(_("Writing raster map..."));

	stat = output_raster(fd);
    } while (stat == 0);

    G_suppress_warnings(0);
    /* stat: 0 == repeat; 1 == done; -1 == error; */

    Vect_destroy_line_struct(Points);

    if (stat < 0) {
	Rast_unopen(fd);

	return 1;
    }

    Vect_close(&Map);

    G_verbose_message(_("Creating support files for raster map..."));
    Rast_close(fd);
    update_hist(raster_map, vector_map, Map.head.orig_scale);

    /* colors */
    if (rgbcolumn) {
	if (use != USE_ATTR && use != USE_CAT) {
	    G_warning(_("Color can be updated from database only if use=attr"));
	}
	else {
	  update_dbcolors(raster_map, vector_map, field, rgbcolumn, is_fp,
			  column);
	}
    }

    update_cats(raster_map);

    /* labels */
    update_labels(raster_map, vector_map, field, labelcolumn, use, value,
		  column);

#if 0
    /* maximum possible numer of areas: number of centroids
     * actual number of areas, currently unknown:
     * number of areas with centroid that are within cat constraint
     * and overlap with current region */
    if (nareas_all > 0)
	G_message(_("Converted areas: %d of %d"), nareas,
	          Vect_get_num_primitives(&Map, GV_CENTROID));
    /* maximum possible numer of lines: number of GV_LINE + GV_POINT
     * actual number of lines, currently unknown:
     * number of lines are within cat constraint
     * and overlap with current region */
    if (nlines > 0 && nplines_all > 0)
	G_message(_("Converted points/lines: %d of %d"), nlines, nplines_all);
#endif

    return 0;
}
