#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <grass/parson.h>

#include "local_proto.h"

#define printline(x) G_faprintf(stdout, " | %-74.74s |\n", x)
#define divider(x)             \
    fprintf(stdout, " %c", x); \
    for (i = 0; i < 76; i++)   \
        fprintf(stdout, "-");  \
    fprintf(stdout, "%c\n", x)

/* cloned from lib/gis/wind_format.c */
void format_double(double value, char buf[BUFSZ])
{
    snprintf(buf, BUFSZ, "%.8f", value);
    G_trim_decimal(buf);
}

/* convert UTM zone number X to XN or XS */
static char *format_zone(int zone_num)
{
    char *zone_str = NULL;

    if (zone_num < -60 || zone_num > 60)
        G_asprintf(&zone_str, _("%s"), "invalid");
    else if (zone_num == 0)
        G_asprintf(&zone_str, _("%s"), "unspecified");
    else if (zone_num < 0)
        G_asprintf(&zone_str, "%dS", -zone_num);
    else
        G_asprintf(&zone_str, "%dN", zone_num);

    return zone_str;
}

void print_region(struct Map_info *Map, enum OutputFormat format,
                  JSON_Object *root_object)
{
    char tmp1[1024], tmp2[1024];

    struct bound_box box;

    /* print the spatial extent as double values */
    Vect_get_map_box(Map, &box);

    switch (format) {
    case PLAIN:
        break;
    case SHELL:
        G_format_northing(box.N, tmp1, -1);
        G_format_northing(box.S, tmp2, -1);
        fprintf(stdout, "north=%s\n", tmp1);
        fprintf(stdout, "south=%s\n", tmp2);

        G_format_easting(box.E, tmp1, -1);
        G_format_easting(box.W, tmp2, -1);
        fprintf(stdout, "east=%s\n", tmp1);
        fprintf(stdout, "west=%s\n", tmp2);
        fprintf(stdout, "top=%f\n", box.T);
        fprintf(stdout, "bottom=%f\n", box.B);
        break;
    case JSON:
        json_object_set_number(root_object, "north", box.N);
        json_object_set_number(root_object, "south", box.S);
        json_object_set_number(root_object, "east", box.E);
        json_object_set_number(root_object, "west", box.W);
        json_object_set_number(root_object, "top", box.T);
        json_object_set_number(root_object, "bottom", box.B);
        break;
    }
}

void print_topo(struct Map_info *Map, enum OutputFormat format,
                JSON_Object *root_object)
{
    int with_z;
    long nprimitives;

    nprimitives = 0;
    with_z = Vect_is_3d(Map);

    nprimitives += Vect_get_num_primitives(Map, GV_POINT);
    nprimitives += Vect_get_num_primitives(Map, GV_LINE);
    nprimitives += Vect_get_num_primitives(Map, GV_BOUNDARY);
    nprimitives += Vect_get_num_primitives(Map, GV_CENTROID);

    if (with_z) {
        nprimitives += Vect_get_num_primitives(Map, GV_FACE);
        nprimitives += Vect_get_num_primitives(Map, GV_KERNEL);
    }

    switch (format) {
    case PLAIN:
        break;
    case SHELL:
        fprintf(stdout, "nodes=%d\n", Vect_get_num_nodes(Map));
        fflush(stdout);

        fprintf(stdout, "points=%d\n", Vect_get_num_primitives(Map, GV_POINT));
        fflush(stdout);

        fprintf(stdout, "lines=%d\n", Vect_get_num_primitives(Map, GV_LINE));
        fflush(stdout);

        fprintf(stdout, "boundaries=%d\n",
                Vect_get_num_primitives(Map, GV_BOUNDARY));
        fflush(stdout);

        fprintf(stdout, "centroids=%d\n",
                Vect_get_num_primitives(Map, GV_CENTROID));
        fflush(stdout);

        fprintf(stdout, "areas=%d\n", Vect_get_num_areas(Map));
        fflush(stdout);

        fprintf(stdout, "islands=%d\n", Vect_get_num_islands(Map));
        fflush(stdout);

        if (with_z) {
            fprintf(stdout, "faces=%d\n",
                    Vect_get_num_primitives(Map, GV_FACE));
            fflush(stdout);

            fprintf(stdout, "kernels=%d\n",
                    Vect_get_num_primitives(Map, GV_KERNEL));
            fflush(stdout);

            fprintf(stdout, "volumes=%d\n",
                    Vect_get_num_primitives(Map, GV_VOLUME));
            fflush(stdout);

            fprintf(stdout, "holes=%d\n", Vect_get_num_holes(Map));
            fflush(stdout);
        }

        fprintf(stdout, "primitives=%ld\n", nprimitives);
        fflush(stdout);

        fprintf(stdout, "map3d=%d\n", Vect_is_3d(Map) ? 1 : 0);
        fflush(stdout);

        break;
    case JSON:
        json_object_set_number(root_object, "nodes", Vect_get_num_nodes(Map));
        json_object_set_number(root_object, "points",
                               Vect_get_num_primitives(Map, GV_POINT));
        json_object_set_number(root_object, "lines",
                               Vect_get_num_primitives(Map, GV_LINE));
        json_object_set_number(root_object, "boundaries",
                               Vect_get_num_primitives(Map, GV_BOUNDARY));
        json_object_set_number(root_object, "centroids",
                               Vect_get_num_primitives(Map, GV_CENTROID));
        json_object_set_number(root_object, "areas", Vect_get_num_areas(Map));
        json_object_set_number(root_object, "islands",
                               Vect_get_num_islands(Map));
        if (with_z) {
            json_object_set_number(root_object, "faces",
                                   Vect_get_num_primitives(Map, GV_FACE));
            json_object_set_number(root_object, "kernels",
                                   Vect_get_num_primitives(Map, GV_KERNEL));
            json_object_set_number(root_object, "volumes",
                                   Vect_get_num_primitives(Map, GV_VOLUME));
            json_object_set_number(root_object, "holes",
                                   Vect_get_num_holes(Map));
        }
        json_object_set_number(root_object, "primitives", nprimitives);
        json_object_set_boolean(root_object, "map3d", Vect_is_3d(Map));
    }
}

void print_columns(struct Map_info *Map, const char *input_opt,
                   const char *field_opt, enum OutputFormat format)
{
    int num_dblinks, col, ncols;

    struct field_info *fi;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString table_name;
    dbTable *table;

    num_dblinks = Vect_get_num_dblinks(Map);

    if (num_dblinks <= 0) {
        Vect_close(Map);
        G_fatal_error(
            _("Database connection for map <%s> is not defined in DB file"),
            input_opt);
    }

    if (format == PLAIN) {
        fprintf(stdout,
                _("Column names and types for database connection of "
                  "layer <%s>:\n"),
                field_opt);
    }

    if ((fi = Vect_get_field2(Map, field_opt)) == NULL) {
        Vect_close(Map);
        G_fatal_error(
            _("Database connection not defined for layer <%s> of <%s>"),
            field_opt, input_opt);
    }
    driver = db_start_driver(fi->driver);
    if (driver == NULL) {
        Vect_close(Map);
        G_fatal_error(_("Unable to open driver <%s>"), fi->driver);
    }
    db_init_handle(&handle);
    db_set_handle(&handle, fi->database, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
        db_shutdown_driver(driver);
        Vect_close(Map);
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                      fi->database, fi->driver);
    }
    db_init_string(&table_name);
    db_set_string(&table_name, fi->table);
    if (db_describe_table(driver, &table_name, &table) != DB_OK) {
        db_close_database_shutdown_driver(driver);
        Vect_close(Map);
        G_fatal_error(_("Unable to describe table <%s>"), fi->table);
    }

    JSON_Value *root_value = NULL, *columns_value = NULL, *column_value = NULL;
    JSON_Object *root_object = NULL, *column_object = NULL;
    JSON_Array *columns_array = NULL;

    if (format == JSON) {
        root_value = json_value_init_object();
        root_object = json_object(root_value);
        columns_value = json_value_init_array();
        columns_array = json_array(columns_value);
        json_object_set_value(root_object, "columns", columns_value);
    }

    ncols = db_get_table_number_of_columns(table);
    for (col = 0; col < ncols; col++) {
        switch (format) {
        case SHELL:
            fprintf(stdout, "%s|%s\n",
                    db_sqltype_name(
                        db_get_column_sqltype(db_get_table_column(table, col))),
                    db_get_column_name(db_get_table_column(table, col)));
            break;

        case JSON:
            column_value = json_value_init_object();
            column_object = json_object(column_value);

            json_object_set_string(
                column_object, "name",
                db_get_column_name(db_get_table_column(table, col)));

            int sql_type =
                db_get_column_sqltype(db_get_table_column(table, col));
            json_object_set_string(column_object, "sql_type",
                                   db_sqltype_name(sql_type));

            int c_type = db_sqltype_to_Ctype(sql_type);
            json_object_set_boolean(
                column_object, "is_number",
                (c_type == DB_C_TYPE_INT || c_type == DB_C_TYPE_DOUBLE));

            json_array_append_value(columns_array, column_value);
            break;

        case PLAIN:
            fprintf(stdout, "%s: %s\n",
                    db_get_column_name(db_get_table_column(table, col)),
                    db_sqltype_name(db_get_column_sqltype(
                        db_get_table_column(table, col))));
            break;
        }
    }

    if (format == JSON) {
        char *serialized_string = NULL;
        serialized_string = json_serialize_to_string_pretty(root_value);
        if (serialized_string == NULL) {
            json_value_free(root_value);
            db_close_database_shutdown_driver(driver);
            Vect_close(Map);
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
    }

    Vect_destroy_field_info(fi);
    db_close_database_shutdown_driver(driver);
}

void print_shell(struct Map_info *Map, const char *field_opt,
                 enum OutputFormat format, JSON_Object *root_object)
{
    int map_type;
    int time_ok, first_time_ok, second_time_ok;
    char timebuff[256];
    struct field_info *fi;
    struct TimeStamp ts;

    time_ok = first_time_ok = second_time_ok = FALSE;

    /* Check the Timestamp */
    time_ok = G_read_vector_timestamp(Vect_get_name(Map), NULL, "", &ts);

    /* Check for valid entries, show none if no timestamp available */
    if (time_ok == TRUE) {
        if (ts.count > 0)
            first_time_ok = TRUE;
        if (ts.count > 1)
            second_time_ok = TRUE;
    }

    map_type = Vect_maptype(Map);
    const char *maptype_str = Vect_maptype_info(Map);
    char *finfo_lname = Vect_get_finfo_layer_name(Map);
    const char *geom_type = Vect_get_finfo_geometry_type(Map);

    char scale_tmp[18];
    snprintf(scale_tmp, 18, "1:%d", Vect_get_scale(Map));

    switch (format) {
    case PLAIN:
        break;
    case SHELL:
        fprintf(stdout, "name=%s\n", Vect_get_name(Map));
        fprintf(stdout, "mapset=%s\n", Vect_get_mapset(Map));
        fprintf(stdout, "location=%s\n", G_location());
        fprintf(stdout, "project=%s\n", G_location());
        fprintf(stdout, "database=%s\n", G_gisdbase());
        fprintf(stdout, "title=%s\n", Vect_get_map_name(Map));
        fprintf(stdout, "scale=%s\n", scale_tmp);
        fprintf(stdout, "creator=%s\n", Vect_get_person(Map));
        fprintf(stdout, "organization=%s\n", Vect_get_organization(Map));
        fprintf(stdout, "source_date=%s\n", Vect_get_map_date(Map));
        break;
    case JSON:
        json_object_set_string(root_object, "name", Vect_get_name(Map));
        json_object_set_string(root_object, "mapset", Vect_get_mapset(Map));
        json_object_set_string(root_object, "project", G_location());
        json_object_set_string(root_object, "database", G_gisdbase());
        json_object_set_string(root_object, "title", Vect_get_map_name(Map));
        json_object_set_string(root_object, "scale", scale_tmp);
        json_object_set_string(root_object, "creator", Vect_get_person(Map));
        json_object_set_string(root_object, "organization",
                               Vect_get_organization(Map));
        json_object_set_string(root_object, "source_date",
                               Vect_get_map_date(Map));
        break;
    }

    /* This shows the TimeStamp (if present) */
    if (time_ok == TRUE && (first_time_ok || second_time_ok)) {
        G_format_timestamp(&ts, timebuff);
        switch (format) {
        case PLAIN:
            break;
        case SHELL:
            fprintf(stdout, "timestamp=%s\n", timebuff);
            break;
        case JSON:
            json_object_set_string(root_object, "timestamp", timebuff);
            break;
        }
    }
    else {
        switch (format) {
        case PLAIN:
            break;
        case SHELL:
            fprintf(stdout, "timestamp=none\n");
            break;
        case JSON:
            json_object_set_null(root_object, "timestamp");
            break;
        }
    }

    if (map_type == GV_FORMAT_OGR || map_type == GV_FORMAT_OGR_DIRECT) {
        switch (format) {
        case PLAIN:
            break;
        case SHELL:
            fprintf(stdout, "format=%s,%s\n", maptype_str,
                    Vect_get_finfo_format_info(Map));
            fprintf(stdout, "ogr_layer=%s\n", finfo_lname);
            fprintf(stdout, "ogr_dsn=%s\n", Vect_get_finfo_dsn_name(Map));
            fprintf(stdout, "feature_type=%s\n", geom_type);
            break;
        case JSON:
            json_object_set_string(root_object, "format", maptype_str);
            json_object_set_string(root_object, "format-detail",
                                   Vect_get_finfo_format_info(Map));
            json_object_set_string(root_object, "ogr_layer", finfo_lname);
            json_object_set_string(root_object, "ogr_dsn",
                                   Vect_get_finfo_dsn_name(Map));
            json_object_set_string(root_object, "feature_type", geom_type);
            break;
        }
    }
    else if (map_type == GV_FORMAT_POSTGIS) {
        int topo_format;
        char *toposchema_name = NULL, *topogeom_column = NULL;
        const struct Format_info *finfo;

        finfo = Vect_get_finfo(Map);

        switch (format) {
        case PLAIN:
            break;
        case SHELL:
            fprintf(stdout, "format=%s,%s\n", maptype_str,
                    Vect_get_finfo_format_info(Map));
            fprintf(stdout, "pg_table=%s\n", finfo_lname);
            fprintf(stdout, "pg_dbname=%s\n", Vect_get_finfo_dsn_name(Map));
            fprintf(stdout, "geometry_column=%s\n", finfo->pg.geom_column);
            fprintf(stdout, "feature_type=%s\n", geom_type);
            break;
        case JSON:
            json_object_set_string(root_object, "format", maptype_str);
            json_object_set_string(root_object, "format-detail",
                                   Vect_get_finfo_format_info(Map));
            json_object_set_string(root_object, "pg_table", finfo_lname);
            json_object_set_string(root_object, "pg_dbname",
                                   Vect_get_finfo_dsn_name(Map));
            json_object_set_string(root_object, "geometry_column",
                                   finfo->pg.geom_column);
            json_object_set_string(root_object, "feature_type", geom_type);
            break;
        }

        topo_format = Vect_get_finfo_topology_info(Map, &toposchema_name,
                                                   &topogeom_column, NULL);
        if (topo_format == GV_TOPO_POSTGIS) {
            switch (format) {
            case PLAIN:
                break;
            case SHELL:
                fprintf(stdout, "pg_topo_schema=%s\n", toposchema_name);
                fprintf(stdout, "pg_topo_column=%s\n", topogeom_column);
                break;
            case JSON:
                json_object_set_string(root_object, "pg_topo_schema",
                                       toposchema_name);
                json_object_set_string(root_object, "pg_topo_column",
                                       topogeom_column);
                break;
            }
        }
        G_free(topogeom_column);
        G_free(toposchema_name);
    }
    else {
        switch (format) {
        case PLAIN:
            break;
        case SHELL:
            fprintf(stdout, "format=%s\n", maptype_str);
            break;
        case JSON:
            json_object_set_string(root_object, "format", maptype_str);
            break;
        }
    }

    switch (format) {
    case PLAIN:
        break;
    case SHELL:
        fprintf(stdout, "level=%d\n", Vect_level(Map));
        break;
    case JSON:
        json_object_set_number(root_object, "level", Vect_level(Map));
        break;
    }
    if (Vect_level(Map) > 0) {
        switch (format) {
        case PLAIN:
            break;
        case SHELL:
            fprintf(stdout, "num_dblinks=%d\n", Vect_get_num_dblinks(Map));
            break;
        case JSON:
            json_object_set_number(root_object, "num_dblinks",
                                   Vect_get_num_dblinks(Map));
            break;
        }

        if (Vect_get_num_dblinks(Map) > 0) {
            fi = Vect_get_field2(Map, field_opt);
            if (fi != NULL) {
                switch (format) {
                case PLAIN:
                    break;
                case SHELL:
                    fprintf(stdout, "attribute_layer_number=%i\n", fi->number);
                    fprintf(stdout, "attribute_layer_name=%s\n", fi->name);
                    fprintf(stdout, "attribute_database=%s\n", fi->database);
                    fprintf(stdout, "attribute_database_driver=%s\n",
                            fi->driver);
                    fprintf(stdout, "attribute_table=%s\n", fi->table);
                    fprintf(stdout, "attribute_primary_key=%s\n", fi->key);
                    break;
                case JSON:
                    json_object_set_number(
                        root_object, "attribute_layer_number", fi->number);
                    json_object_set_string(root_object, "attribute_layer_name",
                                           fi->name);
                    json_object_set_string(root_object, "attribute_database",
                                           fi->database);
                    json_object_set_string(
                        root_object, "attribute_database_driver", fi->driver);
                    json_object_set_string(root_object, "attribute_table",
                                           fi->table);
                    json_object_set_string(root_object, "attribute_primary_key",
                                           fi->key);
                    break;
                }
            }
            Vect_destroy_field_info(fi);
        }
    }

    switch (format) {
    case PLAIN:
        break;
    case SHELL:
        fprintf(stdout, "projection=%s\n", Vect_get_proj_name(Map));
        if (G_projection() == PROJECTION_UTM) {
            fprintf(stdout, "zone=%d\n", Vect_get_zone(Map));
        }
        fprintf(stdout, "digitization_threshold=%f\n", Vect_get_thresh(Map));
        fprintf(stdout, "comment=%s\n", Vect_get_comment(Map));
        break;
    case JSON:
        json_object_set_string(root_object, "projection",
                               Vect_get_proj_name(Map));
        if (G_projection() == PROJECTION_UTM) {
            json_object_set_number(root_object, "zone", Vect_get_zone(Map));
        }
        json_object_set_number(root_object, "digitization_threshold",
                               Vect_get_thresh(Map));
        json_object_set_string(root_object, "comment", Vect_get_comment(Map));
        break;
    }
    G_free(finfo_lname);
    G_free((void *)maptype_str);
    G_free((void *)geom_type);
}

void print_info(struct Map_info *Map)
{
    int i, map_type;
    char line[1024];
    char timebuff[256];
    struct TimeStamp ts;
    int time_ok, first_time_ok, second_time_ok;
    struct bound_box box;
    char tmp1[BUFSZ], tmp2[BUFSZ];

    time_ok = first_time_ok = second_time_ok = FALSE;
    map_type = Vect_maptype(Map);
    const char *maptype_str = Vect_maptype_info(Map);
    char *finfo_lname = Vect_get_finfo_layer_name(Map);
    const char *geom_type = Vect_get_finfo_geometry_type(Map);

    /* Check the Timestamp */
    time_ok = G_read_vector_timestamp(Vect_get_name(Map), NULL, "", &ts);

    /* Check for valid entries, show none if no timestamp available */
    if (time_ok == TRUE) {
        if (ts.count > 0)
            first_time_ok = TRUE;
        if (ts.count > 1)
            second_time_ok = TRUE;
    }

    divider('+');
    G_saprintf(line, "%-17s%s", _("Name:"), Vect_get_name(Map));
    printline(line);
    G_saprintf(line, "%-17s%s", _("Mapset:"), Vect_get_mapset(Map));
    printline(line);

    G_saprintf(line, "%-17s%s", _("Project:"), G_location());
    printline(line);
    G_saprintf(line, "%-17s%s", _("Database:"), G_gisdbase());
    printline(line);

    G_saprintf(line, "%-17s%s", _("Title:"), Vect_get_map_name(Map));
    printline(line);
    G_saprintf(line, "%-17s1:%d", _("Map scale:"), Vect_get_scale(Map));
    printline(line);

    G_saprintf(line, "%-17s%s", _("Name of creator:"), Vect_get_person(Map));
    printline(line);
    G_saprintf(line, "%-17s%s", _("Organization:"), Vect_get_organization(Map));
    printline(line);
    G_saprintf(line, "%-17s%s", _("Source date:"), Vect_get_map_date(Map));
    printline(line);

    /* This shows the TimeStamp (if present) */
    if (time_ok == TRUE && (first_time_ok || second_time_ok)) {
        G_format_timestamp(&ts, timebuff);
        G_saprintf(line, "%-17s%s", _("Timestamp (first layer): "), timebuff);
        printline(line);
    }
    else {
        strcpy(line, _("Timestamp (first layer): none"));
        printline(line);
    }

    divider('|');

    if (map_type == GV_FORMAT_OGR || map_type == GV_FORMAT_OGR_DIRECT) {
        G_saprintf(line, "%-17s%s (%s)", _("Map format:"), maptype_str,
                   Vect_get_finfo_format_info(Map));
        printline(line);

        /* for OGR format print also datasource and layer */
        G_saprintf(line, "%-17s%s", _("OGR layer:"), finfo_lname);
        printline(line);
        G_saprintf(line, "%-17s%s", _("OGR datasource:"),
                   Vect_get_finfo_dsn_name(Map));
        printline(line);
        G_saprintf(line, "%-17s%s", _("Feature type:"), geom_type);
        printline(line);
    }
    else if (map_type == GV_FORMAT_POSTGIS) {
        int topo_format;
        char *toposchema_name = NULL, *topogeom_column = NULL;
        int topo_geo_only;

        const struct Format_info *finfo;

        finfo = Vect_get_finfo(Map);

        G_saprintf(line, "%-17s%s (%s)", _("Map format:"), maptype_str,
                   Vect_get_finfo_format_info(Map));
        printline(line);

        /* for PostGIS format print also datasource and layer */
        G_saprintf(line, "%-17s%s", _("DB table:"), finfo_lname);
        printline(line);
        G_saprintf(line, "%-17s%s", _("DB name:"),
                   Vect_get_finfo_dsn_name(Map));
        printline(line);

        G_saprintf(line, "%-17s%s", _("Geometry column:"),
                   finfo->pg.geom_column);
        printline(line);

        G_saprintf(line, "%-17s%s", _("Feature type:"), geom_type);
        printline(line);

        topo_format = Vect_get_finfo_topology_info(
            Map, &toposchema_name, &topogeom_column, &topo_geo_only);
        if (topo_format == GV_TOPO_POSTGIS) {
            G_saprintf(line, "%-17s%s (%s %s%s)", _("Topology:"), "PostGIS",
                       _("schema:"), toposchema_name,
                       topo_geo_only ? ", topo-geo-only: yes" : "");
            printline(line);

            G_saprintf(line, "%-17s%s", _("Topology column:"), topogeom_column);
        }
        else
            G_saprintf(line, "%-17s%s", _("Topology:"),
                       "pseudo (simple features)");

        printline(line);
        G_free(toposchema_name);
        G_free(topogeom_column);
    }
    else {
        G_saprintf(line, "%-17s%s", _("Map format:"), maptype_str);
        printline(line);
    }

    divider('|');

    G_saprintf(line, "  %s: %s (%s: %i)", _("Type of map"), _("vector"),
               _("level"), Vect_level(Map));
    printline(line);

    if (Vect_level(Map) > 0) {
        printline("");
        G_saprintf(line, "  %-24s%-9d       %-22s%-9d", _("Number of points:"),
                   Vect_get_num_primitives(Map, GV_POINT),
                   _("Number of centroids:"),
                   Vect_get_num_primitives(Map, GV_CENTROID));
        printline(line);
        G_saprintf(line, "  %-24s%-9d       %-22s%-9d", _("Number of lines:"),
                   Vect_get_num_primitives(Map, GV_LINE),
                   _("Number of boundaries:"),
                   Vect_get_num_primitives(Map, GV_BOUNDARY));
        printline(line);
        G_saprintf(line, "  %-24s%-9d       %-22s%-9d", _("Number of areas:"),
                   Vect_get_num_areas(Map), _("Number of islands:"),
                   Vect_get_num_islands(Map));
        printline(line);
        if (Vect_is_3d(Map)) {
            G_saprintf(
                line, "  %-24s%-9d       %-22s%-9d", _("Number of faces:"),
                Vect_get_num_primitives(Map, GV_FACE), _("Number of kernels:"),
                Vect_get_num_primitives(Map, GV_KERNEL));
            printline(line);
            G_saprintf(line, "  %-24s%-9d       %-22s%-9d",
                       _("Number of volumes:"), Vect_get_num_volumes(Map),
                       _("Number of holes:"), Vect_get_num_holes(Map));
            printline(line);
        }
        printline("");

        G_saprintf(line, "  %-24s%s", _("Map is 3D:"),
                   Vect_is_3d(Map) ? _("Yes") : _("No"));
        printline(line);
        G_saprintf(line, "  %-24s%-9d", _("Number of dblinks:"),
                   Vect_get_num_dblinks(Map));
        printline(line);
    }

    printline("");
    /* this differs from r.info in that proj info IS taken from the map here,
     * not the location settings */
    /* Vect_get_proj_name() and _zone() are typically unset?! */
    if (G_projection() == PROJECTION_UTM) {
        int utm_zone;
        char *utm_zone_str;

        utm_zone = Vect_get_zone(Map);
        utm_zone_str = format_zone(utm_zone);

        G_saprintf(line, "  %s: %s (%s %s)", _("Projection"),
                   Vect_get_proj_name(Map), _("zone"), utm_zone_str);
    }
    else
        G_saprintf(line, "  %s: %s", _("Projection"), Vect_get_proj_name(Map));

    printline(line);
    printline("");

    Vect_get_map_box(Map, &box);

    G_format_northing(box.N, tmp1, G_projection());
    G_format_northing(box.S, tmp2, G_projection());
    snprintf(line, sizeof(line), "              %c: %17s    %c: %17s", 'N',
             tmp1, 'S', tmp2);
    printline(line);

    G_format_easting(box.E, tmp1, G_projection());
    G_format_easting(box.W, tmp2, G_projection());
    snprintf(line, sizeof(line), "              %c: %17s    %c: %17s", 'E',
             tmp1, 'W', tmp2);
    printline(line);

    if (Vect_is_3d(Map)) {
        format_double(box.B, tmp1);
        format_double(box.T, tmp2);
        snprintf(line, sizeof(line), "              %c: %17s    %c: %17s", 'B',
                 tmp1, 'T', tmp2);
        printline(line);
    }
    printline("");

    format_double(Vect_get_thresh(Map), tmp1);
    G_saprintf(line, "  %s: %s", _("Digitization threshold"), tmp1);
    printline(line);
    G_saprintf(line, "  %s:", _("Comment"));
    printline(line);
    snprintf(line, sizeof(line), "    %s", Vect_get_comment(Map));
    printline(line);
    divider('+');
    fprintf(stdout, "\n");
    G_free((void *)maptype_str);
    G_free(finfo_lname);
    G_free((void *)geom_type);
}

/*!
   \brief Extracts and assigns values from a history line to command, gisdbase,
   location, mapset, user, date, and mapset_path based on specific prefixes.

 */
void parse_history_line(const char *buf, char *command, char *gisdbase,
                        char *location, char *mapset, char *user, char *date,
                        char *mapset_path)
{
    if (strncmp(buf, "COMMAND:", 8) == 0) {
        sscanf(buf, "COMMAND: %[^\n]", command);
    }
    else if (strncmp(buf, "GISDBASE:", 9) == 0) {
        sscanf(buf, "GISDBASE: %[^\n]", gisdbase);
    }
    else if (strncmp(buf, "LOCATION:", 9) == 0) {
        sscanf(buf, "LOCATION: %s MAPSET: %s USER: %s DATE: %[^\n]", location,
               mapset, user, date);

        snprintf(mapset_path, GPATH_MAX, "%s/%s/%s", gisdbase, location,
                 mapset);
    }
}

/*!
   \brief Creates a JSON object with fields for command, user, date, and
   mapset_path, appends it to a JSON array.

 */
void add_record_to_json(char *command, char *user, char *date,
                        char *mapset_path, JSON_Array *record_array,
                        int history_number)
{

    JSON_Value *info_value = json_value_init_object();
    if (info_value == NULL) {
        G_fatal_error(_("Failed to initialize JSON object. Out of memory?"));
    }
    JSON_Object *info_object = json_object(info_value);

    json_object_set_number(info_object, "history_number", history_number);
    json_object_set_string(info_object, "command", command);
    json_object_set_string(info_object, "mapset_path", mapset_path);
    json_object_set_string(info_object, "user", user);
    json_object_set_string(info_object, "date", date);

    json_array_append_value(record_array, info_value);
}

/*!
   \brief Reads history entries from a map, formats them based on the specified
   output format (PLAIN, SHELL, or JSON), and prints the results.

 */
void print_history(struct Map_info *Map, enum OutputFormat format)
{
    int history_number = 0;

    char buf[STR_LEN] = {0};
    char command[STR_LEN] = {0}, gisdbase[STR_LEN] = {0};
    char location[STR_LEN] = {0}, mapset[STR_LEN] = {0};
    char user[STR_LEN] = {0}, date[STR_LEN] = {0};
    char mapset_path[GPATH_MAX] = {0};

    JSON_Value *root_value = NULL, *record_value = NULL;
    JSON_Object *root_object = NULL;
    JSON_Array *record_array = NULL;

    if (format == JSON) {
        root_value = json_value_init_object();
        if (root_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        root_object = json_object(root_value);

        record_value = json_value_init_array();
        if (record_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        record_array = json_array(record_value);
    }

    Vect_hist_rewind(Map);
    while (Vect_hist_read(buf, sizeof(buf) - 1, Map) != NULL) {
        switch (format) {
        case PLAIN:
        case SHELL:
            fprintf(stdout, "%s\n", buf);
            break;
        case JSON:
            // Parse each line based on its prefix
            parse_history_line(buf, command, gisdbase, location, mapset, user,
                               date, mapset_path);
            if (command[0] != '\0' && mapset_path[0] != '\0' &&
                user[0] != '\0' && date[0] != '\0') {
                // Increment history counter
                history_number++;

                add_record_to_json(command, user, date, mapset_path,
                                   record_array, history_number);

                // Clear the input strings before processing new
                // entries in the history file
                command[0] = '\0';
                user[0] = '\0';
                date[0] = '\0';
                mapset_path[0] = '\0';
            }
            break;
        }
    }

    if (format == JSON) {
        json_object_set_value(root_object, "records", record_value);

        char *serialized_string = json_serialize_to_string_pretty(root_value);
        if (!serialized_string) {
            json_value_free(root_value);
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);

        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
    }
}
