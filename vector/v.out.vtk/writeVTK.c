
 /***************************************************************************
 *
 * MODULE:     v.out.vtk  
 * AUTHOR(S):  Soeren Gebbert
 *
 * PURPOSE:    v.out.vtk: writes ASCII VTK file
 *             this module is based on v.out.ascii
 * COPYRIGHT:  (C) 2000 by the GRASS Development Team
 *
 *             This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "writeVTK.h"
#include "local_proto.h"


/*Prototype */
/*Formatted coordinates output */
static void write_point_coordinates(struct line_pnts *Points, int dp,
				    double scale, FILE * ascii);


/* ************************************************************************* */
/* This function writes the vtk points and coordinates ********************* */
/* ************************************************************************* */
int write_vtk_points(FILE * ascii, struct Map_info *Map, VTKInfo * info,
		     int *types, int typenum, int dp, double scale)
{
    int type, cur, i, k, centroid;
    int pointoffset = 0;
    int lineoffset = 0;
    int polygonoffset = 0;
    static struct line_pnts *Points;
    struct line_cats *Cats;

    Points = Vect_new_line_struct();	/* init line_pnts struct */
    Cats = Vect_new_cats_struct();

    G_message("Writing coordinates ...");

    /*For every available vector type */
    for (k = 0; k < typenum; k++) {
	/*POINT KERNEL CENTROID */
	if (types[k] == GV_POINT || types[k] == GV_KERNEL ||
	    types[k] == GV_CENTROID) {

	    /*Get the number of the points to generate */
	    info->typeinfo[types[k]]->pointoffset = pointoffset;

	    /*count the number of line_nodes and lines */
	    Vect_rewind(Map);
	    while (1) {
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    info->typeinfo[types[k]]->numpoints++;
		}
	    }

	    pointoffset += info->typeinfo[types[k]]->numpoints;

	    info->typeinfo[types[k]]->numvertices =
		info->typeinfo[types[k]]->numpoints;
	    info->maxnumvertices += info->typeinfo[types[k]]->numpoints;

	    info->maxnumpoints += info->typeinfo[types[k]]->numpoints;
	    /*
	     * printf("Points Type %i Number %i offset %i\n", types[k],
	     * info->typeinfo[types[k]]->numpoints,
	     * info->typeinfo[types[k]]->pointoffset);
	     */
	}
    }

    for (k = 0; k < typenum; k++) {
	/*LINE BOUNDARY */
	if (types[k] == GV_LINE || types[k] == GV_BOUNDARY) {

	    info->typeinfo[types[k]]->pointoffset = pointoffset;
	    info->typeinfo[types[k]]->lineoffset = lineoffset;

	    /*count the number of line_nodes and lines */
	    Vect_rewind(Map);
	    while (1) {
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    info->typeinfo[types[k]]->numpoints += Points->n_points;
		    info->typeinfo[types[k]]->numlines++;
		}
	    }
	    pointoffset += info->typeinfo[types[k]]->numpoints;
	    lineoffset += info->typeinfo[types[k]]->lineoffset;

	    info->maxnumpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumlinepoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumlines += info->typeinfo[types[k]]->numlines;
	    /*
	     * printf("Lines  Type %i Number %i offset %i\n", types[k],
	     * info->typeinfo[types[k]]->numlines,
	     * info->typeinfo[types[k]]->lineoffset);
	     */
	}
    }

    for (k = 0; k < typenum; k++) {
	/*FACE */
	if (types[k] == GV_FACE) {

	    info->typeinfo[types[k]]->pointoffset = pointoffset;
	    info->typeinfo[types[k]]->polygonoffset = polygonoffset;

	    /*count the number of line_nodes and lines */
	    Vect_rewind(Map);
	    while (1) {
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    info->typeinfo[types[k]]->numpoints += Points->n_points;
		    info->typeinfo[types[k]]->numpolygons++;
		}
	    }

	    pointoffset += info->typeinfo[types[k]]->numpoints;
	    polygonoffset += info->typeinfo[types[k]]->numpolygons;

	    info->maxnumpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumpolygonpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumpolygons += info->typeinfo[types[k]]->numpolygons;
	    /*
	     * printf("Polygons  Type %i Number %i offset %i\n", types[k],
	     * info->typeinfo[types[k]]->numpolygons,
	     * info->typeinfo[types[k]]->polygonoffset);
	     */
	}
    }

    for (k = 0; k < typenum; k++) {
	/*AREA */
	if (types[k] == GV_AREA) {

	    info->typeinfo[types[k]]->numpolygons = Vect_get_num_areas(Map);
	    info->typeinfo[types[k]]->pointoffset = pointoffset;
	    info->typeinfo[types[k]]->polygonoffset = polygonoffset;

	    /*Count the coordinate points */
	    Vect_rewind(Map);
	    for (i = 1; i <= info->typeinfo[types[k]]->numpolygons; i++) {
		centroid = Vect_get_area_centroid(Map, i);
		if (centroid > 0) {
		    Vect_read_line(Map, NULL, Cats, centroid);
		}
		Vect_get_area_points(Map, i, Points);
		info->typeinfo[types[k]]->numpoints += Points->n_points;
	    }

	    pointoffset += info->typeinfo[types[k]]->numpoints;
	    polygonoffset += info->typeinfo[types[k]]->numpolygons;

	    info->maxnumpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumpolygonpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumpolygons += info->typeinfo[types[k]]->numpolygons;
	    /*
	     * printf("Polygons  Type %i Number %i offset %i\n", types[k],
	     * info->typeinfo[types[k]]->numpolygons,
	     * info->typeinfo[types[k]]->polygonoffset);
	     */
	}
    }

    /*
     * printf("Maxnum points %i \n", info->maxnumpoints);
     * printf("Maxnum vertices %i \n", info->maxnumvertices);
     * printf("Maxnum lines %i \n", info->maxnumlines);
     * printf("Maxnum line points %i \n", info->maxnumlinepoints);
     * printf("Maxnum polygons %i \n", info->maxnumpolygons);
     * printf("Maxnum polygon points %i \n", info->maxnumpolygonpoints);
     */
    /*break if nothing to generate */
    if (info->maxnumpoints == 0)
	G_fatal_error(_("No coordinates to generate the output! Maybe an empty vector type chosen?"));

    /************************************************/
    /*Write the coordinates into the vtk ascii file */
    /************************************************/

    fprintf(ascii, "POINTS %i float\n", info->maxnumpoints);

    /*For every available vector type */
    for (k = 0; k < typenum; k++) {
	/*POINT KERNEL CENTROID */
	if (types[k] == GV_POINT || types[k] == GV_KERNEL ||
	    types[k] == GV_CENTROID) {
	    Vect_rewind(Map);

	    /*Write the coordinates */
	    cur = 0;
	    while (1) {
		if (cur <= info->typeinfo[types[k]]->numpoints)
		    G_percent(cur, info->typeinfo[types[k]]->numpoints, 2);
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    write_point_coordinates(Points, dp, scale, ascii);

		    if (Cats->n_cats == 0)
			info->typeinfo[types[k]]->generatedata = 0;	/*No data generation */
		}
		cur++;
	    }
	}
    }

    for (k = 0; k < typenum; k++) {
	/*LINE BOUNDARY */
	if (types[k] == GV_LINE || types[k] == GV_BOUNDARY) {
	    Vect_rewind(Map);
	    cur = 0;
	    while (1) {
		if (cur <= info->typeinfo[types[k]]->numlines)
		    G_percent(cur, info->typeinfo[types[k]]->numlines, 2);
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    write_point_coordinates(Points, dp, scale, ascii);
		}
		cur++;
	    }
	}
    }

    for (k = 0; k < typenum; k++) {
	/* FACE */
	if (types[k] == GV_FACE) {
	    Vect_rewind(Map);
	    cur = 0;
	    while (1) {
		if (cur <= info->typeinfo[types[k]]->numpolygons)
		    G_percent(cur, info->typeinfo[types[k]]->numpolygons, 2);
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    write_point_coordinates(Points, dp, scale, ascii);
		}
		cur++;
	    }
	}
    }

    for (k = 0; k < typenum; k++) {
	/* AREA */
	if (types[k] == GV_AREA) {
	    Vect_rewind(Map);
	    for (i = 1; i <= info->typeinfo[types[k]]->numpolygons; i++) {
		centroid = Vect_get_area_centroid(Map, i);
		if (centroid > 0) {
		    Vect_read_line(Map, NULL, Cats, centroid);
		}
		Vect_get_area_points(Map, i, Points);
		write_point_coordinates(Points, dp, scale, ascii);
	    }
	}
    }

    return 1;
}


/* ************************************************************************* */
/* This function writes the vtk cells ************************************** */
/* ************************************************************************* */
int write_vtk_cells(FILE * ascii, struct Map_info *Map, VTKInfo * info,
		    int *types, int typenum)
{
    int type, i, j, k, centroid;
    static struct line_pnts *Points;
    struct line_cats *Cats;

    /*The keywords may only be written once! */
    int vertkeyword = 1;
    int linekeyword = 1;
    int polykeyword = 1;

    G_message("Writing vtk cells ...");

    Points = Vect_new_line_struct();	/* init line_pnts struct */
    Cats = Vect_new_cats_struct();

    /*For every available vector type */
    for (k = 0; k < typenum; k++) {

	/*POINT KERNEL CENTROID */
	if (types[k] == GV_POINT || types[k] == GV_KERNEL ||
	    types[k] == GV_CENTROID) {
	    Vect_rewind(Map);

	    /*Write the vertices */
	    if (info->typeinfo[types[k]]->numpoints > 0) {
		if (vertkeyword) {
		    fprintf(ascii, "VERTICES %i %i\n", info->maxnumvertices,
			    info->maxnumvertices * 2);
		    vertkeyword = 0;
		}
		for (i = 0; i < info->typeinfo[types[k]]->numpoints; i++) {
		    fprintf(ascii, "1 %i\n",
			    i + info->typeinfo[types[k]]->pointoffset);
		}
		fprintf(ascii, "\n");
	    }
	}
    }
    for (k = 0; k < typenum; k++) {
	/*LINE BOUNDARY */
	if (types[k] == GV_LINE || types[k] == GV_BOUNDARY) {
	    Vect_rewind(Map);

	    if (info->maxnumlines > 0) {
		if (linekeyword) {
		    fprintf(ascii, "LINES %i %i\n", info->maxnumlines,
			    info->maxnumlinepoints + info->maxnumlines);
		    linekeyword = 0;
		}

		Vect_rewind(Map);
		i = 0;
		while (1) {

		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {

			/*Check for data generation */
			if (Cats->n_cats == 0)
			    info->typeinfo[types[k]]->generatedata = 0;	/*No data generation */

			fprintf(ascii, "%i", Points->n_points);
			while (Points->n_points--) {
			    fprintf(ascii, " %i",
				    i +
				    info->typeinfo[types[k]]->pointoffset);
			    i++;
			}
			fprintf(ascii, "\n");
		    }
		}
	    }
	}
    }
    for (k = 0; k < typenum; k++) {
	/*LINE BOUNDARY FACE */
	if (types[k] == GV_FACE) {
	    Vect_rewind(Map);

	    if (info->maxnumpolygons > 0) {
		if (polykeyword) {
		    fprintf(ascii, "POLYGONS %i %i\n",
			    info->maxnumpolygons,
			    info->maxnumpolygonpoints + info->maxnumpolygons);
		    polykeyword = 0;
		}

		Vect_rewind(Map);
		i = 0;
		while (1) {

		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {

			/*Check for data generation */
			if (Cats->n_cats == 0)
			    info->typeinfo[types[k]]->generatedata = 0;	/*No data generation */

			fprintf(ascii, "%i", Points->n_points);
			while (Points->n_points--) {
			    fprintf(ascii, " %i",
				    i +
				    info->typeinfo[types[k]]->pointoffset);
			    i++;
			}
			fprintf(ascii, "\n");
		    }
		}
	    }
	}
    }

    for (k = 0; k < typenum; k++) {
	/*AREA */
	if (types[k] == GV_AREA) {
	    Vect_rewind(Map);

	    if (info->maxnumpolygons > 0) {
		if (polykeyword) {
		    fprintf(ascii, "POLYGONS %i %i\n",
			    info->maxnumpolygons,
			    info->maxnumpolygonpoints + info->maxnumpolygons);
		    polykeyword = 0;
		}

		j = 0;
		for (i = 1; i <= info->typeinfo[types[k]]->numpolygons; i++) {
		    centroid = Vect_get_area_centroid(Map, i);
		    if (centroid > 0) {
			Vect_read_line(Map, NULL, Cats, centroid);
		    }
		    Vect_get_area_points(Map, i, Points);

		    /*Check for data generation */
		    if (Cats->n_cats == 0)
			info->typeinfo[types[k]]->generatedata = 0;	/*No data generation */

		    fprintf(ascii, "%i", Points->n_points);
		    while (Points->n_points--) {
			fprintf(ascii, " %i",
				j + info->typeinfo[types[k]]->pointoffset);
			j++;
		    }
		    fprintf(ascii, "\n");
		}

	    }
	}
    }

    return 1;
}

/* ************************************************************************* */
/* This function writes the categories as vtk cell data ******************** */
/* ************************************************************************* */
int write_vtk_cat_data(FILE * ascii, struct Map_info *Map, VTKInfo * info,
		       int layer, int *types, int typenum, int dp)
{
    int type, cat, i, k, centroid;
    static struct line_pnts *Points;
    struct line_cats *Cats;

    /*The keywords may only be written once! */
    int numcelldata =
	info->maxnumvertices + info->maxnumlines + info->maxnumpolygons;

    Points = Vect_new_line_struct();	/* init line_pnts struct */
    Cats = Vect_new_cats_struct();

    G_message("Writing category cell data ...");

    if (numcelldata > 0) {
	/*Write the pointdata */
	fprintf(ascii, "CELL_DATA %i\n", numcelldata);
	fprintf(ascii, "SCALARS cat_%s int 1\n", Map->name);
	fprintf(ascii, "LOOKUP_TABLE default\n");

	/*For every available vector type */
	for (k = 0; k < typenum; k++) {
	    /*POINT KERNEL CENTROID */
	    if (types[k] == GV_POINT || types[k] == GV_KERNEL ||
		types[k] == GV_CENTROID) {

		Vect_rewind(Map);

		while (1) {
		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {
			Vect_cat_get(Cats, layer, &cat);
			fprintf(ascii, " %d", cat);
		    }
		}
	    }
	}

	for (k = 0; k < typenum; k++) {
	    /*LINE BOUNDARY */
	    if (types[k] == GV_LINE || types[k] == GV_BOUNDARY) {
		Vect_rewind(Map);
		while (1) {
		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {
			Vect_cat_get(Cats, layer, &cat);
			fprintf(ascii, " %d", cat);
		    }
		}
	    }
	}

	for (k = 0; k < typenum; k++) {
	    /*FACE */
	    if (types[k] == GV_FACE) {
		Vect_rewind(Map);
		while (1) {
		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {
			Vect_cat_get(Cats, layer, &cat);
			fprintf(ascii, " %d", cat);
		    }
		}
	    }
	}

	for (k = 0; k < typenum; k++) {
	    /*AREA */
	    if (types[k] == GV_AREA) {
		Vect_rewind(Map);
		for (i = 1; i <= info->typeinfo[types[k]]->numpolygons; i++) {
		    centroid = Vect_get_area_centroid(Map, i);
		    if (centroid > 0) {
			Vect_read_line(Map, NULL, Cats, centroid);
		    }
		    Vect_cat_get(Cats, layer, &cat);
		    fprintf(ascii, " %d", cat);
		}
	    }
	}
	fprintf(ascii, "\n");
    }

    return 1;
}


/* 
   Reads the attribute field "name" for current cat and returns the value as a string 
   or NULL on error 

   Memory for the result string is allocated by this function and must be free'd by
   the caller.
 */
char *get_att(char *name, int cat, struct field_info *Fi, dbDriver * Driver,
	      int ncol)
{
    char buf[2000];
    int more;
    dbTable *Table;
    static dbString dbstring;
    dbColumn *Column;
    dbCursor cursor;
    char *retval;
    static int first = 1;

    if (first) {
	db_init_string(&dbstring);
	first = 0;
    }

    sprintf(buf, "SELECT %s FROM %s WHERE %s = %d", name, Fi->table, Fi->key, cat);

    db_set_string(&dbstring, buf);

    if (db_open_select_cursor(Driver, &dbstring, &cursor, DB_SEQUENTIAL) !=
	DB_OK) {
	G_fatal_error(_("Cannot select attributes for cat = %d"), cat);
    }

    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	G_fatal_error(_("Unable to fetch data from table"));

    Table = db_get_cursor_table(&cursor);

    Column = db_get_table_column(Table, 0);
    if (!strcmp(name, db_get_column_name(Column))) {
	db_convert_column_value_to_string(Column, &dbstring);
	retval =
	    G_malloc(sizeof(char) *
		     (strlen(db_get_string(&dbstring)) + 1));
	retval = G_store(db_get_string(&dbstring));
	db_close_cursor(&cursor);
	return (retval);
    }

    db_close_cursor(&cursor);
    return (NULL);
}


/* ************************************************************************* */
/* This function writes numerical attribute table fields as VTK scalars **** */
/* ************************************************************************* */
int write_vtk_db_data(FILE * ascii, struct Map_info *Map, VTKInfo * info,
		      int layer, int *types, int typenum, int dp)
{
    int type, cat, i, k, centroid;
    struct line_cats *Cats;

    /*The keywords may only be written once! */
    int numcelldata =
	info->maxnumvertices + info->maxnumlines + info->maxnumpolygons;
    /* attribute table info */
    int ncol = 0, colsqltype, colctype, num_atts, cur_att, progress;
    struct field_info *Fi = NULL;
    dbDriver *Driver = NULL;
    dbHandle handle;
    dbTable *Table;
    dbString dbstring;
    dbColumn *Column;
    char *valbuf;

    if (layer < 1) {
	G_warning(_("Cannot export attribute table fields for layer < 1. Skipping export"));
	return 1;
    }

    /* attempt to open attribute table for selected layer */
    db_init_string(&dbstring);
    Fi = Vect_get_field(Map, layer);
    if (Fi == NULL) {
	G_fatal_error(_("No attribute table found"));
    }
    Driver = db_start_driver(Fi->driver);
    if (Driver == NULL)
	G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);

    db_init_handle(&handle);
    db_set_handle(&handle, Fi->database, NULL);
    if (db_open_database(Driver, &handle) != DB_OK)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);
    db_set_error_handler_driver(Driver);

    db_set_string(&dbstring, Fi->table);
    if (db_describe_table(Driver, &dbstring, &Table) != DB_OK)
	G_fatal_error(_("Unable to describe table <%s>"), Fi->table);

    /* analyse field structure */
    ncol = db_get_table_number_of_columns(Table);
    num_atts = 0;
    for (i = 0; i < ncol; i++) {
	Column = db_get_table_column(Table, i);
	colsqltype = db_get_column_sqltype(Column);
	colctype = db_sqltype_to_Ctype(colsqltype);
	if ((colctype == DB_C_TYPE_INT) || (colctype == DB_C_TYPE_DOUBLE)) {
	    /* we don't want to export the category field twice */
	    if (strcmp(db_get_column_name(Column), "cat")) {
		num_atts++;
		/* fprintf ( stderr, "%i: %s\n", num_atts, db_get_column_name(Column) ); */
	    }
	}
    }
    if (num_atts < 1) {
	G_warning(_("No numerical attributes found. Skipping export"));
	db_close_database(Driver);
	db_shutdown_driver(Driver);
	return 1;
    }

    Cats = Vect_new_cats_struct();

    G_message("Writing %i scalar variables as cell data ...", num_atts);

    progress = 0;
    for (cur_att = 0; cur_att < ncol; cur_att++) {

	if (numcelldata > 0) {
	    /*Write the pointdata */
	    Column = db_get_table_column(Table, cur_att);
	    colsqltype = db_get_column_sqltype(Column);
	    colctype = db_sqltype_to_Ctype(colsqltype);

	    if ((strcmp("cat", db_get_column_name(Column))) &&
		((colctype == DB_C_TYPE_INT) ||
		 (colctype == DB_C_TYPE_DOUBLE))) {

		if (colctype == DB_C_TYPE_INT) {
		    /* G_message("  Writing integer scalar %s", db_get_column_name(Column) ); */
		    fprintf(ascii, "SCALARS %s int 1\n",
			    db_get_column_name(Column));
		}
		if (colctype == DB_C_TYPE_DOUBLE) {
		    /* *G_message("  Writing double scalar %s", db_get_column_name(Column) ); */
		    fprintf(ascii, "SCALARS %s double 1\n",
			    db_get_column_name(Column));
		}

		fprintf(ascii, "LOOKUP_TABLE default\n");
		progress++;

		/*For every available vector type */
		for (k = 0; k < typenum; k++) {
		    /*POINT KERNEL CENTROID */
		    if (types[k] == GV_POINT || types[k] == GV_KERNEL ||
			types[k] == GV_CENTROID) {

			Vect_rewind(Map);

			while (1) {
			    if (-1 ==
				(type =
				 Vect_read_next_line(Map, NULL, Cats)))
				break;
			    if (type == -2)	/* EOF */
				break;
			    if (type == types[k]) {
				Vect_cat_get(Cats, layer, &cat);
				valbuf =
				    get_att((char *)
					    db_get_column_name(Column), cat,
					    Fi, Driver, ncol);
				if (valbuf == NULL) {
				    G_fatal_error(_("Error reading value of attribute '%s'"),
						  db_get_column_name(Column));
				}
				/* DEBUG 
				   fprintf ( stderr, "%s (%i) = %s\n", db_get_column_name(Column), cat, valbuf );
				 */
				fprintf(ascii, " %s", valbuf);
				G_free(valbuf);
			    }
			}
		    }
		}

		for (k = 0; k < typenum; k++) {
		    /*LINE BOUNDARY */
		    if (types[k] == GV_LINE || types[k] == GV_BOUNDARY) {
			Vect_rewind(Map);
			while (1) {
			    if (-1 ==
				(type =
				 Vect_read_next_line(Map, NULL, Cats)))
				break;
			    if (type == -2)	/* EOF */
				break;
			    if (type == types[k]) {
				Vect_cat_get(Cats, layer, &cat);
				valbuf =
				    get_att((char *)
					    db_get_column_name(Column), cat,
					    Fi, Driver, ncol);
				if (valbuf == NULL) {
				    G_fatal_error(_("Error reading value of attribute '%s'"),
						  db_get_column_name(Column));
				}
				/* DEBUG 
				   fprintf ( stderr, "%s (%i) = %s\n", db_get_column_name(Column), cat, valbuf );
				 */
				fprintf(ascii, " %s", valbuf);
				G_free(valbuf);
			    }
			}
		    }
		}

		for (k = 0; k < typenum; k++) {
		    /*FACE */
		    if (types[k] == GV_FACE) {
			Vect_rewind(Map);
			while (1) {
			    if (-1 ==
				(type =
				 Vect_read_next_line(Map, NULL, Cats)))
				break;
			    if (type == -2)	/* EOF */
				break;
			    if (type == types[k]) {
				Vect_cat_get(Cats, layer, &cat);
				valbuf =
				    get_att((char *)
					    db_get_column_name(Column), cat,
					    Fi, Driver, ncol);
				if (valbuf == NULL) {
				    G_fatal_error(_("Error reading value of attribute '%s'"),
						  db_get_column_name(Column));
				}
				/* DEBUG 
				   fprintf ( stderr, "%s (%i) = %s\n", db_get_column_name(Column), cat, valbuf );
				 */
				fprintf(ascii, " %s", valbuf);
				G_free(valbuf);
			    }
			}
		    }
		}

		for (k = 0; k < typenum; k++) {
		    /*AREA */
		    if (types[k] == GV_AREA) {
			Vect_rewind(Map);
			for (i = 1;
			     i <= info->typeinfo[types[k]]->numpolygons;
			     i++) {
			    centroid = Vect_get_area_centroid(Map, i);
			    if (centroid > 0) {
				Vect_read_line(Map, NULL, Cats, centroid);
			    }
			    Vect_cat_get(Cats, layer, &cat);
			    valbuf =
				get_att((char *)db_get_column_name(Column),
					cat, Fi, Driver, ncol);
			    if (valbuf == NULL) {
				G_fatal_error(_("Error reading value of attribute '%s'"),
					      db_get_column_name(Column));
			    }
			    /* DEBUG 
			       fprintf ( stderr, "%s (%i) = %s\n", db_get_column_name(Column), cat, valbuf );
			     */
			    fprintf(ascii, " %s", valbuf);
			    G_free(valbuf);
			}
		    }
		}
		fprintf(ascii, "\n");
	    }			/* END (do for all scalars != cat */
	}
    }				/* END (step through all numerical attributes) */
    fprintf(stdout, "\n");
    fflush(stdout);

    db_close_database(Driver);
    db_shutdown_driver(Driver);

    return 1;
}


/* ************************************************************************* */
/* This function writes attribute table fields as VTK labels            **** */
/* ************************************************************************* */
int write_vtk_db_labels(FILE * ascii, struct Map_info *Map, VTKInfo * info,
			int layer, int *types, int typenum, int dp)
{
    return 1;
}

/* ************************************************************************* */
/* This function writes the point coordinates and the geometric feature **** */
/* ************************************************************************* */
int write_vtk(FILE * ascii, struct Map_info *Map, int layer, int *types,
	      int typenum, int dp, double scale, int numatts, int labels)
{
    VTKInfo *info;
    VTKTypeInfo **typeinfo;
    int i;
    int infonum =
	GV_POINT + GV_KERNEL + GV_CENTROID + GV_LINE + GV_BOUNDARY + GV_FACE +
	GV_AREA;

    /*Initiate the typeinfo structure for every supported type */
    typeinfo = (VTKTypeInfo **) calloc(infonum, sizeof(VTKTypeInfo *));
    for (i = 0; i < infonum; i++) {
	typeinfo[i] = (VTKTypeInfo *) calloc(1, sizeof(VTKTypeInfo));
	typeinfo[i]->numpoints = 0;
	typeinfo[i]->pointoffset = 0;
	typeinfo[i]->numvertices = 0;
	typeinfo[i]->verticesoffset = 0;
	typeinfo[i]->numlines = 0;
	typeinfo[i]->lineoffset = 0;
	typeinfo[i]->numpolygons = 0;
	typeinfo[i]->polygonoffset = 0;
	typeinfo[i]->generatedata = 1;
    }

    /*Initiate the info structure */
    info = (VTKInfo *) calloc(infonum, sizeof(VTKInfo));
    info->maxnumpoints = 0;
    info->maxnumvertices = 0;
    info->maxnumlines = 0;
    info->maxnumlinepoints = 0;
    info->maxnumpolygons = 0;
    info->maxnumpolygonpoints = 0;
    info->typeinfo = typeinfo;

    /*1. write the points */
    write_vtk_points(ascii, Map, info, types, typenum, dp, scale);

    /*2. write the cells */
    write_vtk_cells(ascii, Map, info, types, typenum);

    /*3. write the cat data */
    write_vtk_cat_data(ascii, Map, info, layer, types, typenum, dp);

    /*4. write the DB data: numerical attributes */
    if (numatts) {
	write_vtk_db_data(ascii, Map, info, layer, types, typenum, dp);
    }

    /*5. Write labels (not yet supported) 
       if ( labels ) {
       write_vtk_db_labels(ascii, Map, info, layer, types, typenum, dp);
       }
     */

    /*Release the memory */
    for (i = 0; i < infonum; i++) {
	free(typeinfo[i]);
    }
    free(typeinfo);
    free(info);

    return 1;
}

/* ************************************************************************* */
/* This function writes the point coordinates ****************************** */
/* ************************************************************************* */
void write_point_coordinates(struct line_pnts *Points, int dp, double scale,
			     FILE * ascii)
{
    char *xstring = NULL, *ystring = NULL, *zstring = NULL;
    double *xptr, *yptr, *zptr;

    xptr = Points->x;
    yptr = Points->y;
    zptr = Points->z;

    while (Points->n_points--) {
	G_asprintf(&xstring, "%.*f", dp, *xptr++ - x_extent);
	G_trim_decimal(xstring);
	G_asprintf(&ystring, "%.*f", dp, *yptr++ - y_extent);
	G_trim_decimal(ystring);
	G_asprintf(&zstring, "%.*f", dp, scale * (*zptr++));
	G_trim_decimal(zstring);
	fprintf(ascii, "%s %s %s \n", xstring, ystring, zstring);
    }

    return;
}
