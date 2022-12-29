
/****************************************************************************
 *
 * MODULE:       ps.map
 * FILE:         catval.c
 * AUTHOR(S):    Martin Landa <landa.martin@gmail.com>
 *		 Hamish Bowman
 * PURPOSE:      Support functions for loading dynamic symbol attributes
 *		 Used in PS_vpoints_plot()
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "vector.h"

/**
 * \fn int load_catval_array_rgb (struct Map_info* map, int vec, dbCatValArray* cvarr_rgb)
 *
 * \brief Loads categories and RGB color definition into dbCatValArray structure
 *
 * \param map: pointer to a vector Map_info structure
 * \param level: layer number identifier
 * \param level: output dbCatValArray structure
 *
 * \return number of records or -1 if it fails
 */

int load_catval_array_rgb(struct Map_info *map, int vec,
			  dbCatValArray * cvarr_rgb)
{
    int i, nrec, ctype;
    struct field_info *Fi;
    dbDriver *driver;

    G_debug(2, "Loading dynamic symbol colors ...");
    db_CatValArray_init(cvarr_rgb);

    Fi = Vect_get_field(map, vector.layer[vec].field);
    if (Fi == NULL) {
	G_fatal_error(_("Unable to get layer info for vector map"));
    }

    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    nrec = db_select_CatValArray(driver, Fi->table, Fi->key,
				 vector.layer[vec].rgbcol, NULL, cvarr_rgb);

    G_debug(3, "nrec_rgb = %d", nrec);

    ctype = cvarr_rgb->ctype;
    if (ctype != DB_C_TYPE_STRING)
	G_fatal_error(_("Column type not supported (must be string)"));

    if (nrec < 0)
	G_fatal_error(_("Unable to select data from table"));

    G_debug(2, "\nRGB column: %d records selected from table", nrec);

    for (i = 0; i < cvarr_rgb->n_values; i++) {
	G_debug(4, "cat = %d val = %s", cvarr_rgb->value[i].cat,
		cvarr_rgb->value[i].val.s->string);
    }

    db_close_database_shutdown_driver(driver);

    return nrec;
}



/**** Probably merge _size and _rot fns into a single _double() fn; all that is
 **** needed is to pass the column name from the vector struct to the fn as well.
 */

/**
 * \fn int load_catval_array_size (struct Map_info* map, int vec, dbCatValArray* cvarr_size)
 *
 * \brief Loads categories and dynamic size values into dbCatValArray structure
 *
 * \param map: pointer to a vector Map_info structure
 * \param level: layer number identifier
 * \param level: output dbCatValArray structure
 *
 * \return number of records or -1 if it fails
 */

int load_catval_array_size(struct Map_info *map, int vec,
			   dbCatValArray * cvarr_size)
{
    int i, nrec, ctype;
    struct field_info *Fi;
    dbDriver *driver;

    G_debug(2, "Loading dynamic symbol sizes ...");
    db_CatValArray_init(cvarr_size);

    Fi = Vect_get_field(map, vector.layer[vec].field);
    if (Fi == NULL) {
	G_fatal_error(_("Unable to get layer info for vector map"));
    }

    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    /* Note do not check if the column exists in the table because it may be expression */

    /* TODO: only select values we need instead of all in column */
    nrec = db_select_CatValArray(driver, Fi->table, Fi->key,
				 vector.layer[vec].sizecol, NULL, cvarr_size);

    G_debug(3, "nrec = %d", nrec);

    ctype = cvarr_size->ctype;
    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	G_fatal_error(_("Size column type must be numeric"));

    if (nrec < 0)
	G_fatal_error(_("Unable to select data from table"));
    G_debug(2, "\nSize column: %d records selected from table", nrec);

    db_close_database_shutdown_driver(driver);

    for (i = 0; i < cvarr_size->n_values; i++) {
	if (ctype == DB_C_TYPE_INT) {
	    G_debug(4, "cat = %d val = %d", cvarr_size->value[i].cat,
		    cvarr_size->value[i].val.i);
	}
	else if (ctype == DB_C_TYPE_DOUBLE) {
	    G_debug(4, "cat = %d val = %f", cvarr_size->value[i].cat,
		    cvarr_size->value[i].val.d);
	}
    }

    return nrec;
}



/**
 * \fn int load_catval_array_rot (struct Map_info* map, int vec, dbCatValArray* cvarr_rot)
 *
 * \brief Loads categories and dynamic rotation values into dbCatValArray structure
 *
 * \param map: pointer to a vector Map_info structure
 * \param level: layer number identifier
 * \param level: output dbCatValArray structure
 *
 * \return number of records or -1 if it fails
 */

int load_catval_array_rot(struct Map_info *map, int vec,
			  dbCatValArray * cvarr_rot)
{
    int i, nrec, ctype;
    struct field_info *Fi;
    dbDriver *driver;

    G_debug(2, "Loading dynamic symbol rotation ...");
    db_CatValArray_init(cvarr_rot);

    Fi = Vect_get_field(map, vector.layer[vec].field);
    if (Fi == NULL) {
	G_fatal_error(_("Unable to get layer info for vector map"));
    }

    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    /* Note do not check if the column exists in the table because it may be expression */

    /* TODO: only select values we need instead of all in column */
    nrec = db_select_CatValArray(driver, Fi->table, Fi->key,
				 vector.layer[vec].rotcol, NULL, cvarr_rot);

    G_debug(3, "nrec = %d", nrec);

    ctype = cvarr_rot->ctype;

    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	G_fatal_error(_("Rotation column type must be numeric"));

    if (nrec < 0)
	G_fatal_error(_("Unable to select data from table"));

    G_debug(2, "\nRotate column: %d records selected from table", nrec);

    db_close_database_shutdown_driver(driver);

    for (i = 0; i < cvarr_rot->n_values; i++) {
	if (ctype == DB_C_TYPE_INT) {
	    G_debug(4, "cat = %d val = %d", cvarr_rot->value[i].cat,
		    cvarr_rot->value[i].val.i);
	}
	else if (ctype == DB_C_TYPE_DOUBLE) {
	    G_debug(4, "cat = %d val = %f", cvarr_rot->value[i].cat,
		    cvarr_rot->value[i].val.d);
	}
    }

    return nrec;
}
