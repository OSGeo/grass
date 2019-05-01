#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "what.h"

/* TODO: remove leftover from interactive querying */
static int nlines = 50;

#define WDTH 5
#define SEP "------------------------------------------------------------------"

static void F_generate(const char *drvname, const char *dbname,
		       const char *tblname, const char *key, int keyval,
		       int output, char **form)
{
    int col, ncols, sqltype, more;
    char buf[5000];
    const char *colname;
    char *formbuf;
    dbString sql, html, str;
    dbDriver *driver;
    dbHandle handle;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;

    G_debug(2,
	    "F_generate(): drvname = '%s', dbname = '%s', tblname = '%s', key = '%s', keyval = %d",
	    drvname, dbname, tblname, key, keyval);

    db_init_string(&sql);
    db_init_string(&html);	/* here is the result stored */
    db_init_string(&str);

    G_debug(2, "Open driver");
    driver = db_start_driver(drvname);
    if (!driver)
	G_fatal_error(_("Cannot open driver"));

    G_debug(2, "Driver opened");

    db_init_handle(&handle);
    db_set_handle(&handle, dbname, NULL);
    G_debug(2, "Open database");
    if (db_open_database(driver, &handle) != DB_OK)
	G_fatal_error(_("Cannot open database"));
    db_set_error_handler_driver(driver);

    G_debug(2, "Database opened");

    /* TODO: test if table exist first, but this should be tested by
     * application before F_generate() is called, because it may be correct
     * (connection defined in DB but table does not exist) */

    sprintf(buf, "select * from %s where %s = %d", tblname, key, keyval);
    G_debug(2, "%s", buf);
    db_set_string(&sql, buf);
    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK)
	G_fatal_error(_("Cannot open select cursor"));

    G_debug(2, "Select Cursor opened");

    table = db_get_cursor_table(&cursor);

    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	G_fatal_error(_("Cannot fetch next record"));

    if (!more) {
	G_verbose_message(_("No database record"));
	*form = G_store("No record selected.");
    }
    else {
	ncols = db_get_table_number_of_columns(table);

	/* Start form */
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    sqltype = db_get_column_sqltype(column);
	    value = db_get_column_value(column);
	    db_convert_value_to_string(value, sqltype, &str);
	    colname = db_get_column_name(column);

	    G_debug(2, "%s: %s", colname, db_get_string(&str));
	    switch (output) {
	    case OUTPUT_JSON:
		formbuf = G_str_replace(db_get_string(&str), "\\", "\\\\");
		formbuf = G_str_replace(formbuf, "\"", "\\\"");
		sprintf(buf, "%s\"%s\": \"%s\"", col == 0 ? "" : ",\n",
			colname, formbuf);
		G_free(formbuf);
		break;
	    case OUTPUT_SCRIPT:
		sprintf(buf, "%s=%s\n", colname, db_get_string(&str));
		break;
	    default:
		sprintf(buf, "%s : %s\n", colname, db_get_string(&str));
	    }
	    db_append_string(&html, buf);
	}
    }
    G_debug(2, "FORM STRING:%s", db_get_string(&html));

    db_close_cursor(&cursor);
    db_close_database(driver);
    db_shutdown_driver(driver);

    *form = G_store(db_get_string(&html));

    db_free_string(&sql);
    db_free_string(&html);
    db_free_string(&str);
}

/**
   \brief Creates bounding box (polygon)

   Based on center point; size (2 * maxdist)

   \param[in] east,north coordinates of center
   \param[in] maxdist size of bounding box
   \param[out] result bounding box

   \return
*/
void coord2bbox(double east, double north, double maxdist,
                struct line_pnts *box)
{
    /* TODO: 3D */
    Vect_reset_line(box);

    Vect_append_point(box, east - maxdist, north - maxdist, 0);
    Vect_append_point(box, east + maxdist, north - maxdist, 0);
    Vect_append_point(box, east + maxdist, north + maxdist, 0);
    Vect_append_point(box, east - maxdist, north + maxdist, 0);
    Vect_append_point(box, box->x[0], box->y[0], box->z[0]);

    return;
}

void write_cats(struct Map_info *Map, int field, struct line_cats *Cats,
		int showextra, int output)
{
    int i, j;
    char *formbuf1;
    char *formbuf2;

    if (Cats->n_cats == 0)
	return;

    if (output == OUTPUT_JSON)
	fprintf(stdout, ",\n\"Categories\": [");

    j = 0;
    for (i = 0; i < Cats->n_cats; i++) {
	struct field_info *Fi;

	if (field == -1 || Cats->field[i] == field) {
	    j++;
	    G_debug(2, "field = %d  category = %d\n", Cats->field[i],
		    Cats->cat[i]);
	    switch (output) {
	    case OUTPUT_SCRIPT:
		fprintf(stdout, "Layer=%d\nCategory=%d\n", Cats->field[i],
			Cats->cat[i]);
		break;
	    case OUTPUT_JSON:
		fprintf(stdout, "%s\n{\"Layer\": %d, \"Category\": %d",
			j == 1 ? "": ",", Cats->field[i], Cats->cat[i]);
		break;
	    default:
		fprintf(stdout, _("Layer: %d\nCategory: %d\n"), Cats->field[i],
			Cats->cat[i]);
		break;
	    }
	    Fi = Vect_get_field(Map, Cats->field[i]);
	    if (Fi != NULL && showextra) {
		char *form;

		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout,
			    "Driver=%s\nDatabase=%s\nTable=%s\nKey_column=%s\n",
			    Fi->driver, Fi->database, Fi->table, Fi->key);
		    break;
		case OUTPUT_JSON:
			/* escape backslash to create valid JSON */
		    formbuf2 = G_str_replace(Fi->database, "\\", "\\\\");
		    fprintf(stdout,
			    ",\n\"Driver\": \"%s\",\n\"Database\": \"%s\",\n\"Table\": \"%s\",\n\"Key_column\": \"%s\"",
			    Fi->driver, formbuf2, Fi->table, Fi->key);
		    G_free(formbuf2);
		    break;
		default:
		    fprintf(stdout,
			    _("\nDriver: %s\nDatabase: %s\nTable: %s\nKey column: %s\n"),
			    Fi->driver, Fi->database, Fi->table, Fi->key);
		    break;
		}
		F_generate(Fi->driver, Fi->database, Fi->table,
			   Fi->key, Cats->cat[i], output, &form);

		switch (output) {
		case OUTPUT_SCRIPT:
		    formbuf1 = G_str_replace(form, " ", "_");
		    fprintf(stdout, "%s", formbuf1);
		    G_free(formbuf1);
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, ",\n\"Attributes\": {%s}", form);
		    break;
		default:
		    fprintf(stdout, "%s", form);
		    break;
		}
		G_free(form);
		G_free(Fi);
	    }
	    if (output == OUTPUT_JSON)
		    fprintf(stdout, "}"); /* for cat */
	}
    }
    if (output == OUTPUT_JSON)
	fprintf(stdout, "]"); /* for list of cats */
}

void what(struct Map_info *Map, int nvects, char **vect, double east,
	  double north, double maxdist, int qtype, int topo, int showextra,
	  int output, int multiple, int *field)
{
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct ilist *lineList, *areaList;
    dbString html;
    struct line_pnts *box;
    double sqm_to_sqft;
    char buf[1000], *str;
    int i, j;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    lineList = Vect_new_list();
    areaList = Vect_new_list();
    db_init_string(&html);

    if (multiple) {
	box = Vect_new_line_struct();
	coord2bbox(east, north, maxdist, box);
    }

    /* always use plain feet not US survey ft */
    /* if you really want USfeet, try G_database_units_to_meters_factor()
       here, but then watch that sq_miles is not affected too */
    sqm_to_sqft = 1 / ( 0.0254 * 0.0254 * 12 * 12 );


    for (i = 0; i < nvects; i++) {
	int nfeats;
	int getz;
	double z;
	int first;

	Vect_reset_cats(Cats);
	Vect_reset_line(Points);
	Vect_reset_list(lineList);
	Vect_reset_list(areaList);

	if (multiple) {
	    int type;

	    type = ((GV_POINTS | GV_LINE | GV_BOUNDARY | GV_FACE) & qtype);
	    if (type)
		Vect_select_lines_by_polygon(&Map[i], box, 0, NULL, type,
					     lineList);
	    Vect_select_areas_by_polygon(&Map[i], box, 0, NULL, areaList);

	    G_debug(2, "num lines = %d, num areas = %d", lineList->n_values,
		    areaList->n_values);
	}
	else {
	    int line, area;
	    int type;

	    /* Try to find point first and only if no one was found try lines,
	     * otherwise point on line could not be selected and similarly for
	     * areas */
	    line = area = 0;
	    type = (GV_POINTS & qtype);
	    if (type)
		line = Vect_find_line(&Map[i], east, north, 0.0, type, maxdist,
				      0, 0);

	    type = ((GV_LINE | GV_BOUNDARY | GV_FACE) & qtype);
	    if (line == 0 && type)
		line = Vect_find_line(&Map[i], east, north, 0.0, type, maxdist,
				      0, 0);

	    if (line == 0 && (qtype & GV_AREA))
		area = Vect_find_area(&Map[i], east, north);

	    if (line > 0)
		Vect_list_append(lineList, line);
	    if (area > 0)
		Vect_list_append(areaList, area);

	    G_debug(2, "line = %d area = %d", line, area);
	}
	if (areaList->n_values > 0)
	    getz = Vect_tin_get_z(&Map[i], east, north, &z, NULL, NULL);
	else {
	    getz = 0;
	    z = 0;
	}
	nfeats = lineList->n_values + areaList->n_values;

	if (!i) {
	    char east_buf[40], north_buf[40];

	    G_format_easting(east, east_buf, G_projection());
	    G_format_northing(north, north_buf, G_projection());

	    if (nfeats > 0 || G_verbose() >= G_verbose_std()) {
		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout, "East=%s\nNorth=%s\n", east_buf,
			    north_buf);
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, "{\"Coordinates\": {\"East\": \"%s\", \"North\": \"%s\"}",
			    east_buf, north_buf);
		    break;
		default:
		    fprintf(stdout, "East: %s\nNorth: %s\n", east_buf,
			    north_buf);
		    break;
		}
	    }
	    nlines++;
	}

	strcpy(buf, vect[i]);
	if ((str = strchr(buf, '@')))
	    *str = 0;

	switch (output) {
	case OUTPUT_SCRIPT:
	    fprintf(stdout, "\nMap=%s\nMapset=%s\n", Map[i].name,
		    Map[i].mapset);
	    break;
	case OUTPUT_JSON:
	    if (!i) {
		fprintf(stdout, "%s\"Maps\": [",
			nfeats > 0 || G_verbose() >= G_verbose_std() ?
			",\n" : "{");
	    }
	    else
		fprintf(stdout, ",");
	    fprintf(stdout, "\n{\"Map\": \"%s\",\n\"Mapset\": \"%s\"",
		    Map[i].name, Map[i].mapset);
	    break;
	default:
	    fprintf(stdout, "%s\nMap: %s \nMapset: %s\n", SEP, Map[i].name,
		    Map[i].mapset);
	    break;
	}

	nlines++;

	if (nfeats == 0) {
	    switch (output) {
	    case OUTPUT_JSON:
		fprintf(stdout, "}\n");
		break;
	    case OUTPUT_TEXT:
		fprintf(stdout, _("Nothing found.\n"));
		break;
	    }
	    nlines++;
	    continue;
	}

	if (multiple && output == OUTPUT_JSON)
	    fprintf(stdout, ",\n\"Features\": [");

	first = 1;

	for (j = 0; j < lineList->n_values; j++) {
	    int line, type;
	    double l;

	    line = lineList->value[j];
	    type = Vect_read_line(&Map[i], Points, Cats, line);
	    l = 0;

	    if (field[i] != -1 && !Vect_cat_get(Cats, field[i], NULL)) {
		if (output == OUTPUT_JSON && !multiple) {
		    fprintf(stdout, "}\n");
		}
		continue;
	    }

	    if (multiple) {
		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout, "\n");
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, "%s\n{", first ? "" : ",");
		    break;
		default:
		    fprintf(stdout, "%s\n", SEP);
		    break;
		}
	    }
	    first = 0;

	    switch (type) {
	    case GV_POINT:
		sprintf(buf, "Point");
		break;
	    case GV_LINE:
		sprintf(buf, "Line");
		break;
	    case GV_BOUNDARY:
		sprintf(buf, "Boundary");
		break;
	    case GV_FACE:
		sprintf(buf, "Face");
		break;
	    case GV_CENTROID:
		sprintf(buf, "Centroid");
		break;
	    default:
		sprintf(buf, "Unknown");
	    }
	    if (type & GV_LINES) {
		if (G_projection() == 3)
		    l = Vect_line_geodesic_length(Points);
		else
		    l = Vect_line_length(Points);
	    }


	    if (topo) {
		int n, node[2], nnodes, nnlines, nli, nodeline, left, right;
		float angle;

		if (type & GV_BOUNDARY)
		    Vect_get_line_areas(&(Map[i]), line, &left, &right);
		else
		    left = right = 0;
		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout, "Feature_max_distance=%f\n", maxdist);
		    fprintf(stdout,
			    "Id=%d\nType=%s\nLeft=%d\nRight=%d\n",
			    line, buf, left, right);
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, "%s\"Feature_max_distance\": %f",
			    multiple ? "" : ",\n", maxdist);
		    fprintf(stdout,
			    ",\n\"Id\": %d,\n\"Type\": \"%s\",\n\"Left\": %d,\n\"Right\": %d",
			    line, buf, left, right);
		    break;
		default:
		    fprintf(stdout, "Looking for features within: %f\n",
			    maxdist);
		    fprintf(stdout,
			    _("Id: %d\nType: %s\nLeft: %d\nRight: %d\n"),
			    line, buf, left, right);
		    break;
		}
		if (type & GV_LINES) {
		    nnodes = 2;
		    fprintf(stdout, _("Length: %f\n"), l);
		}
		else {		/* points */
		    nnodes = 0;
		    if (output != OUTPUT_SCRIPT)
			fprintf(stdout, "\n");
		}

		if (nnodes > 0)
		    Vect_get_line_nodes(&(Map[i]), line, &node[0], &node[1]);

		for (n = 0; n < nnodes; n++) {
		    double nx, ny, nz;

		    nnlines = Vect_get_node_n_lines(&(Map[i]), node[n]);

		    Vect_get_node_coor(&(Map[i]), node[n], &nx, &ny, &nz);
		    switch (output) {
		    case OUTPUT_SCRIPT:
			fprintf(stdout,
				_("Node[%d]=%d\nNumber_lines=%d\nCoordinates=%.6f,%.6f,%.6f\n"),
				n, node[n], nnlines, nx, ny, nz);
			break;
		    case OUTPUT_JSON:
			fprintf(stdout,
				_(",\n\"Node[%d]\": %d,\n\"Number_lines\": %d,\n\"Coordinates\": %.6f,%.6f,%.6f"),
				n, node[n], nnlines, nx, ny, nz);
			break;
		    default:
			fprintf(stdout,
				_("Node[%d]: %d\nNumber of lines: %d\nCoordinates: %.6f, %.6f, %.6f\n"),
				n, node[n], nnlines, nx, ny, nz);
			break;
		    }

		    for (nli = 0; nli < nnlines; nli++) {
			nodeline =
			    Vect_get_node_line(&(Map[i]), node[n], nli);
			angle =
			    Vect_get_node_line_angle(&(Map[i]), node[n], nli);
			switch (output) {
			case OUTPUT_SCRIPT:
			    fprintf(stdout, "Id=%5d\nAngle=%.8f\n",
				    nodeline, angle);
			    break;
			case OUTPUT_JSON:
			    fprintf(stdout, ",\n\"Id\": %5d,\n\"Angle\": %.8f",
				    nodeline, angle);
			    break;
			default:
			    fprintf(stdout, _("Id: %5d\nAngle: %.8f\n"),
				    nodeline, angle);
			    break;
			}
		    }
		}
	    }
	    else {
		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout, "Type=%s\n", buf);
		    fprintf(stdout, "Id=%d\n", line);
		    if (type & GV_LINES)
			fprintf(stdout, "Length=%f\n", l);
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, "%s\"Type\": \"%s\"", multiple ? "" : ",\n",
			    buf);
		    fprintf(stdout, ",\n\"Id\": %d", line);
		    if (type & GV_LINES)
			fprintf(stdout, ",\n\"Length\": %f", l);
		    break;
		default:
		    fprintf(stdout, _("Type: %s\n"), buf);
		    fprintf(stdout, _("Id: %d\n"), line);
		    if (type & GV_LINES)
			fprintf(stdout, _("Length: %f\n"), l);
		    break;
		}
	    }

	    /* Height */
	    if (Vect_is_3d(&(Map[i]))) {
		double min, max;

		if (type & GV_POINTS) {
		    switch (output) {
		    case OUTPUT_SCRIPT:
			fprintf(stdout, "Point_height=%f\n", Points->z[0]);
			break;
		    case OUTPUT_JSON:
			fprintf(stdout, ",\n\"Point_height\": %f",
				Points->z[0]);
			break;
		    default:
			fprintf(stdout, _("Point height: %f\n"),
				Points->z[0]);
			break;
		    }
		}
		else if (type & GV_LINES) {
		    int k;

		    min = max = Points->z[0];
		    for (k = 1; k < Points->n_points; k++) {
			if (Points->z[k] < min)
			    min = Points->z[k];
			if (Points->z[k] > max)
			    max = Points->z[k];
		    }
		    if (min == max) {
			switch (output) {
			case OUTPUT_SCRIPT:
			    fprintf(stdout, "Line_height=%f\n", min);
			    break;
			case OUTPUT_JSON:
			    fprintf(stdout, ",\n\"Line_height\": %f", min);
			    break;
			default:
			    fprintf(stdout, _("Line height: %f\n"), min);
			    break;
			}
		    }
		    else {
			switch (output) {
			case OUTPUT_SCRIPT:
			    fprintf(stdout,
				    "Line_height_min=%f\nLine_height_max=%f\n",
				    min, max);
			    break;
			case OUTPUT_JSON:
			    fprintf(stdout,
				    ",\n\"Line_height_min\": %f,\n\"Line_height_max\": %f",
				    min, max);
			    break;
			default:
			    fprintf(stdout,
				    _("Line height min: %f\nLine height max: %f\n"),
				    min, max);
			    break;
			}
		    }
		}
	    }			/* if height */

	    write_cats(&Map[i], field[i], Cats, showextra, output);

	    if (output == OUTPUT_JSON && multiple)
		fprintf(stdout, "}");
	}			/* for lineList */

	for (j = 0; j < areaList->n_values; j++) {
	    int area, centroid;
	    double sq_meters, acres, hectares, sq_miles;

	    if (multiple) {
		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout, "\n");
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, "%s\n{", first ? "" : ",");
		    break;
		default:
		    fprintf(stdout, "%s\n", SEP);
		    break;
		}
	    }
	    first = 0;

	    area = areaList->value[j];
	    if (Map[i].head.with_z && getz) {
		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout, "Type=Area\nArea_height=%f\n", z);
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, "%s\n\"Type\": \"Area\",\n\"Area_height\": %f",
			    multiple ? "" : ",", z);
		    break;
		default:
		    fprintf(stdout, _("Type: Area\nArea height: %f\n"), z);
		    break;
		}
	    }
	    else {
		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout, "Type=Area\n");
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, "%s\n\"Type\": \"Area\"",
			    multiple ? "" : ",");
		    break;
		default:
		    fprintf(stdout, _("Type: Area\n"));
		    break;
		}
	    }


	    sq_meters = Vect_get_area_area(&Map[i], area);
	    hectares  = sq_meters / 10000.;
	    /* 1 acre = 1 chain(66') * 1 furlong(10 chains),
		or if you prefer ( 5280 ft/mi ^2 / 640 acre/sq mi ) */
	    acres = (sq_meters * sqm_to_sqft) / (66 * 660);
	    sq_miles = acres / 640.;


	    if (topo) {
		int nisles, isleidx, isle, isle_area;

		nisles = Vect_get_area_num_isles(&Map[i], area);
		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout, "Area=%d\nNumber_isles=%d\n", area,
			    nisles);
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, ",\n\"Area\": %d,\n\"Number_isles\": %d",
			    area, nisles);
		    break;
		default:
		    fprintf(stdout, _("Area: %d\nNumber of isles: %d\n"), area,
			    nisles);
		    break;
		}

		for (isleidx = 0; isleidx < nisles; isleidx++) {
		    isle = Vect_get_area_isle(&Map[i], area, isleidx);
		    switch (output) {
		    case OUTPUT_SCRIPT:
			fprintf(stdout, "Isle[%d]=%d\n", isleidx, isle);
			break;
		    case OUTPUT_JSON:
			fprintf(stdout, ",\n\"Isle[%d]\": %d", isleidx, isle);
			break;
		    default:
			fprintf(stdout, _("Isle[%d]: %d\n"), isleidx, isle);
			break;
		    }
		}

		isle = Vect_find_island(&Map[i], east, north);

		if (isle) {
		    isle_area = Vect_get_isle_area(&Map[i], isle);
		    switch (output) {
		    case OUTPUT_SCRIPT:
			fprintf(stdout, "Island=%d\nIsland_area=%d\n", isle,
				isle_area);
			break;
		    case OUTPUT_JSON:
			fprintf(stdout, ",\n\"Island\": %d,\n\"Island_area\": %d",
				isle, isle_area);
			break;
		    default:
			fprintf(stdout, _("Island: %d In area: %d\n"), isle,
				isle_area);
			break;
		    }
		}
	    }
	    else {
		switch (output) {
		case OUTPUT_SCRIPT:
		    fprintf(stdout, "Sq_Meters=%.3f\nHectares=%.3f\n",
			    sq_meters, hectares);
		    fprintf(stdout, "Acres=%.3f\nSq_Miles=%.4f\n", acres,
			    sq_miles);
		    break;
		case OUTPUT_JSON:
		    fprintf(stdout, ",\n\"Sq_Meters\": %.3f,\n\"Hectares\": %.3f",
			    sq_meters, hectares);
		    fprintf(stdout, ",\n\"Acres\": %.3f,\n\"Sq_Miles\": %.4f",
			    acres, sq_miles);
		    break;
		default:
		    fprintf(stdout, _("Sq Meters: %.3f\nHectares: %.3f\n"),
			    sq_meters, hectares);
		    fprintf(stdout, _("Acres: %.3f\nSq Miles: %.4f\n"), acres,
			    sq_miles);
		    break;
		}
		nlines += 3;
	    }
	    centroid = Vect_get_area_centroid(&Map[i], area);
	    if (centroid > 0)
		Vect_read_line(&Map[i], Points, Cats, centroid);

	    write_cats(&Map[i], field[i], Cats, showextra, output);

	    if (output == OUTPUT_JSON && multiple)
		fprintf(stdout, "}");
	}			/* for areaList */

	if (output == OUTPUT_JSON) {
	    if (multiple)
		fprintf(stdout, "]"); /* for features */
	    fprintf(stdout, "}"); /* for map */
	}
    }
    if (output == OUTPUT_JSON)
        fprintf(stdout, "]}\n"); /* for nvects */

    fflush(stdout);
}
