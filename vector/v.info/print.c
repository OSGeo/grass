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
    char tmp1[100], tmp2[100];
    
    struct bound_box box;

    Vect_get_map_box(Map, &box);
    G_format_northing(box.N, tmp1, Vect_get_proj(Map));
    G_format_northing(box.S, tmp2, Vect_get_proj(Map));
    fprintf(stdout, "north=%s\n", tmp1);
    fprintf(stdout, "south=%s\n", tmp2);
    
    G_format_easting(box.E, tmp1, Vect_get_proj(Map));
    G_format_easting(box.W, tmp2, Vect_get_proj(Map));
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
    

    fprintf(stdout, "nodes=%ld\n",
	    Vect_get_num_nodes(Map));
    fflush(stdout);

    fprintf(stdout, "points=%ld\n",
	    Vect_get_num_primitives(Map, GV_POINT));
    fflush(stdout);
    
    fprintf(stdout, "lines=%ld\n",
	    Vect_get_num_primitives(Map, GV_LINE));
    fflush(stdout);
    
    fprintf(stdout, "boundaries=%ld\n",
	    Vect_get_num_primitives(Map, GV_BOUNDARY));
    fflush(stdout);
    
    fprintf(stdout, "centroids=%ld\n",
	    Vect_get_num_primitives(Map, GV_CENTROID));
    fflush(stdout);
    
    fprintf(stdout, "areas=%ld\n", Vect_get_num_areas(Map));
    fflush(stdout);
    
    fprintf(stdout, "islands=%ld\n",
	    Vect_get_num_islands(Map));
    fflush(stdout);
    
    if (with_z) {
	fprintf(stdout, "faces=%ld\n",
		Vect_get_num_primitives(Map, GV_FACE));
	fflush(stdout);
	
	fprintf(stdout, "kernels=%ld\n",
		Vect_get_num_primitives(Map, GV_KERNEL));
	fflush(stdout);
	
	fprintf(stdout, "volumes=%ld\n",
		Vect_get_num_primitives(Map, GV_VOLUME));
	fflush(stdout);
	
	fprintf(stdout, "holes=%ld\n",
		Vect_get_num_holes(Map));
	fflush(stdout);
    }

    fprintf(stdout, "primitives=%ld\n", nprimitives);
    fflush(stdout);
    
    fprintf(stdout, "map3d=%d\n", Vect_is_3d(Map));
    fflush(stdout);
}

void print_columns(const struct Map_info *Map, const char *input_opt, const char *field_opt)
{
    int num_dblinks, field, col, ncols;

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

    field = atoi(field_opt);
    G_message(_("Displaying column types/names for database connection of layer %d:"),
	      field);

    if ((fi = Vect_get_field(Map, field)) == NULL)
	G_fatal_error(_("Database connection not defined for layer %d"),
		      field);
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

void print_info(const struct Map_info *Map)
{
    int i;
    char line[100];
    char tmp1[100], tmp2[100];

    struct bound_box box;
    
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
    sprintf(line, "%-17s%s", _("Map format:"),
	    Vect_maptype_info(Map));
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
    
    divider('|');
    
    sprintf(line, "  %s: %s (%s: %i)",
	    _("Type of map"), _("vector"), _("level"), Vect_level(Map));
    
    printline(line);
    
    if (Vect_level(Map) > 0) {
	printline("");
	sprintf(line,
		"  %-24s%-9ld       %-22s%-9ld",
		_("Number of points:"), 
		Vect_get_num_primitives(Map, GV_POINT),
		_("Number of centroids:"),
		Vect_get_num_primitives(Map, GV_CENTROID));
	printline(line);
	sprintf(line,
		"  %-24s%-9ld       %-22s%-9ld",
		_("Number of lines:"),
		Vect_get_num_primitives(Map, GV_LINE),
		_("Number of boundaries:"),
		Vect_get_num_primitives(Map, GV_BOUNDARY));
	printline(line);
	sprintf(line,
		"  %-24s%-9ld       %-22s%-9ld",
		_("Number of areas:"),
		Vect_get_num_areas(Map),
		_("Number of islands:"),
		Vect_get_num_islands(Map));
	printline(line);
	if (Vect_is_3d(Map)) {
	    sprintf(line,
		    "  %-24s%-9ld       %-22s%-9ld",
		    _("Number of faces:"),
		    Vect_get_num_primitives(Map, GV_FACE),
		    _("Number of kernels:"),
		    Vect_get_num_primitives(Map, GV_KERNEL));
	    printline(line);
	    sprintf(line,
		    "  %-24s%-9ld       %-22s%-9ld",
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
    if (G_projection() == PROJECTION_UTM)
	sprintf(line, "  %s: %s (%s %d)",
		_("Projection:"),
		Vect_get_proj_name(Map),
		_("zone"), Vect_get_zone(Map));
    else
	sprintf(line, "  %s: %s",
		_("Projection"),
		Vect_get_proj_name(Map));

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
