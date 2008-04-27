#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "local.h"


int vect_to_rast(char *vector_map,char *raster_map, int field, char *column, int nrows, 
	          int use, double value, int value_type, char *rgbcolumn, 
                  char *labelcolumn, int ftype)
{
#ifdef DEBUG
    int i;
#endif
    char *vector_mapset;
    struct Map_info Map;
    struct line_pnts *Points;
    int fd;	                          /* for raster map */
    int nareas, nlines;                   /* number of converted features */
    int nareas_all, nplines_all;          /* number of all areas, points/lines */
    int stat;
    int format;
    int pass, npasses;

    /* Attributes */
    int nrec;
    int ctype;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;
    int is_fp = 0;

    nareas = 0;

    if ((vector_mapset = G_find_vector2 (vector_map, "")) == NULL)
	G_fatal_error (_("Vector map <%s> not found"), vector_map);

    G_debug (1, "Loading vector information...");
    Vect_set_open_level (2);
    Vect_open_old (&Map, vector_map, vector_mapset);

    if ( (use == USE_Z) && !(Vect_is_3d(&Map)) )
	G_fatal_error (_("Vector map <%s> is not 3D"), Vect_get_full_name(&Map));

    switch (use)
    {
    case USE_ATTR:
	db_CatValArray_init ( &cvarr );
        if (!(Fi = Vect_get_field (&Map, field)))
	    G_fatal_error (_("Database connection not defined for layer %d"),
			   field);

	if ((Driver = db_start_driver_open_database ( Fi->driver, Fi->database)) == NULL)
	    G_fatal_error (_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver);

	/* Note do not check if the column exists in the table because it may be expression */

	if ((nrec = db_select_CatValArray ( Driver, Fi->table, Fi->key, column, NULL, &cvarr )) == -1 )
	   G_fatal_error( _("Column <%s> not found"), column);
	G_debug (3, "nrec = %d", nrec );

	ctype = cvarr.ctype;
	if ( ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE )
	    G_fatal_error (_("Column type (%s) not supported (did you mean 'labelcolumn'?)"),
			   db_sqltype_name(ctype));

	if ( nrec < 0 )
            G_fatal_error (_("No records selected from table <%s>"),
			   Fi->table);

	G_debug (1, "%d records selected from table", nrec);

	db_close_database_shutdown_driver(Driver);

#ifdef DEBUG
	for ( i = 0; i < cvarr.n_values; i++ ) {
	    if ( ctype == DB_C_TYPE_INT ) {
		G_debug (3, "cat = %d val = %d", cvarr.value[i].cat, cvarr.value[i].val.i );
	    } else if ( ctype == DB_C_TYPE_DOUBLE ) {
		G_debug (3, "cat = %d val = %f", cvarr.value[i].cat, cvarr.value[i].val.d );
	    }
	}
#endif

        switch (ctype)
        {
        case DB_C_TYPE_INT:
            format = USE_CELL;
        break;
        case DB_C_TYPE_DOUBLE:
            format = USE_DCELL;
        break;
        default:
            G_fatal_error (_("Unable to use column <%s>"), column);
        break;
        }
    break;
    case USE_CAT:
	format = USE_CELL;
    break;
    case USE_VAL:
	format = value_type;
    break;
    case USE_Z:
	format = USE_DCELL;
        is_fp  = 1;
    break;
    case USE_D:
        format = USE_DCELL;
    break;
    default:
        G_fatal_error (_("Unknown use type: %d"), use);
    }

    switch (format)
    {
    case USE_CELL:
        if ((fd = G_open_cell_new (raster_map)) < 0)
            G_fatal_error (_("Unable to create raster map <%s>"), raster_map);
    break;
    case USE_DCELL:
        if ((fd = G_open_raster_new (raster_map, DCELL_TYPE)) < 0)
            G_fatal_error (_("Unable to create raster map <%s>"), raster_map);
    break;
    default:
        G_fatal_error (_("Unknown raster map type"));
    break;
    }

    Points = Vect_new_line_struct ();

    if (use != USE_Z && use != USE_D && (ftype & GV_AREA))
    {
        if ((nareas = sort_areas (&Map, Points, field)) < 0)
            G_fatal_error (_("Unable to process areas from vector map <%s>"), vector_map);

	G_debug (1, "%d areas sorted", nareas);
    }

    nlines = 1;
    npasses = begin_rasterization(nrows, format);
    pass = 0;

    nareas_all = Vect_get_num_areas (&Map);

    do {
	pass++;

	if (npasses > 1)
            G_message (_("Pass %d of %d:"), pass, npasses);

	stat = 0;

	if ( (use != USE_Z && use != USE_D) && nareas ) {
	    if(do_areas (&Map, Points, &cvarr, ctype, field, use, value, value_type) < 0) {
		G_warning (_("Problem processing areas from vector map <%s>, continuing..."), vector_map);
		stat = -1;
		break;
	    }
	}

	if (nlines) {
	    if((nlines = do_lines (&Map, Points, &cvarr, ctype, field, use, value, value_type, ftype, &nplines_all)) < 0) {
		G_warning (_("Problem processing lines from vector map <%s>, continuing..."), vector_map);
		stat = -1;
		break;
	    }
	}

	G_message (_("Converted areas: %d of %d"), nareas, nareas_all);
	G_message (_("Converted points/lines: %d of %d"), nlines, nplines_all);

	G_debug (1, "Writing raster map ...");

	stat = output_raster(fd);
    } while (stat == 0);

    /* stat: 0 == repeat; 1 == done; -1 == error; */

    Vect_destroy_line_struct (Points);

    if (stat < 0) {
	G_unopen_cell(fd);

	return 1;
    }

    Vect_close ( &Map );

    G_debug (1, "Creating support files for raster map...");
    G_close_cell(fd);
    update_hist(raster_map, vector_map, vector_mapset, Map.head.orig_scale);

    /* colors */
    if (rgbcolumn) {
        if (use != USE_ATTR) {
            G_warning(_("Color can be updated from database only if use=attr"));
            update_colors (raster_map);
        }

        update_dbcolors(raster_map, vector_map, field, rgbcolumn, is_fp, column);
    }
    else if (use == USE_D)
        update_fcolors(raster_map);
    else
        update_colors (raster_map);

    update_cats (raster_map);

    /* labels */
    update_labels (raster_map, vector_map, field, labelcolumn, use, value, column);

    return 0;
}
