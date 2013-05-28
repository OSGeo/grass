#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

#define printline(x) fprintf (stdout, " | %-74.74s |\n", x)
#define divider(x) \
        fprintf (stdout, " %c", x); \
        for (i = 0; i < 76; i++ ) \
                fprintf ( stdout, "-" ); \
        fprintf (stdout, "%c\n", x)

/* cloned from lib/gis/wind_format.c */
void format_double(double value, char *buf)
{
    sprintf(buf, "%.8f", value);
    G_trim_decimal(buf);
}

void print_region(const struct Map_info *Map)
{
    char tmp1[1024], tmp2[1024];
    
    struct bound_box box;

    /* print the spatial extent as double values */
    Vect_get_map_box(Map, &box);
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
}

void print_topo(const struct Map_info *Map)
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
    

    fprintf(stdout, "nodes=%d\n",
            Vect_get_num_nodes(Map));
    fflush(stdout);

    fprintf(stdout, "points=%d\n",
            Vect_get_num_primitives(Map, GV_POINT));
    fflush(stdout);
    
    fprintf(stdout, "lines=%d\n",
            Vect_get_num_primitives(Map, GV_LINE));
    fflush(stdout);
    
    fprintf(stdout, "boundaries=%d\n",
            Vect_get_num_primitives(Map, GV_BOUNDARY));
    fflush(stdout);
    
    fprintf(stdout, "centroids=%d\n",
            Vect_get_num_primitives(Map, GV_CENTROID));
    fflush(stdout);
    
    fprintf(stdout, "areas=%d\n", Vect_get_num_areas(Map));
    fflush(stdout);
    
    fprintf(stdout, "islands=%d\n",
            Vect_get_num_islands(Map));
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
        
        fprintf(stdout, "holes=%d\n",
                Vect_get_num_holes(Map));
        fflush(stdout);
    }

    fprintf(stdout, "primitives=%ld\n", nprimitives);
    fflush(stdout);

    fprintf(stdout, "map3d=%d\n",
            Vect_is_3d(Map) ? 1 : 0);
    fflush(stdout);
}

void print_columns(const struct Map_info *Map, const char *input_opt, const char *field_opt)
{
    int num_dblinks, col, ncols;

    struct field_info *fi;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString table_name;
    dbTable *table;

    num_dblinks = Vect_get_num_dblinks(Map);

    if (num_dblinks <= 0) {
        G_fatal_error(_("Database connection for map <%s> is not defined in DB file"),
                      input_opt);
    }

    G_message(_("Displaying column types/names for database connection of layer <%s>:"),
              field_opt);

    if ((fi = Vect_get_field2(Map, field_opt)) == NULL)
        G_fatal_error(_("Database connection not defined for layer <%s>"),
                      field_opt);
    driver = db_start_driver(fi->driver);
    if (driver == NULL)
        G_fatal_error(_("Unable to open driver <%s>"),
                      fi->driver);
    db_init_handle(&handle);
    db_set_handle(&handle, fi->database, NULL);
    if (db_open_database(driver, &handle) != DB_OK)
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                      fi->database, fi->driver);
    db_init_string(&table_name);
    db_set_string(&table_name, fi->table);
    if (db_describe_table(driver, &table_name, &table) != DB_OK)
        G_fatal_error(_("Unable to describe table <%s>"),
                      fi->table);
    
    ncols = db_get_table_number_of_columns(table);
    for (col = 0; col < ncols; col++)
        fprintf(stdout, "%s|%s\n",
                db_sqltype_name(db_get_column_sqltype
                                (db_get_table_column
                                 (table, col))),
                db_get_column_name(db_get_table_column
                                   (table, col)));
    
    db_close_database(driver);
    db_shutdown_driver(driver);
}

void print_shell(const struct Map_info *Map)
{
    int map_type;
    int time_ok, first_time_ok, second_time_ok;
    char timebuff[256];
    
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
    
    fprintf(stdout, "name=%s\n",
            Vect_get_name(Map));
    fprintf(stdout, "mapset=%s\n",
            Vect_get_mapset(Map));
    fprintf(stdout, "location=%s\n",
            G_location());
    fprintf(stdout, "database=%s\n",
            G_gisdbase());
    fprintf(stdout, "title=%s\n",
            Vect_get_map_name(Map));
    fprintf(stdout, "scale=1:%d\n",
            Vect_get_scale(Map));
    fprintf(stdout, "creator=%s\n",
            Vect_get_person(Map));
    fprintf(stdout, "organization=%s\n",
            Vect_get_organization(Map));
    fprintf(stdout, "source_date=%s\n",
            Vect_get_map_date(Map));
    /* This shows the TimeStamp (if present) */
    if (time_ok  == TRUE && (first_time_ok || second_time_ok)) {
        G_format_timestamp(&ts, timebuff);
        fprintf(stdout, "timestamp=%s\n", timebuff);
    }
    else {
        fprintf(stdout, "timestamp=none\n");
    }

    if (map_type == GV_FORMAT_OGR ||
        map_type == GV_FORMAT_OGR_DIRECT) {
        fprintf(stdout, "format=%s,%s\n",
                Vect_maptype_info(Map), Vect_get_finfo_format_info(Map));
        fprintf(stdout, "ogr_layer=%s\n",
                Vect_get_finfo_layer_name(Map));
        fprintf(stdout, "ogr_dsn=%s\n",
                Vect_get_finfo_dsn_name(Map));
        fprintf(stdout, "feature_type=%s\n",
                Vect_get_finfo_geometry_type(Map));

    }
    else if (map_type == GV_FORMAT_POSTGIS) {
        const struct Format_info *finfo;
        
        finfo = Vect_get_finfo(Map);
        
        fprintf(stdout, "format=%s,%s\n",
                Vect_maptype_info(Map), Vect_get_finfo_format_info(Map));
        fprintf(stdout, "pg_table=%s\n",
                Vect_get_finfo_layer_name(Map));
        fprintf(stdout, "pg_dbname=%s\n",
                Vect_get_finfo_dsn_name(Map));
        fprintf(stdout, "geometry_column=%s\n",
                finfo->pg.geom_column);
        fprintf(stdout, "feature_type=%s\n",
                Vect_get_finfo_geometry_type(Map));
        if (finfo->pg.toposchema_name) {
            fprintf(stdout, "pg_topo_schema=%s\n",
                    finfo->pg.toposchema_name);
            fprintf(stdout, "pg_topo_column=%s\n",
                    finfo->pg.topogeom_column);
        }
    }
    else {
        fprintf(stdout, "format=%s\n",
                Vect_maptype_info(Map));
    }

    fprintf(stdout, "level=%d\n", 
            Vect_level(Map));
    
    if (Vect_level(Map) > 0) {
        fprintf(stdout, "num_dblinks=%d\n",
                Vect_get_num_dblinks(Map));
    }

    fprintf(stdout, "projection=%s\n",
            Vect_get_proj_name(Map));
    if (G_projection() == PROJECTION_UTM) {
        fprintf(stdout, "zone=%d\n",
                Vect_get_zone(Map));
    }
    fprintf(stdout, "digitization_threshold=%f\n",
            Vect_get_thresh(Map));
    fprintf(stdout, "comment=%s\n",
            Vect_get_comment(Map));
}

void print_info(const struct Map_info *Map)
{
    int i, map_type;
    char line[1024];
    char tmp1[1024], tmp2[1024];
    char timebuff[256];
    struct TimeStamp ts;
    int time_ok, first_time_ok, second_time_ok;
    struct bound_box box;
    int utm_zone;
   
    time_ok = first_time_ok = second_time_ok = FALSE;
    utm_zone = -1;
    map_type = Vect_maptype(Map);
    
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
    sprintf(line, "%-17s%s", _("Name:"),
            Vect_get_name(Map));
    printline(line);
    sprintf(line, "%-17s%s", _("Mapset:"),
            Vect_get_mapset(Map));
    printline(line);
    
    sprintf(line, "%-17s%s", _("Location:"),
            G_location());
    printline(line);
    sprintf(line, "%-17s%s", _("Database:"),
            G_gisdbase());
    printline(line);

    sprintf(line, "%-17s%s", _("Title:"),
            Vect_get_map_name(Map));
    printline(line);
    sprintf(line, "%-17s1:%d", _("Map scale:"),
            Vect_get_scale(Map));
    printline(line);

    sprintf(line, "%-17s%s", _("Name of creator:"),
            Vect_get_person(Map));
    printline(line);
    sprintf(line, "%-17s%s", _("Organization:"),
            Vect_get_organization(Map));
    printline(line);
    sprintf(line, "%-17s%s", _("Source date:"),
            Vect_get_map_date(Map));
    printline(line);

    /* This shows the TimeStamp (if present) */
    if (time_ok  == TRUE && (first_time_ok || second_time_ok)) {
        G_format_timestamp(&ts, timebuff);
        sprintf(line, "%-17s%s", _("Timestamp (first layer): "), timebuff);
        printline(line);
    }
    else {
        strcpy(line, _("Timestamp (first layer): none"));
        printline(line);
    }
    
    divider('|');
    
    if (map_type == GV_FORMAT_OGR ||
        map_type == GV_FORMAT_OGR_DIRECT) {
        sprintf(line, "%-17s%s (%s)", _("Map format:"),
                Vect_maptype_info(Map), Vect_get_finfo_format_info(Map));
        printline(line);
        
        /* for OGR format print also datasource and layer */
        sprintf(line, "%-17s%s", _("OGR layer:"),
                Vect_get_finfo_layer_name(Map));
        printline(line);
        sprintf(line, "%-17s%s", _("OGR datasource:"),
                Vect_get_finfo_dsn_name(Map));
        printline(line);
        sprintf(line, "%-17s%s", _("Feature type:"),
                Vect_get_finfo_geometry_type(Map));
        printline(line);
    }
    else if (map_type == GV_FORMAT_POSTGIS) {
        const struct Format_info *finfo;

        finfo = Vect_get_finfo(Map);
        
        sprintf(line, "%-17s%s (%s)", _("Map format:"),
                Vect_maptype_info(Map), Vect_get_finfo_format_info(Map));
        printline(line);
        
        /* for PostGIS format print also datasource and layer */
        sprintf(line, "%-17s%s", _("DB table:"),
                Vect_get_finfo_layer_name(Map));
        printline(line);
        sprintf(line, "%-17s%s", _("DB name:"),
                Vect_get_finfo_dsn_name(Map));
        printline(line);
        sprintf(line, "%-17s%s", _("Geometry column:"),
                finfo->pg.geom_column);
        printline(line);

        sprintf(line, "%-17s%s", _("Feature type:"),
                Vect_get_finfo_geometry_type(Map));
        printline(line);
        if (finfo->pg.toposchema_name) {
            sprintf(line, "%-17s%s (%s %s%s)", _("Topology:"), "PostGIS",
                    _("schema:"), finfo->pg.toposchema_name,
                    finfo->pg.topo_geo_only ? ", topo-geo-only: yes" : "");
            printline(line);

            sprintf(line, "%-17s%s", _("Topology column:"),
                    finfo->pg.topogeom_column);
        }
        else
            sprintf(line, "%-17s%s", _("Topology:"), "pseudo (simple features)");
        
        printline(line);
    }
    else {
        sprintf(line, "%-17s%s", _("Map format:"),
                Vect_maptype_info(Map));
        printline(line);
    }
    

    divider('|');
    
    sprintf(line, "  %s: %s (%s: %i)",
            _("Type of map"), _("vector"), _("level"), Vect_level(Map));
    printline(line);
    
    if (Vect_level(Map) > 0) {
        printline("");
        sprintf(line,
                "  %-24s%-9d       %-22s%-9d",
                _("Number of points:"), 
                Vect_get_num_primitives(Map, GV_POINT),
                _("Number of centroids:"),
                Vect_get_num_primitives(Map, GV_CENTROID));
        printline(line);
        sprintf(line,
                "  %-24s%-9d       %-22s%-9d",
                _("Number of lines:"),
                Vect_get_num_primitives(Map, GV_LINE),
                _("Number of boundaries:"),
                Vect_get_num_primitives(Map, GV_BOUNDARY));
        printline(line);
        sprintf(line,
                "  %-24s%-9d       %-22s%-9d",
                _("Number of areas:"),
                Vect_get_num_areas(Map),
                _("Number of islands:"),
                Vect_get_num_islands(Map));
        printline(line);
        if (Vect_is_3d(Map)) {
            sprintf(line,
                    "  %-24s%-9d       %-22s%-9d",
                    _("Number of faces:"),
                    Vect_get_num_primitives(Map, GV_FACE),
                    _("Number of kernels:"),
                    Vect_get_num_primitives(Map, GV_KERNEL));
            printline(line);
            sprintf(line,
                    "  %-24s%-9d       %-22s%-9d",
                    _("Number of volumes:"),
                    Vect_get_num_volumes(Map),
                    _("Number of holes:"),
                    Vect_get_num_holes(Map));
            printline(line);
        }
        printline("");

        sprintf(line, "  %-24s%s",
                _("Map is 3D:"),
                Vect_is_3d(Map) ? _("Yes") : _("No"));
        printline(line);
        sprintf(line, "  %-24s%-9d",
                _("Number of dblinks:"),
                Vect_get_num_dblinks(Map));
        printline(line);
    }

    printline("");
    /* this differs from r.info in that proj info IS taken from the map here, not the location settings */
    /* Vect_get_proj_name() and _zone() are typically unset?! */
    if (G_projection() == PROJECTION_UTM) {
        utm_zone = Vect_get_zone(Map);
        if (utm_zone < 0 || utm_zone > 60)
            strcpy(tmp1, _("invalid"));
        else if (utm_zone == 0)
            strcpy(tmp1, _("unspecified"));
        else
            sprintf(tmp1, "%d", utm_zone);

        sprintf(line, "  %s: %s (%s %s)",
                _("Projection"), Vect_get_proj_name(Map),
                _("zone"), tmp1);
    }
    else
        sprintf(line, "  %s: %s",
                _("Projection"), Vect_get_proj_name(Map));

    printline(line);
    printline("");

    Vect_get_map_box(Map, &box);

    G_format_northing(box.N, tmp1, G_projection());
    G_format_northing(box.S, tmp2, G_projection());
    sprintf(line, "              %c: %17s    %c: %17s",
            'N', tmp1, 'S', tmp2);
    printline(line);
    
    G_format_easting(box.E, tmp1, G_projection());
    G_format_easting(box.W, tmp2, G_projection());
    sprintf(line, "              %c: %17s    %c: %17s",
            'E', tmp1, 'W', tmp2);
    printline(line);
    
    if (Vect_is_3d(Map)) {
        format_double(box.B, tmp1);
        format_double(box.T, tmp2);
        sprintf(line, "              %c: %17s    %c: %17s",
                'B', tmp1, 'T', tmp2);
        printline(line);
    }
    printline("");

    format_double(Vect_get_thresh(Map), tmp1);
    sprintf(line, "  %s: %s", _("Digitization threshold"), tmp1);
    printline(line);
    sprintf(line, "  %s:", _("Comment"));
    printline(line);
    sprintf(line, "    %s", Vect_get_comment(Map));
    printline(line);
    divider('+');
    fprintf(stdout, "\n");
}
