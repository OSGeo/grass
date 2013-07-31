
/****************************************************************************
 *
 * MODULE:       r.to.vect
 *
 * AUTHOR(S):    Bill Brown, Mike Baba, Jean Ezell and Andrew Heekin,
 *               David Satnik, Andrea Aime, Radim Blazek
 *
 * PURPOSE:      Converts a raster map into a vector map layer
 *
 * COPYRIGHT:    (C) 2007, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "global.h"

/* 
 * Attributes for lines are ignored. For points and area by default unique new
 * category is assigned to each and raster value is written to 'value' column.
 * Labels are written to 'label' column if exists. If value flag (-v) is used
 * and type is CELL, raster values are used as categories. 
 *
 * 2007/2: attributes for lines supported
 */

int data_type;
int data_size;
struct Map_info Map;
int input_fd;			/* input raster map descriptor */
struct line_cats *Cats;
struct Cell_head cell_head;

int direction;
int first_read, last_read;
int input_fd;
int row_length, row_count, n_rows;
int total_areas;

int smooth_flag;		/* this is 0 for no smoothing, 1 for smoothing of lines */
int value_flag;			/* use raster values as categories */
struct Categories RastCats;
int has_cats;			/* Category labels available */
struct field_info *Fi;
dbDriver *driver;
dbString sql, label;

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *in_opt, *out_opt, *feature_opt, *column_name;
    struct Flag *smooth_flg, *value_flg, *z_flg, *no_topol, *notab_flg;
    int feature, notab_flag;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("conversion"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("vectorization"));
    module->description = _("Converts a raster map into a vector map.");

    in_opt = G_define_standard_option(G_OPT_R_INPUT);

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    
    feature_opt = G_define_standard_option(G_OPT_V_TYPE);
    feature_opt->required = YES;
    feature_opt->multiple = NO;
    feature_opt->options = "point,line,area";
    feature_opt->answer = NULL;

    column_name = G_define_standard_option(G_OPT_DB_COLUMN);
    column_name->label = _("Name of attribute column to store value");
    column_name->description = _("Name must be SQL compliant");
    column_name->answer = "value";

    smooth_flg = G_define_flag();
    smooth_flg->key = 's';
    smooth_flg->description = _("Smooth corners of area features");

    value_flg = G_define_flag();
    value_flg->key = 'v';
    value_flg->description =
	_("Use raster values as categories instead of unique sequence (CELL only)");
    value_flg->guisection = _("Attributes");

    z_flg = G_define_flag();
    z_flg->key = 'z';
    z_flg->label = _("Write raster values as z coordinate");
    z_flg->description = _("Table is not created. "
			   "Currently supported only for points.");
    z_flg->guisection = _("Attributes");

    no_topol = G_define_flag();
    no_topol->key = 'b';
    no_topol->label = _("Do not build vector topology");
    no_topol->description = _("Recommended for massive point conversion");

    notab_flg = G_define_standard_flag(G_FLG_V_TABLE);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    feature = Vect_option_to_types(feature_opt);
    smooth_flag = (smooth_flg->answer) ? SMOOTH : NO_SMOOTH;
    value_flag = value_flg->answer;
    notab_flag = notab_flg->answer;

    if (z_flg->answer && (feature != GV_POINT))
	G_fatal_error(_("z flag is supported only for points"));

    /* Open files */
    input_fd = Rast_open_old(in_opt->answer, "");

    data_type = Rast_get_map_type(input_fd);
    data_size = Rast_cell_size(data_type);
    G_get_window(&cell_head);

    if (value_flag && data_type != CELL_TYPE) {
	if (!notab_flag)
	    G_warning(_("Raster is not CELL, '-v' flag ignored, raster values will be written to the table."));
	else if (z_flg->answer)
	    G_warning(_("Raster is not CELL, '-v' flag ignored, raster values will be z coordinate."));
	else
	    G_warning(_("Raster is not CELL, '-v' flag ignored, raster values will be lost."));
	value_flag = 0;
    }

    if (!value_flag && notab_flag) {
	G_warning(_("Categories will be unique sequence, raster values will be lost."));
    }

    if (z_flg->answer)
	Vect_open_new(&Map, out_opt->answer, 1);
    else
	Vect_open_new(&Map, out_opt->answer, 0);

    Vect_hist_command(&Map);

    Cats = Vect_new_cats_struct();

    /* Open category labels */
    if (data_type == CELL_TYPE) {
	if (0 == Rast_read_cats(in_opt->answer, "", &RastCats))
	    has_cats = 1;
    }
    else
	has_cats = 0;

    db_init_string(&sql);
    db_init_string(&label);

    /* Create table */
    if ((feature & (GV_AREA | GV_POINT | GV_LINE)) &&
	(!value_flag || (value_flag && has_cats)) && !(z_flg->answer)
	&& !notab_flag) {
	char buf[1000];

	Fi = Vect_default_field_info(&Map, 1, NULL, GV_1TABLE);
	Vect_map_add_dblink(&Map, 1, NULL, Fi->table, GV_KEY_COLUMN, Fi->database,
			    Fi->driver);

	driver =
	    db_start_driver_open_database(Fi->driver,
					  Vect_subst_var(Fi->database, &Map));
	if (driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);

	/* Create new table */
	db_zero_string(&sql);
	sprintf(buf, "create table %s ( cat integer", Fi->table);
	db_append_string(&sql, buf);

	if (!value_flag) {	/* add value to the table */
	    if (data_type == CELL_TYPE) {
		db_append_string(&sql, ", ");
		db_append_string(&sql, column_name->answer);
		db_append_string(&sql, " integer");
	    } else {
		db_append_string(&sql, ",");
		db_append_string(&sql, column_name->answer);
		db_append_string(&sql, " double precision");
	    }
	}

	if (has_cats) {
	    int i, len;
	    int clen = 0;

	    /* Get maximum column length */
	    for (i = 0; i < RastCats.ncats; i++) {
		len = strlen(RastCats.labels[i]);
		if (len > clen)
		    clen = len;
	    }
	    clen += 10;

	    sprintf(buf, ", label varchar(%d)", clen);
	    db_append_string(&sql, buf);
	}

	db_append_string(&sql, ")");

	G_debug(3, db_get_string(&sql));

	if (db_execute_immediate(driver, &sql) != DB_OK)
	    G_fatal_error(_("Unable to create table: %s"),
			  db_get_string(&sql));

	if (db_create_index2(driver, Fi->table, GV_KEY_COLUMN) != DB_OK)
	    G_warning(_("Unable to create index"));

	if (db_grant_on_table
	    (driver, Fi->table, DB_PRIV_SELECT,
	     DB_GROUP | DB_PUBLIC) != DB_OK)
	    G_fatal_error(_("Unable to grant privileges on table <%s>"),
			  Fi->table);

	db_begin_transaction(driver);

    }
    else {
	driver = NULL;
    }

    /* init variables for lines and areas */
    first_read = 1;
    last_read = 0;
    direction = FORWARD;
    row_length = cell_head.cols;
    n_rows = cell_head.rows;
    row_count = 0;

    if (feature == GV_LINE) {
	alloc_lines_bufs(row_length + 2);
	extract_lines();
    }
    else if (feature == GV_AREA) {
	alloc_areas_bufs(row_length + 2);
	extract_areas();
    }
    else {			/* GV_POINT */

	extract_points(z_flg->answer);
    }

    Rast_close(input_fd);

    if (!no_topol->answer)
	Vect_build(&Map);


    /* insert cats and optionally labels if raster cats were used */
    if (driver && value_flag) {
	char buf[1000];
	int c, i, cat, fidx, ncats, lastcat, tp, id;

	fidx = Vect_cidx_get_field_index(&Map, 1);
	if (fidx >= 0) {
	    ncats = Vect_cidx_get_num_cats_by_index(&Map, fidx);
	    lastcat = -1;
            
            G_important_message(_("Updating attributes..."));
	    for (c = 0; c < ncats; c++) {
		Vect_cidx_get_cat_by_index(&Map, fidx, c, &cat, &tp, &id);

		if (lastcat == cat)
		    continue;

		/* find label, slow -> TODO faster */
		db_set_string(&label, "");
		for (i = 0; i < RastCats.ncats; i++) {
		    if (cat == (int)RastCats.q.table[i].dLow) {	/* cats are in dLow/High not in cLow/High !!! */
			db_set_string(&label, RastCats.labels[i]);
			db_double_quote_string(&label);
			break;
		    }
		}
		G_debug(3, "cat = %d label = %s", cat, db_get_string(&label));

		sprintf(buf, "insert into %s values ( %d, '%s')", Fi->table,
			cat, db_get_string(&label));
		db_set_string(&sql, buf);
		G_debug(3, db_get_string(&sql));

		if (db_execute_immediate(driver, &sql) != DB_OK)
		    G_fatal_error(_("Unable to insert into table: %s"),
				  db_get_string(&sql));

		lastcat = cat;
	    }
	}
    }

    if (has_cats)
	Rast_free_cats(&RastCats);

    if (driver != NULL) {
	db_commit_transaction(driver);
	db_close_database_shutdown_driver(driver);
    }

    Vect_close(&Map);
    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
