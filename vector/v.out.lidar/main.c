/***********************************************************************
 *
 * MODULE:       v.out.lidar
 *
 * AUTHOR(S):    Vaclav Petras
 *
 * PURPOSE:      Export LiDAR LAS points
 *
 * COPYRIGHT:    (C) 2015 by Vaclav Petras and the GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
***********************************************************************/

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include <liblas/capi/liblas.h>

#define LAS_FIRST 1
#define LAS_MID 2
#define LAS_LAST 3

struct WriteContext
{
    LASWriterH las_writer;
    LASPointH las_point;
    LASColorH las_color;
    struct Colors *color_table;
    int layer;
    int return_layer;
    int class_layer;
    int rgb_layer;
    dbCatValArray *return_column_values;
    dbCatValArray *n_returns_column_values;
    dbCatValArray *class_column_values;
    dbCatValArray *grass_rgb_column_values;
    dbCatValArray *red_column_values;
    dbCatValArray *green_column_values;
    dbCatValArray *blue_column_values;
};


struct LidarColumnNames
{
    const char *return_n;
    const char *n_returns;
    const char *class_n;
    const char *grass_rgb;
    const char *red;
    const char *green;
    const char *blue;
};


/*! Open database and store driver and field info
 * 
 * Use close_database() when you are finished with queries
 */
static void open_database(struct Map_info *vector, int field,
                          dbDriver ** driver, struct field_info **f_info)
{
    struct field_info *f_info_tmp = Vect_get_field(vector, field);
    if (f_info_tmp == NULL) {
        /* not ideal message since we don't know the original name of
         * the field in case of OGR */
        G_fatal_error(_("Database connection not defined for layer <%d>"),
                      field);
    }

    dbDriver *driver_tmp =
        db_start_driver_open_database(f_info_tmp->driver,
                                      f_info_tmp->database);
    if (driver_tmp == NULL)
        G_fatal_error("Unable to open database <%s> by driver <%s>",
                      f_info_tmp->database, f_info_tmp->driver);
    db_set_error_handler_driver(driver_tmp);
    *f_info = f_info_tmp;
    *driver = driver_tmp;
}

static void close_database(dbDriver * driver)
{
    db_close_database_shutdown_driver(driver);
}

/*! Get values in a column
 *
 * Checks the type of the column; fails with fatal for non-numeric columns
 * and warns for floating point columns.
 *
 * Use db_CatValArray_free() and G_free() to deallocate the memory.
 *
 * \returns cat-value array with column values for each category
 */
static dbCatValArray *select_integers_from_database(dbDriver * driver,
                                                    struct field_info *f_info,
                                                    const char *column,
                                                    const char *where)
{
    G_debug(1, "select_integers_from_database: column=%s", column);
    dbCatValArray *column_values = G_malloc(sizeof(dbCatValArray));

    /* check if column exists */
    int ctype = db_column_Ctype(driver, f_info->table, column);

    if (ctype == -1)
        G_fatal_error(_("Column <%s> not found in table <%s>"),
                      column, f_info->table);
    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
        G_fatal_error(_("Only numeric column type is supported (column <%s> in table <%s>)"),
                      column, f_info->table);
    if (ctype == DB_C_TYPE_DOUBLE)
        G_warning(_("Double values will be converted to integers (column <%s> in table <%s>)"),
                      column, f_info->table);

    db_CatValArray_init(column_values);
    int nrec =
        db_select_CatValArray(driver, f_info->table, f_info->key, column,
                              where, column_values);

    G_debug(2, "db_select_CatValArray() nrec = %d", nrec);
    if (nrec < 0)
        G_fatal_error(_("Unable to select data from table"));
    return column_values;
}

/*! Get values in a column
 *
 * Checks the type of the column; fails with fatal for non-numeric columns
 * and warns for floating point columns.
 *
 * \returns cat-value array with column values for each category
 */
static dbCatValArray *select_strings_from_database(dbDriver * driver,
                                                   struct field_info *f_info,
                                                   const char *column,
                                                   const char *where)
{
    G_debug(1, "select_strings_from_database: column=%s", column);
    dbCatValArray *column_values = G_malloc(sizeof(dbCatValArray));

    /* check if column exists */
    int ctype = db_column_Ctype(driver, f_info->table, column);

    if (ctype == -1)
        G_fatal_error(_("Column <%s> not found in table <%s>"),
                      column, f_info->table);
    if (ctype != DB_C_TYPE_STRING)
        G_fatal_error(_("Only numeric column type is supported"));

    db_CatValArray_init(column_values);
    int nrec =
        db_select_CatValArray(driver, f_info->table, f_info->key, column,
                              where, column_values);

    G_debug(2, "db_select_CatValArray() nrec = %d", nrec);
    if (nrec < 0)
        G_fatal_error(_("Unable to select data from table"));
    return column_values;
}

/*! Get integer value in a column for a category
 *
 * Floating point numbers are casted to integers.
 * If the column is not numerical, fatal error is issued.
 *
 * \returns The value of the column as an integer
 */
static int get_integer_column_value(dbCatValArray * column_values, int cat)
{
    int val;
    dbCatVal *catval;

    if (db_CatValArray_get_value(column_values, cat, &catval) != DB_OK) {
        G_fatal_error(_("No record for cat = %d"), cat);
    }
    if (catval->isNull) {
        G_fatal_error(_("NULL value for cat = %d"), cat);
    }

    if (column_values->ctype == DB_C_TYPE_INT) {
        val = catval->val.i;
    }
    else if (column_values->ctype == DB_C_TYPE_DOUBLE) {
        val = catval->val.d;
    } else {
        G_fatal_error(_("Column type is not numeric (type = %d, cat = %d"),
            column_values->ctype, cat);
    }
    return val;
}

/*! Get RGB in a column for a category as three integers
 *
 * Expects the column to be a string.
 */
static void get_color_column_value(dbCatValArray * cvarr, int cat,
                                   int *red, int *green, int *blue)
{
    char colorstring[12];       /* RRR:GGG:BBB */
    dbCatVal *value = NULL;

    /* read RGB colors from db for current area # */
    if (cvarr && db_CatValArray_get_value(cvarr, cat, &value) == DB_OK) {
        sprintf(colorstring, "%s", db_get_string(value->val.s));
        if (*colorstring != '\0') {
            G_debug(5, "element: colorstring: %s", colorstring);
            if (G_str_to_color(colorstring, red, green, blue) == 1) {
                G_debug(5, "element: cat %d r:%d g:%d b:%d",
                        cat, *red, *green, *blue);
                /* TODO: handle return code 2 for none? */
            }
            else {
                G_debug(5, "Invalid color definition '%s' ignored",
                        colorstring);
            }
        }
        else {
            G_debug(5, "Invalid color definition '%s' ignored", colorstring);
        }
    }
}

static void load_columns(struct WriteContext *write_context,
                         dbDriver * db_driver, struct field_info *f_info,
                         struct LidarColumnNames *columns, const char *where)
{
    if (columns->return_n) {
        write_context->return_column_values =
            select_integers_from_database(db_driver, f_info,
                                          columns->return_n, where);
    }
    if (columns->n_returns) {
        write_context->n_returns_column_values =
            select_integers_from_database(db_driver, f_info,
                                          columns->n_returns, where);
    }
    if (columns->class_n) {
        write_context->class_column_values =
            select_integers_from_database(db_driver, f_info,
                                          columns->class_n, where);
    }
    if (columns->grass_rgb) {
        write_context->grass_rgb_column_values =
            select_strings_from_database(db_driver, f_info,
                                         columns->grass_rgb, where);
    }
    if (columns->red) {
        write_context->red_column_values =
            select_integers_from_database(db_driver, f_info,
                                          columns->red, where);
    }
    if (columns->green) {
        write_context->green_column_values =
            select_integers_from_database(db_driver, f_info,
                                          columns->green, where);
    }
    if (columns->blue) {
        write_context->blue_column_values =
            select_integers_from_database(db_driver, f_info,
                                          columns->blue, where);
    }
}

/*! Deallocate the memory for cat-value arrays */
static void free_columns(struct WriteContext *write_context)
{
    if (write_context->return_column_values) {
        db_CatValArray_free(write_context->return_column_values);
        G_free(write_context->return_column_values);
    }
    if (write_context->n_returns_column_values) {
        db_CatValArray_free(write_context->n_returns_column_values);
        G_free(write_context->n_returns_column_values);
    }
    if (write_context->class_column_values) {
        db_CatValArray_free(write_context->class_column_values);
        G_free(write_context->class_column_values);
    }
    if (write_context->grass_rgb_column_values) {
        db_CatValArray_free(write_context->grass_rgb_column_values);
        G_free(write_context->grass_rgb_column_values);
    }
    if (write_context->red_column_values) {
        db_CatValArray_free(write_context->red_column_values);
        G_free(write_context->red_column_values);
    }
    if (write_context->green_column_values) {
        db_CatValArray_free(write_context->green_column_values);
        G_free(write_context->green_column_values);
    }
    if (write_context->blue_column_values) {
        db_CatValArray_free(write_context->blue_column_values);
        G_free(write_context->blue_column_values);
    }
}

/*! Set point attributes from the attribute table
 *
 * All tables are taken from the context structure. The point and color
 * are also taken from there.
 */
static void set_point_attributes_from_table(struct WriteContext *context,
                                            int cat)
{
    LASPointH las_point = context->las_point;

    if (context->return_column_values) {
        int return_n =
            get_integer_column_value(context->return_column_values, cat);
        LASPoint_SetReturnNumber(las_point, return_n);
    }
    if (context->n_returns_column_values) {
        int val =
            get_integer_column_value(context->n_returns_column_values, cat);
        LASPoint_SetNumberOfReturns(las_point, val);
    }
    if (context->class_column_values) {
        int val = get_integer_column_value(context->class_column_values, cat);

        LASPoint_SetClassification(las_point, val);
    }
    if (context->grass_rgb_column_values || context->red_column_values ||
        context->green_column_values || context->blue_column_values) {
        LASColorH las_color = context->las_color;

        if (context->grass_rgb_column_values) {
            int red, green, blue;

            get_color_column_value(context->grass_rgb_column_values, cat,
                                   &red, &green, &blue);
            LASColor_SetRed(las_color, red);
            LASColor_SetGreen(las_color, green);
            LASColor_SetBlue(las_color, blue);
        }
        if (context->red_column_values) {
            int val =
                get_integer_column_value(context->red_column_values, cat);
            LASColor_SetRed(las_color, val);
        }
        if (context->green_column_values) {
            int val =
                get_integer_column_value(context->green_column_values, cat);
            LASColor_SetGreen(las_color, val);
        }
        if (context->blue_column_values) {
            int val =
                get_integer_column_value(context->blue_column_values, cat);
            LASColor_SetBlue(las_color, val);
        }
        LASPoint_SetColor(las_point, las_color);
    }
}

static void write_point(struct WriteContext *context, int cat, double x,
                        double y, double z, struct line_cats *cats)
{
    LASPointH las_point = context->las_point;

    LASPoint_SetX(las_point, x);
    LASPoint_SetY(las_point, y);
    LASPoint_SetZ(las_point, z);

    /* only call when we actually using the attributes */
    if (context->layer > 0)
        set_point_attributes_from_table(context, cat);
    /* after this point cat is used as a short term variable
     * to store category to retrieve attributes */
    
    /* read color table */
    if (context->color_table) {
        int red, green, blue;
        LASColorH las_color = context->las_color;
        if (Rast_get_c_color(&cat, &red, &green, &blue, context->color_table) == 1) {
            LASColor_SetRed(las_color, red);
            LASColor_SetGreen(las_color, green);
            LASColor_SetBlue(las_color, blue);
            LASPoint_SetColor(las_point, las_color);
        }
        /* TODO: what else, fail, skip or put some defaults? */
    }

    if (context->return_layer) {
        if (!Vect_cat_get(cats, context->return_layer, &cat))
            return;             /* TODO: is this an error? */
        if (cat == LAS_FIRST) {
            LASPoint_SetReturnNumber(las_point, LAS_FIRST);
            LASPoint_SetNumberOfReturns(las_point, LAS_FIRST);
        } else if (cat == LAS_LAST) {
            LASPoint_SetReturnNumber(las_point, LAS_LAST);
            LASPoint_SetNumberOfReturns(las_point, LAS_LAST);
        } else {
            LASPoint_SetReturnNumber(las_point, LAS_MID);
            LASPoint_SetNumberOfReturns(las_point, LAS_LAST);
        }
    }
    if (context->class_layer) {
        if (!Vect_cat_get(cats, context->class_layer, &cat))
            return;             /* TODO: is this an error? */
        LASPoint_SetClassification(las_point, cat);
    }
    if (context->rgb_layer) {
        LASColorH las_color = context->las_color;

        /* TODO: defaults for the color are what? */
        /* TODO: check the range for RGB? */
        if (context->rgb_layer) {
            if (!Vect_cat_get(cats, context->rgb_layer, &cat))
                return;         /* TODO: is this an error? */
            /* cat 0 is not valid, so we are adding 1 when storing
             * now we need to subtract 1 */
            int rgb = cat - 1;
            int red = (rgb >> 16) & 0xFF;
            int green = (rgb >> 8) & 0xFF;
            int blue = rgb & 0xFF;

            LASColor_SetRed(las_color, red);
            LASColor_SetGreen(las_color, green);
            LASColor_SetBlue(las_color, blue);
        }                       /* TODO: else all the others? */
        LASPoint_SetColor(las_point, las_color);
    }

    LASError error = LASWriter_WritePoint(context->las_writer, las_point);

    if (error)
        G_fatal_error("Failure when writing a point");
}


/* TODO: these have overlap with vector lib, really needed? */
static int point_in_region_2d(struct Cell_head *region, double x, double y)
{
    if (x > region->east || x < region->west || y < region->south ||
        y > region->north)
        return FALSE;
    return TRUE;
}


int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *map_opt, *foutput_opt;
    struct Option *field_opt, *cats_opt;
    struct Option *id_layer_opt;
    struct Option *return_layer_opt;
    struct Option *class_layer_opt;
    struct Option *rgb_layer_opt;
    struct Option *return_column_opt, *n_returns_column_opt;
    struct Option *class_column_opt;
    struct Option *grass_rgb_column_opt;
    struct Option *red_column_opt, *green_column_opt, *blue_column_opt;
    struct Option *where_opt;
    struct Option *las_xyscale_opt, *las_zscale_opt;
    struct Flag *region_flag, *no_color_table_flag;
    struct Map_info vinput;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    G_add_keyword(_("LIDAR"));
    G_add_keyword(_("points"));
    module->label = _("Exports vector points as LAS point cloud");
    module->description = _("Converts LAS LiDAR point clouds to a GRASS"
                            " vector map with libLAS");

    map_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    field_opt->required = NO;

    foutput_opt = G_define_standard_option(G_OPT_F_OUTPUT);

    cats_opt = G_define_standard_option(G_OPT_V_CATS);
    cats_opt->guisection = _("Selection");

    /* TODO: supported only when attributes are actually used */
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

    id_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    id_layer_opt->key = "id_layer";
    id_layer_opt->label =
        _("Layer number to store generated point ID as category");
    id_layer_opt->answer = NULL;
    id_layer_opt->guisection = _("Categories");

    return_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    return_layer_opt->key = "return_layer";
    return_layer_opt->label =
        _("Layer number to store return number as category");
    return_layer_opt->answer = NULL;
    return_layer_opt->guisection = _("Categories");

    class_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    class_layer_opt->key = "class_layer";
    class_layer_opt->label =
        _("Layer number to store class number as category");
    class_layer_opt->answer = NULL;
    class_layer_opt->guisection = _("Categories");

    rgb_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    rgb_layer_opt->key = "rgb_layer";
    rgb_layer_opt->label =
        _("Layer number where RGB color is stored as category");
    rgb_layer_opt->answer = NULL;
    rgb_layer_opt->guisection = _("Categories");

    /* TODO: probably replace the option by standardized/expected column names */

    return_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    return_column_opt->key = "return_column";
    return_column_opt->label = _("Column with return number");
    return_column_opt->required = NO;
    return_column_opt->guisection = _("Columns");

    n_returns_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    n_returns_column_opt->key = "n_returns_column";
    n_returns_column_opt->label = _("Column with return number");
    n_returns_column_opt->required = NO;
    n_returns_column_opt->guisection = _("Columns");

    class_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    class_column_opt->key = "class_column";
    class_column_opt->label = _("Column with return number");
    class_column_opt->required = NO;
    class_column_opt->guisection = _("Columns");

    grass_rgb_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    grass_rgb_column_opt->key = "rgb_column";
    grass_rgb_column_opt->label = _("RGB color definition column");
    grass_rgb_column_opt->description = _("Color definition in R:G:B form");
    grass_rgb_column_opt->required = NO;
    grass_rgb_column_opt->guisection = _("Columns");

    red_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    red_column_opt->key = "red_column";
    red_column_opt->label = _("Column with red color");
    red_column_opt->required = NO;
    red_column_opt->guisection = _("Columns");

    green_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    green_column_opt->key = "green_column";
    green_column_opt->label = _("Column with green color");
    green_column_opt->required = NO;
    green_column_opt->guisection = _("Columns");

    blue_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    blue_column_opt->key = "blue_column";
    blue_column_opt->label = _("Column with blue color");
    blue_column_opt->required = NO;
    blue_column_opt->guisection = _("Columns");

    las_xyscale_opt = G_define_option();
    las_xyscale_opt->key = "las_xyscale";
    las_xyscale_opt->type = TYPE_DOUBLE;
    las_xyscale_opt->required = YES;
    las_xyscale_opt->answer = "0.01";
    las_xyscale_opt->label = _("Internal scale to apply to X and Y values");
    las_xyscale_opt->description = _("This scale does not change"
        " the values itself but only how precisely they are stored,"
        " for example 0.01 will preserve two decimal places");

    las_zscale_opt = G_define_option();
    las_zscale_opt->key = "las_zscale";
    las_zscale_opt->type = TYPE_DOUBLE;
    las_zscale_opt->required = YES;
    las_zscale_opt->answer = "0.01";
    las_zscale_opt->label = _("Internal scale to apply to z values");
    las_zscale_opt->description = _("This scale does not change"
        " the values itself but only how precisely they are stored,"
        " for example 0.01 will preserve two decimal places");

    region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->guisection = _("Selection");
    region_flag->description = _("Limit export to the current region");

    no_color_table_flag = G_define_flag();
    no_color_table_flag->key = 'w';
    no_color_table_flag->label = _("Ignore color table");
    no_color_table_flag->description =
        _("Ignore color table even when set and not other options are present");


    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* TODO: layer required > 0 with columns */

    /* TODO: do some check */
    /*Vect_check_input_output_name(map_opt->answer, voutput_opt->answer,
       G_FATAL_EXIT);
     */

    if (Vect_open_old2(&vinput, map_opt->answer, "", field_opt->answer) < 0)
        G_fatal_error(_("Unable to open vector map <%s>"), map_opt->answer);
    int layer = Vect_get_field_number(&vinput, field_opt->answer);

    struct cat_list *allowed_cats = NULL;

    if (layer > 0)
        allowed_cats = Vect_cats_set_constraint(&vinput, layer, NULL,
                                                cats_opt->answer);

    struct line_pnts *line = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();

    struct Cell_head comp_region;

    G_get_window(&comp_region);

    struct WriteContext write_context;

    write_context.return_layer = 0;
    write_context.class_layer = 0;
    write_context.rgb_layer = 0;
    if (return_layer_opt->answer)
        write_context.return_layer = atoi(return_layer_opt->answer);
    if (class_layer_opt->answer)
        write_context.class_layer = atoi(class_layer_opt->answer);
    if (rgb_layer_opt->answer)
        write_context.rgb_layer = atoi(rgb_layer_opt->answer);

    /* get GRASS loc proj info */
    struct Key_Value *proj_info;
    struct Key_Value *proj_units;

    /* TODO: should we test for PROJECTION_XY? */
    proj_info = G_get_projinfo();
    proj_units = G_get_projunits();
    char *current_wkt = GPJ_grass_to_wkt(proj_info, proj_units, FALSE, FALSE);

    G_free_key_value(proj_info);
    G_free_key_value(proj_units);

    /* TODO: ignoring errors */
    LASWriterH las_writer;
    LASHeaderH las_header = LASHeader_Create();
    LASSRSH las_srs = LASSRS_Create();

    LASSRS_SetWKT(las_srs, current_wkt);
    LASHeader_SetSRS(las_header, las_srs);
    LASHeader_SetScale(las_header, atof(las_xyscale_opt->answer),
        atof(las_xyscale_opt->answer), atof(las_zscale_opt->answer));
    /* TODO: support append mode */
    int write_mode = 1;

    las_writer = LASWriter_Create(foutput_opt->answer,
                                  las_header, write_mode);
    write_context.las_writer = las_writer;

    /* to avoid allocation for each point we are writing */
    write_context.las_point = LASPoint_Create();
    LASPoint_SetHeader(write_context.las_point, las_header);
    write_context.las_color = LASColor_Create();

    write_context.layer = layer;
    write_context.return_column_values = 0;
    write_context.n_returns_column_values = 0;
    write_context.class_column_values = 0;
    write_context.grass_rgb_column_values = 0;
    write_context.red_column_values = 0;
    write_context.green_column_values = 0;
    write_context.blue_column_values = 0;
    /* TODO: limit select by the cat values */
    /* TODO: limit select by 2D/3D region and zrange, possible? */

    int use_color_attributes = FALSE;

    if (return_column_opt->answer || n_returns_column_opt->answer
        || class_column_opt->answer || grass_rgb_column_opt->answer
        || red_column_opt->answer || green_column_opt->answer
        || blue_column_opt->answer) {
        dbDriver *db_driver;
        struct field_info *f_info;

        struct LidarColumnNames column_names;

        column_names.return_n = return_column_opt->answer;
        column_names.n_returns = n_returns_column_opt->answer;
        column_names.class_n = class_column_opt->answer;
        column_names.grass_rgb = grass_rgb_column_opt->answer;
        column_names.red = red_column_opt->answer;
        column_names.green = green_column_opt->answer;
        column_names.blue = blue_column_opt->answer;

        open_database(&vinput, layer, &db_driver, &f_info);
        load_columns(&write_context, db_driver, f_info, &column_names,
                     where_opt->answer);
        close_database(db_driver);
        
        if ( grass_rgb_column_opt->answer || red_column_opt->answer || green_column_opt->answer || blue_column_opt->answer)
            use_color_attributes = TRUE;
    }

    struct Colors color_table;
    write_context.color_table = 0;
    if (!use_color_attributes && !no_color_table_flag->answer
        && !(write_context.rgb_layer)) {
        int has_colors = Vect_read_colors(Vect_get_name(&vinput),
                                          Vect_get_mapset(&vinput),
                                          &color_table);
        if (has_colors)
            write_context.color_table = &color_table;
    }

    /* some constraints can be set on the map */
    Vect_set_constraint_type(&vinput, GV_POINT);
    /* noop for layer=-1 and non-native format, skips lines without cat */
    Vect_set_constraint_field(&vinput, layer);
    /* TODO: replace region checks by Vect_set_constraint_region? */

    int ltype;
    int cat;

    while (TRUE) {
        ltype = Vect_read_next_line(&vinput, line, cats);
        if (ltype == -1)
            G_fatal_error(_("Unable to read vector map"));
        if (ltype == -2)
            break;              /* end of the map */

        double x, y, z;

        Vect_line_get_point(line, 0, &x, &y, &z);

        /* selections/filters */
        /* TODO: use region only when actually needed */
        if (region_flag->answer && !point_in_region_2d(&comp_region, x, y))
            continue;
        if (layer > 0 && allowed_cats &&
            !Vect_cats_in_constraint(cats, layer, allowed_cats))
            continue;

        /* TODO: test: skip points without category, unless layer=-1 */
        /* Use cases:
         * - all points have category (correct)
         * - no categories for any point (correct, layer=-1 required)
         * - some points miss category (not handled)
         * Here we assume that there is only one set of attributes for one point.
         * If no layer available, cat contains junk and shouldn't be used.
	 * 
	 * TODO: done
         */
	cat = -1;
        if (layer > 0) {
	    if (allowed_cats) {
		int i;

		for (i = 0; i < cats->n_cats; i++) {
		    if (cats->field[i] == layer &&
			Vect_cat_in_cat_list(cats->cat[i], allowed_cats)) {
			cat = cats->cat[i];
			break;
		    }
		}
	    }
	    else {
		Vect_cat_get(cats, layer, &cat);
	    }
	}

        write_point(&write_context, cat, x, y, z, cats);
    }

    /* partially unnecessary as deallocated by the system */
    Vect_destroy_line_struct(line);
    Vect_destroy_cats_struct(cats);
    Vect_close(&vinput);

    free_columns(&write_context);

    LASPoint_Destroy(write_context.las_point);
    LASColor_Destroy(write_context.las_color);
    LASWriter_Destroy(write_context.las_writer);
    LASSRS_Destroy(las_srs);

    return EXIT_SUCCESS;
}
