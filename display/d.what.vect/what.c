#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/vector.h>
#include <grass/form.h>
#include <grass/dbmi.h>
#include "what.h"
#include <grass/glocale.h>

static int nlines = 50;

#define WDTH 5

int what(int once, int txt, int terse, int width, int mwidth,
	 int topo, int edit)
{
    int type, edit_mode;
    int row, col;
    int nrows, ncols;
    struct Cell_head window;
    int screen_x, screen_y;
    double east, north;
    int button;
    char east_buf[40], north_buf[40];
    double sq_meters, sqm_to_sqft, acres, hectares, sq_miles;
    double x1, y1, x2, y2, z = 0, l = 0;
    int notty;
    double maxdist;
    int getz = 0;
    struct field_info *Fi;

    plus_t line, area = 0, centroid;
    int i;
    struct line_pnts *Points;
    struct line_cats *Cats;
    char buf[1000], *str, title[500];
    dbString html;
    char *form;

    if (terse)
	txt = 1;		/* force text for terse */

    G_get_set_window(&window);

    G_begin_polygon_area_calculations();
    nrows = window.rows;
    ncols = window.cols;

    screen_x = ((int)D_get_d_west() + (int)D_get_d_east()) / 2;
    screen_y = ((int)D_get_d_north() + (int)D_get_d_south()) / 2;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    db_init_string(&html);

    if (!isatty(fileno(stdout)))
	notty = 1;		/* no terminal */
    else
	notty = 0;

    /* always use plain feet not US survey ft */
    /*  if you really want USfeet, try G_database_units_to_meters_factor()
	here, but then watch that sq_miles is not affected too */
    sqm_to_sqft = 1 / ( 0.0254 * 0.0254 * 12 * 12 );


    do {
	if (!terse)
	    show_buttons(once);
	R_get_location_with_pointer(&screen_x, &screen_y, &button);
	if (!once) {
	    if (button == 3) {
		break;
	    }
	    if (button == 2) {
		continue;
	    }
	}

	east = D_d_to_u_col((double)screen_x);
	north = D_d_to_u_row((double)screen_y);

	row = (window.north - north) / window.ns_res;
	col = (east - window.west) / window.ew_res;
	if (row < 0 || row >= nrows)
	    continue;
	if (col < 0 || col >= ncols)
	    continue;

	x1 = D_d_to_u_col((double)(screen_x - WDTH));
	y1 = D_d_to_u_row((double)(screen_y - WDTH));
	x2 = D_d_to_u_col((double)(screen_x + WDTH));
	y2 = D_d_to_u_row((double)(screen_y + WDTH));

	x1 = fabs(x2 - x1);
	y1 = fabs(y2 - y1);

	if (x1 > y1)
	    maxdist = x1;
	else
	    maxdist = y1;
	G_debug(1, "Maximum distance in map units = %f\n", maxdist);

	F_clear();
	for (i = 0; i < nvects; i++) {
	    Vect_reset_cats(Cats);
	    /* Try to find point first and only if no one was found try lines,
	     *  otherwise point on line could not be selected ans similarly for areas */
	    line =
		Vect_find_line(&Map[i], east, north, 0.0,
			       GV_POINT | GV_CENTROID, maxdist, 0, 0);
	    if (line == 0)
		line = Vect_find_line(&Map[i], east, north, 0.0,
				      GV_LINE | GV_BOUNDARY | GV_FACE,
				      maxdist, 0, 0);

	    area = 0;
	    if (line == 0) {
		area = Vect_find_area(&Map[i], east, north);
		getz = Vect_tin_get_z(&Map[i], east, north, &z, NULL, NULL);
	    }

	    G_debug(2, "line = %d, area = %d", line, area);

	    if (!i && txt) {
		G_format_easting(east, east_buf, G_projection());
		G_format_northing(north, north_buf, G_projection());
		fprintf(stdout, "\n%s(E) %s(N)\n", east_buf, north_buf);
		if (notty)
		    fprintf(stderr, "\n%s(E) %s(N)\n", east_buf, north_buf);
		nlines++;
	    }

	    strcpy(buf, vect[i]);
	    if ((str = strchr(buf, '@')))
		*str = 0;

	    if (txt) {
		fprintf(stdout, "\n%*s in %-*s  ", width, Map[i].name, mwidth,
			Map[i].mapset);
		if (notty)
		    fprintf(stderr, "\n%*s in %-*s  ", width, Map[i].name,
			    mwidth, Map[i].mapset);
		nlines++;
	    }

	    if (line + area == 0) {
		if (txt) {
		    fprintf(stdout, _("Nothing Found.\n"));
		    if (notty)
			fprintf(stderr, _("Nothing Found.\n"));
		    nlines++;
		}
		continue;
	    }
	    else {
		/* Start form */
		db_set_string(&html, "");
		if (!txt) {
		    sprintf(title, "%s", Map[i].name);
		    db_append_string(&html,
				     "<HTML><HEAD><TITLE>Form</TITLE><BODY>");
		    sprintf(buf, "map: '%s'<BR>mapset: '%s'<BR>", Map[i].name,
			    Map[i].mapset);
		    db_append_string(&html, buf);
		}
	    }

	    if (line > 0) {
		type = Vect_read_line(&Map[i], Points, Cats, line);
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
		    int n, node[2], nnodes, nnlines, nli, nodeline, left,
			right;
		    float angle;

		    fprintf(stdout,
			    "-----------------------------------------------\n");
		    if (type & GV_BOUNDARY)
			Vect_get_line_areas(&(Map[i]), line, &left, &right);
		    else
			left = right = 0;
		    fprintf(stdout,
			    _("Line: %d  Type: %s  Left: %d  Right: %d  "),
			    line, buf, left, right);
		    if (type & GV_LINES) {
			nnodes = 2;
			fprintf(stdout, _("Length: %f\n"), l);
		    }
		    else {	/* points */
			nnodes = 0;
			fprintf(stdout, "\n");
		    }

		    if (nnodes > 0)
			Vect_get_line_nodes(&(Map[i]), line, &node[0], &node[1]);

		    for (n = 0; n < nnodes; n++) {
			double nx, ny, nz;

			nnlines = Vect_get_node_n_lines(&(Map[i]), node[n]);

			Vect_get_node_coor(&(Map[i]), node[n], &nx, &ny, &nz);
			fprintf(stdout,
				_("  Node[%d]: %d  Number of lines: %d  Coordinates: %.6f, %.6f, %.6f\n"),
				n, node[n], nnlines, nx, ny, nz);

			for (nli = 0; nli < nnlines; nli++) {
			    nodeline =
				Vect_get_node_line(&(Map[i]), node[n], nli);
			    angle =
				Vect_get_node_line_angle(&(Map[i]), node[n],
							 nli);
			    fprintf(stdout, _("    Line: %5d  Angle: %.8f\n"),
				    nodeline, angle);
			}
		    }

		}
		else if (txt) {
		    fprintf(stdout, "%s\n", buf);
		    if (type & GV_LINES)
			fprintf(stdout, _("length %f\n"), l);
		}
		else {
		    db_append_string(&html, "feature type: ");
		    db_append_string(&html, buf);
		    db_append_string(&html, "<BR>");

		    if (type & GV_LINES) {
			sprintf(buf, "length: %f<BR>", l);
			db_append_string(&html, buf);
		    }
		}

		/* Height */
		if (Vect_is_3d(&(Map[i]))) {
		    int j;
		    double min, max;

		    if (type & GV_POINTS) {
			if (txt)
			    fprintf(stdout, _("Point height: %f\n"),
				    Points->z[0]);
			else {
			    sprintf(buf, "Point height: %f<BR>",
				    Points->z[0]);
			    db_append_string(&html, buf);
			}
		    }
		    else if (type & GV_LINES) {
			min = max = Points->z[0];
			for (j = 1; j < Points->n_points; j++) {
			    if (Points->z[j] < min)
				min = Points->z[j];
			    if (Points->z[j] > max)
				max = Points->z[j];
			}
			if (min == max) {
			    if (txt)
				fprintf(stdout, _("Line height: %f\n"), min);
			    else {
				sprintf(buf, "Line height: %f<BR>", min);
				db_append_string(&html, buf);
			    }
			}
			else {
			    if (txt)
				fprintf(stdout,
					_("Line height min: %f max: %f\n"),
					min, max);
			    else {
				sprintf(buf,
					"Line height min: %f max: %f<BR>",
					min, max);
				db_append_string(&html, buf);
			    }
			}
		    }
		}
	    }

	    if (area > 0) {
		if (Map[i].head.with_z && getz) {
		    if (txt)
			fprintf(stdout, _("Area height: %f\n"), z);
		    else {
			sprintf(buf, "feature type: Area<BR>height: %f<BR>",
				z);
			db_append_string(&html, buf);
		    }
		}
		else {
		    if (txt) {
			fprintf(stdout, _("Area\n"));
		    }
		    else {
			sprintf(buf, "feature type: Area<BR>");
			db_append_string(&html, buf);
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
		    fprintf(stdout,
			    "-----------------------------------------------\n");
		    fprintf(stdout, _("Area: %d  Number of isles: %d\n"),
			    area, nisles);

		    for (isleidx = 0; isleidx < nisles; isleidx++) {
			isle = Vect_get_area_isle(&Map[i], area, isleidx);
			fprintf(stdout, _("  Isle[%d]: %d\n"), isleidx, isle);
		    }

		    isle = Vect_find_island(&Map[i], east, north);

		    if (isle) {
			isle_area = Vect_get_isle_area(&Map[i], isle);
			fprintf(stdout, _("Island: %d  In area: %d\n"), isle,
				isle_area);
		    }

		}
		else if (txt) {
		    fprintf(stdout,
			    _("Size - Sq Meters: %.3f\t\tHectares: %.3f\n"),
			    sq_meters, hectares);

		    fprintf(stdout,
			    _("           Acres: %.3f\t\tSq Miles: %.4f\n"),
			    acres, sq_miles);
		    if (notty) {
			fprintf(stderr,
				_("Size - Sq Meters: %.3f\t\tHectares: %.3f\n"),
				sq_meters, hectares);

			fprintf(stderr,
				_("           Acres: %.3f\t\tSq Miles: %.4f\n"),
				acres, sq_miles);
		    }
		    nlines += 3;
		}
		else {
		    sprintf(buf, "area size: %f<BR>", sq_meters);
		    db_append_string(&html, buf);
		}

		centroid = Vect_get_area_centroid(&Map[i], area);
		if (centroid > 0) {
		    Vect_read_line(&Map[i], Points, Cats, centroid);
		}
	    }

	    if (Cats->n_cats > 0) {
		int j;

		for (j = 0; j < Cats->n_cats; j++) {
		    G_debug(2, "field = %d category = %d", Cats->field[j],
			    Cats->cat[j]);
		    if (txt) {
			fprintf(stdout, _("Layer: %d\ncategory: %d\n"),
				Cats->field[j], Cats->cat[j]);
		    }
		    else {
			db_append_string(&html, "<HR><BR>");
			sprintf(buf, "Layer: %d<BR>category: %d<BR>",
				Cats->field[j], Cats->cat[j]);
			db_append_string(&html, buf);
		    }
		    Fi = Vect_get_field(&(Map[i]), Cats->field[j]);
		    if (Fi == NULL) {
			if (txt) {
			    fprintf(stdout,
				    _("Database connection not defined"));
			}
			else {
			    db_append_string(&html,
					     "Database connection not defined<BR>");
			}
		    }
		    else {
			int format;

			if (txt) {
			    fprintf(stdout,
				    _("driver: %s\ndatabase: %s\ntable: %s\nkey column: %s\n"),
				    Fi->driver, Fi->database, Fi->table,
				    Fi->key);
			}
			else {
			    sprintf(buf,
				    "driver: %s<BR>database: %s<BR>table: %s<BR>key column: %s<BR>",
				    Fi->driver, Fi->database, Fi->table,
				    Fi->key);
			    db_append_string(&html, buf);
			}

			if (edit && strcmp(Map[i].mapset, G_mapset()) == 0)
			    edit_mode = F_EDIT;
			else
			    edit_mode = F_VIEW;

			if (txt)
			    format = F_TXT;
			else
			    format = F_HTML;
			F_generate(Fi->driver, Fi->database, Fi->table,
				   Fi->key, Cats->cat[j], NULL, NULL,
				   edit_mode, format, &form);

			if (txt) {
			    fprintf(stdout, "%s", form);
			}
			else {
			    db_append_string(&html, form);
			}
			G_free(form);
			G_free(Fi);
		    }
		}
	    }
	    fflush(stdout);
	    if (!txt && !topo) {
		db_append_string(&html, "</BODY></HTML>");
		G_debug(3, db_get_string(&html));
		F_open(title, db_get_string(&html));
	    }
	}

    } while (!once);
    Vect_destroy_line_struct(Points);

    return 0;
}

/* TODO */
int show_buttons(int once)
{
    if (once) {
	fprintf(stderr, _("\nClick mouse button on desired location\n\n"));
	nlines = 3;
    }
    else if (nlines >= 18) {	/* display prompt every screen full */
	fprintf(stderr, "\n");
	fprintf(stderr, _("Buttons\n"));
	fprintf(stderr, _(" Left:  what's here\n"));
	fprintf(stderr, _(" Right: quit\n"));
	nlines = 4;
    }

    return 0;
}
