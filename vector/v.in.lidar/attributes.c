/*
 * attribute handling
 *
 * Copyright 2011-2015 by Markus Metz, and the GRASS Development Team
 * Authors:
 *  Markus Metz (v.in.lidar)
 *  Vaclav Petras (move code to standalone functions)
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#include <liblas/capi/liblas.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#include "lidar.h"

/*
 * Caller must execute
 * db_commit_transaction(driver);
 * db_close_database_shutdown_driver(driver);
 * when done.
 */
void create_table_for_lidar(struct Map_info *vector_map, const char *name,
                            int layer, dbDriver **db_driver,
                            struct field_info **finfo, int have_time,
                            int have_color)
{
    char buf[2000];
    dbString sql;

    db_init_string(&sql);

    char *cat_col_name = GV_KEY_COLUMN;

    struct field_info *Fi =
        Vect_default_field_info(vector_map, layer, NULL, GV_1TABLE);

    Vect_map_add_dblink(vector_map, layer, name, Fi->table, cat_col_name,
                        Fi->database, Fi->driver);

    /* check available LAS info, depends on POINT DATA RECORD FORMAT [0-5] */
    /* X (double),
     * Y (double),
     * Z (double),
     * intensity (double),
     * return number (int),
     * number of returns (int),
     * scan direction (int),
     * flight line edge (int),
     * classification type (char),
     * class (char),
     * time (double) (FORMAT 1, 3, 4, 5),
     * scan angle rank (int),
     * source ID (int),
     * user data (char), ???
     * red (int)  (FORMAT 2, 3, 5),
     * green (int) (FORMAT 2, 3, 5),
     * blue (int) (FORMAT 2, 3, 5)*/

    /* Create table */
    sprintf(buf, "create table %s (%s integer", Fi->table, cat_col_name);
    db_set_string(&sql, buf);

    /* x, y, z */
    sprintf(buf, ", x_coord double precision");
    db_append_string(&sql, buf);
    sprintf(buf, ", y_coord double precision");
    db_append_string(&sql, buf);
    sprintf(buf, ", z_coord double precision");
    db_append_string(&sql, buf);
    /* intensity */
    sprintf(buf, ", intensity integer");
    db_append_string(&sql, buf);
    /* return number */
    sprintf(buf, ", return integer");
    db_append_string(&sql, buf);
    /* number of returns */
    sprintf(buf, ", n_returns integer");
    db_append_string(&sql, buf);
    /* scan direction */
    sprintf(buf, ", scan_dir integer");
    db_append_string(&sql, buf);
    /* flight line edge */
    sprintf(buf, ", edge integer");
    db_append_string(&sql, buf);
    /* classification type */
    sprintf(buf, ", cl_type varchar(20)");
    db_append_string(&sql, buf);
    /* classification class */
    sprintf(buf, ", class varchar(40)");
    db_append_string(&sql, buf);
    /* GPS time */
    if (have_time) {
        sprintf(buf, ", gps_time double precision");
        db_append_string(&sql, buf);
    }
    /* scan angle */
    sprintf(buf, ", angle integer");
    db_append_string(&sql, buf);
    /* source id */
    sprintf(buf, ", src_id integer");
    db_append_string(&sql, buf);
    /* user data */
    sprintf(buf, ", usr_data integer");
    db_append_string(&sql, buf);
    /* colors */
    if (have_color) {
        sprintf(buf, ", red integer, green integer, blue integer");
        db_append_string(&sql, buf);
        sprintf(buf, ", GRASSRGB varchar(11)");
        db_append_string(&sql, buf);
    }

    db_append_string(&sql, ")");
    G_debug(3, "%s", db_get_string(&sql));

    dbDriver *driver = db_start_driver_open_database(
        Fi->driver, Vect_subst_var(Fi->database, vector_map));

    if (driver == NULL) {
        G_fatal_error(_("Unable open database <%s> by driver <%s>"),
                      Vect_subst_var(Fi->database, vector_map), Fi->driver);
    }
    db_set_error_handler_driver(driver);

    if (db_execute_immediate(driver, &sql) != DB_OK) {
        G_fatal_error(_("Unable to create table: '%s'"), db_get_string(&sql));
    }

    if (db_create_index2(driver, Fi->table, cat_col_name) != DB_OK)
        G_warning(_("Unable to create index for table <%s>, key <%s>"),
                  Fi->table, cat_col_name);

    if (db_grant_on_table(driver, Fi->table, DB_PRIV_SELECT,
                          DB_GROUP | DB_PUBLIC) != DB_OK)
        G_fatal_error(_("Unable to grant privileges on table <%s>"), Fi->table);

    db_begin_transaction(driver);

    *db_driver = driver;
    *finfo = Fi;
}

void las_point_to_attributes(struct field_info *Fi, dbDriver *driver, int cat,
                             LASPointH LAS_point, double x, double y, double z,
                             int have_time, int have_color)
{
    static char buf[2000];
    static dbString sql;

    /* unfortunately we have to do allocation every time because
     * we need to clean the string the first time
     * if desired, we could rely on static variable being initialized
     * to 0 (at least C99) which is what the function is currently doing */
    db_init_string(&sql);

    char class_flag;
    int las_class_type, las_class;

    /* use LASPoint_Validate (LASPointH hPoint) to check for
     * return number, number of returns, scan direction, flight line edge,
     * classification, scan angle rank */
    sprintf(buf, "insert into %s values ( %d", Fi->table, cat);
    db_set_string(&sql, buf);

    /* x, y, z */
    sprintf(buf, ", %f", x);
    db_append_string(&sql, buf);
    sprintf(buf, ", %f", y);
    db_append_string(&sql, buf);
    sprintf(buf, ", %f", z);
    db_append_string(&sql, buf);
    /* intensity */
    sprintf(buf, ", %d", LASPoint_GetIntensity(LAS_point));
    db_append_string(&sql, buf);
    /* return number */
    sprintf(buf, ", %d", LASPoint_GetReturnNumber(LAS_point));
    db_append_string(&sql, buf);
    /* number of returns */
    sprintf(buf, ",  %d", LASPoint_GetNumberOfReturns(LAS_point));
    db_append_string(&sql, buf);
    /* scan direction */
    sprintf(buf, ", %d", LASPoint_GetScanDirection(LAS_point));
    db_append_string(&sql, buf);
    /* flight line edge */
    sprintf(buf, ",  %d", LASPoint_GetFlightLineEdge(LAS_point));
    db_append_string(&sql, buf);
    class_flag = LASPoint_GetClassification(LAS_point);
    /* classification type int or char ? */
    las_class_type = class_flag / 32;
    sprintf(buf, ", \'%s\'", class_type[las_class_type].name);
    db_append_string(&sql, buf);
    /* classification class int or char ? */
    las_class = class_flag % 32;
    if (las_class > 13)
        las_class = 13;
    sprintf(buf, ", \'%s\'", class_val[las_class].name);
    db_append_string(&sql, buf);
    /* GPS time */
    if (have_time) {
        sprintf(buf, ", %f", LASPoint_GetTime(LAS_point));
        db_append_string(&sql, buf);
    }
    /* scan angle */
    sprintf(buf, ", %d", LASPoint_GetScanAngleRank(LAS_point));
    db_append_string(&sql, buf);
    /* source id */
    sprintf(buf, ", %d", LASPoint_GetPointSourceId(LAS_point));
    db_append_string(&sql, buf);
    /* user data */
    sprintf(buf, ", %d", LASPoint_GetUserData(LAS_point));
    db_append_string(&sql, buf);
    /* colors */
    if (have_color) {
        LASColorH LAS_color = LASPoint_GetColor(LAS_point);
        int red = LASColor_GetRed(LAS_color);
        int green = LASColor_GetGreen(LAS_color);
        int blue = LASColor_GetBlue(LAS_color);

        sprintf(buf, ", %d, %d, %d", red, green, blue);
        db_append_string(&sql, buf);
        sprintf(buf, ", \"%03d:%03d:%03d\"", red, green, blue);
        db_append_string(&sql, buf);
    }
    db_append_string(&sql, " )");
    G_debug(3, "%s", db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
        G_fatal_error(_("Cannot insert new row: %s"), db_get_string(&sql));
    }
}
