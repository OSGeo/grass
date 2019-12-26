
/****************************************************************
 *
 * MODULE:     v.profile
 *
 * AUTHOR(S):  Maris Nartiss <maris.gis@gmail.com>
 *             with hints from v.out.ascii, v.buffer, v.what 
 *             and other GRASS GIS modules
 *
 * PURPOSE:    Output vector point/line values along sampling line
 *
 * COPYRIGHT:  (C) 2008, 2017 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 * TODO:       Attach a centroid to buffer with tolerance value;
 *             Ability to have "interrupted" profiling line - with holes,
 *                 that are not counted into whole profile length;
 *             Implement area sampling by printing out boundary crossing place?
 *             String quoting is unoptimal:
 *                 * What if delimiter equals string quote character?
 *                 * Quotes within strings are not escaped
 *                 * What if user wants to have different quote symbol or no quotes at all?
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

#if HAVE_GEOS
#include <float.h>

/* A copy of ring2pts from v.buffer geos.c 
 * It is a terrible approach to copy/pasta functions around,
 * but all this GEOS stuff is just a temporary solution before native
 * buffers are fixed. (Will happen at some point after next ten years
 * or so as there's nothing more permanent than a temporary solution.)
 * 2017-11-19
 */
static int ring2pts(const GEOSGeometry *geom, struct line_pnts *Points)
{
    int i, ncoords;
    double x, y, z;
    const GEOSCoordSequence *seq = NULL;

    G_debug(3, "ring2pts()");

    Vect_reset_line(Points);
    if (!geom) {
	G_warning(_("Invalid GEOS geometry!"));
	return 0;
    }
    z = 0.0;
    ncoords = GEOSGetNumCoordinates(geom);
    if (!ncoords) {
	G_warning(_("No coordinates in GEOS geometry (can be ok for negative distance)!"));
	return 0;
    }
    seq = GEOSGeom_getCoordSeq(geom);
    for (i = 0; i < ncoords; i++) {
	GEOSCoordSeq_getX(seq, i, &x);
	GEOSCoordSeq_getY(seq, i, &y);
	if (x != x || x > DBL_MAX || x < -DBL_MAX)
	    G_fatal_error(_("Invalid x coordinate %f"), x);
	if (y != y || y > DBL_MAX || y < -DBL_MAX)
	    G_fatal_error(_("Invalid y coordinate %f"), y);
	Vect_append_point(Points, x, y, z);
    }

    return 1;
}

/* Helper for converting multipoligons to GRASS poligons */
static void add_poly(const GEOSGeometry *OGeom, struct line_pnts *Buffer) {
    const GEOSGeometry *geom2;
    static struct line_pnts *gPoints;
    int i, nrings;
    
    gPoints = Vect_new_line_struct();
    
    geom2 = GEOSGetExteriorRing(OGeom);
    if (!ring2pts(geom2, gPoints)) {
        G_fatal_error(_("Corrupt GEOS geometry"));
    }
    
    Vect_append_points(Buffer, gPoints, GV_FORWARD);
    Vect_reset_line(gPoints);
    
    nrings = GEOSGetNumInteriorRings(OGeom);
    
    for (i = 0; i < nrings; i++) {
        geom2 = GEOSGetInteriorRingN(OGeom, i);
        if (!ring2pts(geom2, gPoints)) {
            G_fatal_error(_("Corrupt GEOS geometry"));
        }
        Vect_append_points(Buffer, gPoints, GV_FORWARD);
        Vect_reset_line(gPoints);
    }
}
#endif

static int compdist(const void *, const void *);

Result *resultset;

int main(int argc, char *argv[])
{
    struct Map_info In, Out, Pro;
    struct line_pnts *Points, *Profil, *Buffer, *Ipoints;
    struct line_cats *Cats;
    struct ilist *Catlist;
    FILE *ascii;

    int i, dp, type, otype, id, nrows, ncats, col, more, open3d,
        layer, pro_layer, *cats, c, field_index, cat;
    size_t j, rescount;

    /* GCC */
    int ncols = 0;
    double xval, yval, bufsize;
    const char *mapset, *pro_mapset;
    char sql[200], *fs;

    struct GModule *module;
    struct Option *old_map, *new_map, *coords_opt, *buffer_opt, *delim_opt,
        *dp_opt, *layer_opt, *where_opt, *inline_map, *inline_where,
        *inline_layer, *type_opt, *file_opt;
    struct Flag *no_column_flag, *no_z_flag;

    struct field_info *Fi, *Fpro;
    dbDriver *driver;
    dbHandle handle;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbString table_name, dbsql, valstr;

    /* initialize GIS environment */
    G_gisinit(argv[0]);

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("profile"));
    G_add_keyword(_("transect"));
    module->description = _("Vector map profiling tool");

    /* Map to be profiled */
    old_map = G_define_standard_option(G_OPT_V_INPUT);
    old_map->required = YES;

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,line";
    type_opt->answer = "point,line";
    type_opt->guisection = _("Selection");

    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

    layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    layer_opt->answer = "1";
    layer_opt->description = _("Use features only from specified layer");
    layer_opt->guisection = _("Selection");

    /* Text output details */
    file_opt = G_define_option();
    file_opt->key = "output";
    file_opt->type = TYPE_STRING;
    file_opt->required = NO;
    file_opt->multiple = NO;
    file_opt->gisprompt = "new_file,file,output";
    file_opt->answer = "-";
    file_opt->description = _("Path to output text file or - for stdout");
    file_opt->guisection = _("Format");

    delim_opt = G_define_standard_option(G_OPT_F_SEP);
    delim_opt->guisection = _("Format");

    dp_opt = G_define_option();
    dp_opt->key = "dp";
    dp_opt->type = TYPE_INTEGER;
    dp_opt->required = NO;
    dp_opt->options = "0-32";
    dp_opt->answer = "2";
    dp_opt->description = _("Number of significant digits");
    dp_opt->guisection = _("Format");

    /* Profiling tolerance */
    buffer_opt = G_define_option();
    buffer_opt->key = "buffer";
    buffer_opt->type = TYPE_DOUBLE;
    buffer_opt->required = YES;
    buffer_opt->answer = "10";
    buffer_opt->label = _("Buffer (tolerance) for points in map units");
    buffer_opt->description = _("How far points can be from sampling line");

    /* Storing tolerance area (buffer) is useful to examine which points match */
    new_map = G_define_option();
    new_map->key = "map_output";
    new_map->type = TYPE_STRING;
    new_map->key_desc = "name";
    new_map->required = NO;
    new_map->multiple = NO;
    new_map->gisprompt = "new,vector,vector";
    new_map->label = _("Name for profile line and buffer output map");
    new_map->description =
        _("Profile line and buffer around it will be written");
    new_map->guisection = _("Output");

    no_column_flag = G_define_flag();
    no_column_flag->key = 'c';
    no_column_flag->description = _("Do not print column names");
    no_column_flag->guisection = _("Output");

    no_z_flag = G_define_flag();
    no_z_flag->key = 'z';
    no_z_flag->label = _("Do not print 3D vector data (z values)");
    no_z_flag->description = _("Only affects 3D vectors");
    no_z_flag->guisection = _("Output");

    /* Two ways of defining profiling line:
     * - by list of coordinates */
    coords_opt = G_define_standard_option(G_OPT_M_COORDS);
    coords_opt->multiple = YES;
    coords_opt->label = _("Coordinates for profiling line nodes");
    coords_opt->description = _("Specify profiling line vertexes and nodes");
    coords_opt->guisection = _("Profiling line");

    /* - or profiling line can be taken form other vector map */
    inline_map = G_define_option();
    inline_map->key = "profile_map";
    inline_map->type = TYPE_STRING;
    inline_map->key_desc = "name";
    inline_map->required = NO;
    inline_map->multiple = NO;
    inline_map->gisprompt = "old,vector,vector";
    inline_map->label = _("Profiling line map");
    inline_map->description = _("Vector map containing profiling line");
    inline_map->guisection = _("Profiling line");

    inline_where = G_define_option();
    inline_where->key = "profile_where";
    inline_where->type = TYPE_STRING;
    inline_where->key_desc = "sql_query";
    inline_where->required = NO;
    inline_where->multiple = NO;
    inline_where->label = _("WHERE conditions for input profile line map");
    inline_where->description =
        _("Use to select only one line from profiling line map");
    inline_where->guisection = _("Profiling line");

    inline_layer = G_define_option();
    inline_layer->key = "profile_layer";
    inline_layer->type = TYPE_INTEGER;
    inline_layer->required = NO;
    inline_layer->answer = "1";
    inline_layer->description = _("Profiling line map layer");
    inline_layer->guisection = _("Profiling line");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);
    
#if HAVE_GEOS
#if (GEOS_VERSION_MAJOR < 3 || (GEOS_VERSION_MAJOR >= 3 && GEOS_VERSION_MINOR < 3))
    G_fatal_error("This module requires GEOS >= 3.3");
#endif
    initGEOS(G_message, G_fatal_error);
#else
    G_fatal_error("GRASS native buffering functions are known to return incorrect results.\n"
        "Till those errors are fixed, this module requires GRASS to be compiled with GEOS support.");
#endif

    /* Start with simple input validation and then move to more complex ones */
    otype = Vect_option_to_types(type_opt);
    layer = atoi(layer_opt->answer);
    pro_layer = atoi(inline_layer->answer);
    if (layer < 1 || pro_layer < 1)
        G_fatal_error(_("Layer 0 not supported"));

    /* The precision of the output */
    if (dp_opt->answer) {
        if (sscanf(dp_opt->answer, "%d", &dp) != 1)
            G_fatal_error(_("Failed to interpreter 'dp' parameter as an integer"));
    }

    /* Get buffer size */
    bufsize = fabs(atof(buffer_opt->answer));
    if (!(bufsize > 0))
        G_fatal_error(_("Tolerance value must be greater than 0"));

    /* If new map name is provided, it has to be useable */
    if (new_map->answer != NULL)
        if (Vect_legal_filename(new_map->answer) < 1)
            G_fatal_error(_("<%s> is not a valid vector map name"),
                          new_map->answer);

    /* inline_where has no use if inline_map has been not provided */
    if (inline_where->answer != NULL && inline_map->answer == NULL)
        G_fatal_error(_("No input profile map name provided, but WHERE conditions for it have been set"));

    /* Currently only one profile input method is supported */
    if (inline_map->answer != NULL && coords_opt->answer != NULL)
        G_fatal_error(_("Profile input coordinates and vector map are provided. "
                       "Please provide only one of them"));

    if (inline_map->answer == NULL && coords_opt->answer == NULL)
        G_fatal_error(_("No profile input coordinates nor vector map are provided. "
                       "Please provide one of them"));

    /* Where to put module output */
    if (file_opt->answer) {
        if (strcmp(file_opt->answer, "-") == 0) {
            ascii = stdout;
        }
        else {
            ascii = fopen(file_opt->answer, "w");
        }

        if (ascii == NULL) {
            G_fatal_error(_("Unable to open file <%s>"), file_opt->answer);
        }
    }
    else {
        ascii = stdout;
    }

    /* Create and initialize struct's where to store points/lines and categories */
    Points = Vect_new_line_struct();
    Profil = Vect_new_line_struct();
    Buffer = Vect_new_line_struct();
    Ipoints = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* Construct profile line from user supplied points */
    if (inline_map->answer == NULL) {
        for (i = 0; coords_opt->answers[i] != NULL; i += 2) {
            xval = atof(coords_opt->answers[i]);
            yval = atof(coords_opt->answers[i + 1]);
            Vect_append_point(Profil, xval, yval, 0);
        }

        /* Line is built from two coordinates */
        if (i == 2)
            G_fatal_error(_("At least profile start and end coordinates are required!"));
    }
    else {
        /* Check provided profile map name validity */
        if ((pro_mapset = G_find_vector2(inline_map->answer, "")) == NULL)
            G_fatal_error(_("Vector map <%s> not found"), inline_map->answer);
    }

    if ((mapset = G_find_vector2(old_map->answer, "")) == NULL)
        G_fatal_error(_("Vector map <%s> not found"), old_map->answer);

    if (Vect_set_open_level(2))
        G_fatal_error(_("Unable to set predetermined vector open level"));

    /* Open existing vector map for reading */
    if (Vect_open_old(&In, old_map->answer, mapset) < 1)
        G_fatal_error(_("Unable to open vector map <%s>"), old_map->answer);

    /* Process input as 3D only if it's required */
    if (!no_z_flag->answer && Vect_is_3d(&In))
        open3d = WITH_Z;
    else
        open3d = WITHOUT_Z;

    /* the field separator */
    fs = G_option_to_separator(delim_opt);

    /* Let's get vector layers db connections information */
    Fi = Vect_get_field(&In, layer);
    if (!Fi && where_opt->answer != NULL) {
        Vect_close(&In);
        G_fatal_error(_("No database connection defined for map <%s> layer %d, "
                       "but WHERE condition is provided"), old_map->answer,
                      layer);
    }

    /* Get profile line from an existing vector map */
    if (inline_map->answer != NULL) {
        /* If we get here, pro_mapset is inicialized */
        if (1 > Vect_open_old(&Pro, inline_map->answer, pro_mapset))
            G_fatal_error(_("Unable to open vector map <%s>"),
                          inline_map->answer);
        if (inline_where->answer != NULL) {
            Fpro = Vect_get_field(&Pro, pro_layer);
            if (!Fpro) {
                Vect_close(&In);
                Vect_close(&Pro);
                G_fatal_error(_("No database connection defined for map <%s> layer %d, "
                               "but WHERE condition is provided"),
                              inline_map->answer, pro_layer);
            }
            /* Prepeare strings for use in db_* calls */
            db_init_string(&dbsql);
            db_init_string(&valstr);
            db_init_string(&table_name);
            db_init_handle(&handle);
            G_debug(1,
                    "Field number:%d; Name:<%s>; Driver:<%s>; Database:<%s>; Table:<%s>; Key:<%s>",
                    Fpro->number, Fpro->name, Fpro->driver, Fpro->database,
                    Fpro->table, Fpro->key);

            /* Prepearing database for use */
            driver = db_start_driver(Fpro->driver);
            if (driver == NULL) {
                Vect_close(&In);
                Vect_close(&Pro);
                G_fatal_error(_("Unable to start driver <%s>"), Fpro->driver);
            }
            db_set_handle(&handle, Fpro->database, NULL);
            if (db_open_database(driver, &handle) != DB_OK) {
                Vect_close(&In);
                Vect_close(&Pro);
                G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                              Fpro->database, Fpro->driver);
            }
            db_set_string(&table_name, Fpro->table);
            if (db_describe_table(driver, &table_name, &table) != DB_OK) {
                Vect_close(&In);
                Vect_close(&Pro);
                G_fatal_error(_("Unable to describe table <%s>"),
                              Fpro->table);
            }
            ncols = db_get_table_number_of_columns(table);

            ncats =
                db_select_int(driver, Fpro->table, Fpro->key,
                              inline_where->answer, &cats);
            if (ncats < 1) {
                Vect_close(&In);
                Vect_close(&Pro);
                G_fatal_error(_("No features match Your query"));
            }
            if (ncats > 1) {
                Vect_close(&In);
                Vect_close(&Pro);
                G_fatal_error(_("Your query matches more than one record in input profiling map. "
                               "Currently it's not supported. Enhance WHERE conditions to get only one line."));
            }
            if (!(Catlist = Vect_new_list())) {
                Vect_close(&In);
                Vect_close(&Pro);
                G_fatal_error(_("Error while initialising line list"));
            }
            /* Get all features matching specified CAT value */
            Vect_cidx_find_all(&Pro, pro_layer, GV_LINE, cats[0], Catlist);
            if (Catlist->n_values > 1) {
                Vect_close(&In);
                Vect_close(&Pro);
                G_fatal_error(_("Your query matches more than one record in input profiling map. "
                               "Currently it's not supported. Enhance WHERE conditions to get only one line."));
            }
            if (Vect_read_line(&Pro, Profil, NULL, Catlist->value[0]) !=
                GV_LINE) {
                G_fatal_error(_("Error while reading vector feature from profile line map"));
            }
        }
        else {
            /* WHERE not provided -> assuming profiling line map contains only one line */
            c = 0;
            while ((type = Vect_read_next_line(&Pro, Profil, NULL)) > 0) {
                if (type & GV_LINE)
                    c++;
            }
            /* Currently we support only one SINGLE input line */
            if (c > 1) {
                Vect_close(&In);
                Vect_close(&Pro);
                G_fatal_error(_("Your input profile map contains more than one line. "
                               "Currently it's not supported. Provide WHERE conditions to get only one line."));
            }
        }
    }

    /* Create a buffer around profile line for point sampling 
       Tolerance is calculated in such way that buffer will have flat end and no cap. */
    /* Native buffering is known to fail.
    Vect_line_buffer(Profil, bufsize, 1 - (bufsize * cos((2 * M_PI) / 2)),
                     Buffer);
    */
#ifdef HAVE_GEOS
    /* Code lifted from v.buffer geos.c (with modifications) */
    GEOSGeometry *IGeom;
    GEOSGeometry *OGeom = NULL;
    const GEOSGeometry *geom2 = NULL;
    
    IGeom = Vect_line_to_geos(Profil, GV_LINE, 0);
    if (!IGeom) {
        G_fatal_error(_("Failed to convert GRASS line to GEOS line"));
    }
    
    GEOSBufferParams* geos_params = GEOSBufferParams_create();
    GEOSBufferParams_setEndCapStyle(geos_params, GEOSBUF_CAP_FLAT);
    OGeom = GEOSBufferWithParams(IGeom, geos_params, bufsize);
    GEOSBufferParams_destroy(geos_params);
    if (!OGeom) {
        G_fatal_error(_("Buffering failed"));
    }
    
    if (GEOSGeomTypeId(OGeom) == GEOS_MULTIPOLYGON) {
        int ngeoms = GEOSGetNumGeometries(OGeom);
        for (i = 0; i < ngeoms; i++) {
            geom2 = GEOSGetGeometryN(OGeom, i);
            add_poly(geom2, Buffer);
        }
    }
    else {
        add_poly(OGeom, Buffer);
    }
    
    if (IGeom)
        GEOSGeom_destroy(IGeom);
    if (OGeom)
        GEOSGeom_destroy(OGeom);
    finishGEOS();
#endif
    
    Vect_cat_set(Cats, 1, 1);

    /* Should we store used buffer for later examination? */
    if (new_map->answer != NULL) {
        /* Open new vector for reading/writing */
        if (0 > Vect_open_new(&Out, new_map->answer, WITHOUT_Z)) {
            Vect_close(&In);
            G_fatal_error(_("Unable to create vector map <%s>"),
                          new_map->answer);
        }

        /* Write profile line and it's buffer into new vector map */
        Vect_write_line(&Out, GV_LINE, Profil, Cats);
        /* No category for boundary */
        Vect_reset_cats(Cats);
        Vect_write_line(&Out, GV_BOUNDARY, Buffer, Cats);
    }

    /* Here starts processing of input map */
    rescount = 0;
    resultset = NULL;

    /* If input vector has a database connection... */
    if (Fi != NULL) {
        field_index = Vect_cidx_get_field_index(&In, layer);
        if (field_index < 0) {
            G_fatal_error(_("Vector map <%s> does not have cat's defined on layer %d"),
                          old_map->answer, layer);
        }

        /* Prepeare strings for use in db_* calls */
        db_init_string(&dbsql);
        db_init_string(&valstr);
        db_init_string(&table_name);
        db_init_handle(&handle);

        G_debug(1,
                "Field number:%d; Name:<%s>; Driver:<%s>; Database:<%s>; Table:<%s>; Key:<%s>",
                Fi->number, Fi->name, Fi->driver, Fi->database, Fi->table,
                Fi->key);

        /* Prepearing database for use */
        driver = db_start_driver(Fi->driver);
        if (driver == NULL) {
            Vect_close(&In);
            G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);
        }
        db_set_handle(&handle, Fi->database, NULL);
        if (db_open_database(driver, &handle) != DB_OK) {
            Vect_close(&In);
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                          Fi->database, Fi->driver);
        }
        db_set_string(&table_name, Fi->table);
        if (db_describe_table(driver, &table_name, &table) != DB_OK) {
            Vect_close(&In);
            G_fatal_error(_("Unable to describe table <%s>"), Fi->table);
        }
        ncols = db_get_table_number_of_columns(table);

        /* Create vector feature list for future processing by applying SQL WHERE conditions... */
        if (where_opt->answer != NULL) {
            ncats =
                db_select_int(driver, Fi->table, Fi->key, where_opt->answer,
                              &cats);
            if (ncats < 1)
                G_fatal_error(_("No features match Your query"));
            for (i = 0; i < ncats; i++) {
                c = Vect_cidx_find_next(&In, field_index, cats[i],
                                        otype, 0, &type, &id);
                /* Crunch over all points/lines, that match specified CAT */
                while (c >= 0) {
                    c++;
                    if (type & otype) {
                        switch (Vect_read_line(&In, Points, Cats, id)) {
                        case GV_POINT:
                            Vect_cat_get(Cats, layer, &cat);
                            proc_point(Points, Profil, Buffer, cat,
                                       &rescount, open3d);
                            break;
                        case GV_LINE:
                            Vect_reset_line(Ipoints);
                            if (Vect_line_get_intersections
                                (Profil, Points, Ipoints, open3d) > 0) {
                                Vect_cat_get(Cats, layer, &cat);
                                proc_line(Ipoints, Profil, cat, &rescount,
                                          open3d);
                            }
                            break;
                        }
                    }
                    else
                        G_fatal_error
                            ("Error in Vect_cidx_find_next function! Report a bug.");
                    c = Vect_cidx_find_next(&In, field_index, cats[i], otype,
                                            c, &type, &id);
                }
            }
        }
    }

    /* Process all lines IF no database exists or WHERE was not provided.
       Read in single line and get its type */
    if (Fi == NULL || (where_opt->answer == NULL && Fi != NULL)) {
        while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
            if (type & GV_POINT) {
                Vect_cat_get(Cats, layer, &cat);
                proc_point(Points, Profil, Buffer, cat, &rescount, open3d);
            }
            if (type & GV_LINE) {
                Vect_reset_line(Ipoints);
                if (Vect_line_get_intersections
                    (Profil, Points, Ipoints, open3d) > 0)
                    Vect_cat_get(Cats, layer, &cat);
                proc_line(Ipoints, Profil, cat, &rescount, open3d);
            }
        }
    }
    /* We don't need input vector anymore */
    Vect_close(&In);
    G_debug(1, "There are %zu features matching profile line", rescount);

    /* Sort results by distance, cat */
    qsort(&resultset[0], rescount, sizeof(Result), compdist);

    /* Print out column names */
    if (!no_column_flag->answer) {
        fprintf(ascii, "Number%sDistance", fs);
        if (open3d == WITH_Z)
            fprintf(ascii, "%sZ", fs);
        if (Fi != NULL) {
            /* ncols are initialized here from previous Fi != NULL if block */
            for (col = 0; col < ncols; col++) {
                column = db_get_table_column(table, col);
                fprintf(ascii, "%s%s", fs, db_get_column_name(column));
            }
        }
        fprintf(ascii, "\n");
    }

    /* Print out result */
    for (j = 0; j < rescount; j++) {
        fprintf(ascii, "%zu%s%.*f", j + 1, fs, dp, resultset[j].distance);
        if (open3d == WITH_Z)
            fprintf(ascii, "%s%.*f", fs, dp, resultset[j].z);
        if (Fi != NULL) {
            sprintf(sql, "select * from %s where %s=%d", Fi->table, Fi->key,
                    resultset[j].cat);
            G_debug(2, "SQL: \"%s\"", sql);
            db_set_string(&dbsql, sql);
            /* driver IS initialized here in case if Fi != NULL */
            if (db_open_select_cursor(driver, &dbsql, &cursor, DB_SEQUENTIAL)
                != DB_OK)
                G_warning(_("Unabale to get attribute data for cat %d"),
                          resultset[j].cat);
            else {
                nrows = db_get_num_rows(&cursor);
                G_debug(1, "Result count: %d", nrows);
                table = db_get_cursor_table(&cursor);

                if (nrows > 0) {
                    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
                        G_warning(_("Error while retreiving database record for cat %d"),
                                  resultset[j].cat);
                    }
                    else {
                        for (col = 0; col < ncols; col++) {
                            /* Column description retreiving is fast, as they live in provided table structure */
                            column = db_get_table_column(table, col);
                            db_convert_column_value_to_string(column,
                                                              &valstr);
                            type = db_get_column_sqltype(column);

                            /* Those values should be quoted */
                            if (type == DB_SQL_TYPE_CHARACTER ||
                                type == DB_SQL_TYPE_DATE ||
                                type == DB_SQL_TYPE_TIME ||
                                type == DB_SQL_TYPE_TIMESTAMP ||
                                type == DB_SQL_TYPE_INTERVAL ||
                                type == DB_SQL_TYPE_TEXT ||
                                type == DB_SQL_TYPE_SERIAL)
                                fprintf(ascii, "%s\"%s\"", fs,
                                        db_get_string(&valstr));
                            else
                                fprintf(ascii, "%s%s", fs,
                                        db_get_string(&valstr));
                        }
                    }
                }
                else {
                    for (col = 0; col < ncols; col++) {
                        fprintf(ascii, "%s", fs);
                    }
                }
            }
            db_close_cursor(&cursor);
        }
        /* Terminate attribute data line and flush data to provided output (file/stdout) */
        fprintf(ascii, "\n");
        if (fflush(ascii))
            G_fatal_error(_("Can not write data portion to provided output"));
    }

    /* Build topology for vector map and close them */
    if (new_map->answer != NULL) {
        Vect_build(&Out);
        Vect_close(&Out);
    }

    if (Fi != NULL)
        db_close_database_shutdown_driver(driver);

    if (ascii != NULL)
        fclose(ascii);

    /* Don't forget to report to caller successful end of data processing :) */
    exit(EXIT_SUCCESS);
}

/* Qsort distance comparison function */
static int compdist(const void *d1, const void *d2)
{
    Result *r1 = (Result *) d1, *r2 = (Result *) d2;

    G_debug(5, "Comparing %f with %f", r1->distance, r2->distance);

    if (r1->distance == r2->distance) {
        if (r1->cat < r2->cat)
            return -1;
        else
            return (r1->cat > r2->cat);
    }
    if (r1->distance < r2->distance)
        return -1;
    else
        return (r1->distance > r2->distance);
}
