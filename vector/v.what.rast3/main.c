/* ***************************************************************
 *
 * MODULE:       v.what.rast3
 *  
 * AUTHOR(S):    Soeren Gebbert, this code is based on a slightly modified version of 
 *               v.what.rast from Radim Blazek and Michael Shapiro
 *                
 *  PURPOSE:      Uploads 3d raster values at positions of vector points to the table
 *                
 *  COPYRIGHT:    (C) 2001, 2011 by the GRASS Development Team
 * 
 *                This program is free software under the GNU General
 *                Public License (>=v2).  Read the file COPYING that
 *                comes with GRASS for details.
 * 
 * **************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/raster3d.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    int i, j, nlines, type, field, cat;

    char buf[2048];
    struct {
	struct Option *vect, *rast3d, *field, *col, *where;
    } opt;
    int Cache_size;
    struct order *cache;
    struct GModule *module;

    RASTER3D_Region region;
    RASTER3D_Map *map;
    
    struct Map_info Map;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int point;
    int point_cnt;		/* number of points in cache */
    int outside_cnt;		/* points outside region */
    int nocat_cnt;		/* points inside region but without category */
    int dupl_cnt;		/* duplicate categories */
    int typeIntern;
    int is_empty;
    struct bound_box box;

    int *catexst, *cex;
    struct field_info *Fi;
    dbString stmt;
    dbDriver *driver;
    int select, norec_cnt, update_cnt, upderr_cnt, col_type;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("sampling"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("position"));
    G_add_keyword(_("querying"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("surface information"));
    module->description =
	_("Uploads 3D raster values at positions of vector points to the table.");

    opt.vect = G_define_standard_option(G_OPT_V_MAP);
    opt.vect->label =
	_("Name of vector points map for which to edit attributes");

    opt.field = G_define_standard_option(G_OPT_V_FIELD);

    opt.rast3d = G_define_standard_option(G_OPT_R3_MAP);
    opt.rast3d->key = "raster_3d";
    opt.rast3d->description = _("Name of existing 3D raster map to be queried");

    opt.col = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.col->required = YES;
    opt.col->description =
	_("Name of attribute column to be updated with the query result");

    opt.where = G_define_standard_option(G_OPT_DB_WHERE);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    db_init_string(&stmt);
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* Initiate the default settings */
    Rast3d_init_defaults();

    /* Figure out the current region settings */
    Rast3d_get_window(&region);
    
    box.N = region.north;
    box.S = region.south;
    box.E = region.east;
    box.W = region.west;
    box.T = region.top;
    box.B = region.bottom;

    /* Open vector */
    Vect_set_open_level(2);
    if (Vect_open_old2(&Map, opt.vect->answer, "", opt.field->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), opt.vect->answer);

    field = Vect_get_field_number(&Map, opt.field->answer);

    Fi = Vect_get_field(&Map, field);
    if (Fi == NULL)
	G_fatal_error(_("Database connection not defined for layer %d"),
		      field);

    /* Open driver */
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL) {
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);
    }
    db_set_error_handler_driver(driver);

    map = Rast3d_open_cell_old(opt.rast3d->answer, G_find_raster3d(opt.rast3d->answer, ""), &region,
                          RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);

    if (map == NULL)
        G_fatal_error(_("Error opening 3D raster map <%s>"), opt.rast3d->answer);

    /* Check column type */
    col_type = db_column_Ctype(driver, Fi->table, opt.col->answer);

    if (col_type == -1)
	G_fatal_error(_("Column <%s> not found"), opt.col->answer);

    if (col_type != DB_C_TYPE_DOUBLE)
	G_fatal_error(_("Column type not supported, please use a column with double type"));

    /* Read vector points to cache */
    Cache_size = Vect_get_num_primitives(&Map, GV_POINT);
    /* Note: Some space may be wasted (outside region or no category) */

    cache = (struct order *)G_calloc(Cache_size, sizeof(struct order));

    point_cnt = outside_cnt = nocat_cnt = 0;

    nlines = Vect_get_num_lines(&Map);

    G_debug(1, "Reading %d vector features from map", nlines);

    G_important_message(_("Reading features from vector map..."));
    for (i = 1; i <= nlines; i++) {
	type = Vect_read_line(&Map, Points, Cats, i);
	G_debug(4, "line = %d type = %d", i, type);

	G_percent(i, nlines, 2);
        
	/* check type */
	if (!(type & GV_POINT))
	    continue;		/* Points only */

	/* check region */
	if (!Vect_point_in_box(Points->x[0], Points->y[0], Points->z[0], &box)) {
	    outside_cnt++;
	    continue;
	}

	Vect_cat_get(Cats, field, &cat);
	if (cat < 0) {		/* no category of given field */
	    nocat_cnt++;
	    continue;
	}

	G_debug(4, "    cat = %d", cat);

	cache[point_cnt].x = Points->x[0];
	cache[point_cnt].y = Points->y[0];
	cache[point_cnt].z = Points->z[0];
	cache[point_cnt].cat = cat;
	cache[point_cnt].count = 1;
	point_cnt++;
    }

    Vect_set_db_updated(&Map);
    Vect_hist_command(&Map);
    Vect_set_db_updated(&Map);
    Vect_close(&Map);

    G_debug(1, "Read %d vector points", point_cnt);
    /* Cache may contain duplicate categories, sort by cat, find and remove duplicates 
     * and recalc count and decrease point_cnt  */
    qsort(cache, point_cnt, sizeof(struct order), by_cat);

    G_debug(1, "Points are sorted, starting duplicate removal loop");

    for (i = 0, j = 1; j < point_cnt; j++)
	if (cache[i].cat != cache[j].cat)
	    cache[++i] = cache[j];
	else
	    cache[i].count++;
    point_cnt = i + 1;

    G_debug(1, "%d vector points left after removal of duplicates",
	    point_cnt);

    /* Report number of points not used */
    if (outside_cnt)
	G_warning(n_("%d point outside current region was skipped",
                     "%d points outside current region were skipped",
                     outside_cnt), outside_cnt);

    if (nocat_cnt)
	G_warning(n_("%d point without category was skipped", 
                     "%d points without category were skipped",
                     nocat_cnt), nocat_cnt);

    /* Extract raster values from file and store in cache */
    G_debug(1, "Extracting raster values");
    
    typeIntern = Rast3d_tile_type_map(map);
    
    /* Sample the 3d raster map */
    for (point = 0; point < point_cnt; point++) {
        
	if (cache[point].count > 1)
	    continue;		/* duplicate cats */
        
        if(typeIntern == FCELL_TYPE) {
            Rast3d_get_window_value(map, cache[point].y, cache[point].x, cache[point].z,
                       &cache[point].fvalue, FCELL_TYPE);
        } else {
            Rast3d_get_window_value(map, cache[point].y, cache[point].x, cache[point].z,
                       &cache[point].dvalue, DCELL_TYPE);
        }
    }

    Rast3d_close(map);
    
    /* Update table from cache */
    G_debug(1, "Updating db table");

    /* select existing categories to array (array is sorted) */
    select = db_select_int(driver, Fi->table, Fi->key, NULL, &catexst);

    db_begin_transaction(driver);

    norec_cnt = update_cnt = upderr_cnt = dupl_cnt = 0;
    
    G_message("Update vector attributes...");
    for (point = 0; point < point_cnt; point++) {
	if (cache[point].count > 1) {
	    G_warning(_("More points (%d) of category %d, value set to 'NULL'"),
		      cache[point].count, cache[point].cat);
	    dupl_cnt++;
	}

	G_percent(point, point_cnt, 2);
	
	/* category exist in DB ? */
	cex =
	    (int *)bsearch((void *)&(cache[point].cat), catexst, select,
			   sizeof(int), srch_cat);
	if (cex == NULL) {	/* cat does not exist in DB */
	    norec_cnt++;
	    G_warning(_("No record for category %d in table <%s>"),
		      cache[point].cat, Fi->table);
	    continue;
	}

	G_snprintf(buf, 2048, "update %s set %s = ", Fi->table, opt.col->answer);

	db_set_string(&stmt, buf);
        
        is_empty = 0;

        if (cache[point].count > 1)
            is_empty = 1;
        if(typeIntern == FCELL_TYPE)
            is_empty = Rast3d_is_null_value_num(&cache[point].fvalue, FCELL_TYPE);
        if(typeIntern == DCELL_TYPE)
            is_empty = Rast3d_is_null_value_num(&cache[point].dvalue, DCELL_TYPE);

        if (is_empty) {
            G_snprintf(buf, 2048, "NULL");
        }else {
            if(typeIntern == FCELL_TYPE)
                G_snprintf(buf, 2048, "%.10f", cache[point].fvalue);
            if(typeIntern == DCELL_TYPE)
                G_snprintf(buf, 2048, "%.15f", cache[point].dvalue);
        }
        
	db_append_string(&stmt, buf);

	G_snprintf(buf, 2048, " where %s = %d", Fi->key, cache[point].cat);
        
	db_append_string(&stmt, buf);
	/* user provides where condition: */
	if (opt.where->answer) {
	    G_snprintf(buf, 2048, " AND %s", opt.where->answer);
	    db_append_string(&stmt, buf);
	}
	G_debug(3, "%s", db_get_string(&stmt));

	/* Update table */
	if (db_execute_immediate(driver, &stmt) == DB_OK) {
	    update_cnt++;
	}
	else {
	    upderr_cnt++;
	}
    }
    G_percent(1, 1, 1);
    
    G_debug(1, "Committing DB transaction");
    db_commit_transaction(driver);

    G_free(catexst);
    db_close_database_shutdown_driver(driver);
    db_free_string(&stmt);

    /* Report */
    G_verbose_message(n_("%d category loaded from table",
                         "%d categories loaded from table",
                         select), select);
    G_verbose_message(n_("%d category loaded from vector",
                         "%d categories loaded from vector",
                         point_cnt), point_cnt);
    G_verbose_message(n_("%d category from vector missing in table",
                         "%d categories from vector missing in table",
                         norec_cnt), norec_cnt);
    if (dupl_cnt > 0)
	G_message(n_("%d duplicate category in vector",
                     "%d duplicate categories in vector",
                     dupl_cnt), dupl_cnt);
    if (upderr_cnt > 0)
	G_warning(n_("%d update error",
                     "%d update errors",
                     upderr_cnt), upderr_cnt);

    G_done_msg(n_("%d record updated.",
                  "%d records updated.",
                  update_cnt), update_cnt);
        
    exit(EXIT_SUCCESS);
}
