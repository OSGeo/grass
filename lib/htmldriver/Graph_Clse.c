/*
 * Close down the graphics processing.  This gets called only at driver
 * termination time.
 */


#include <grass/gis.h>
#include "driverlib.h"
#include "htmlmap.h"

/* sreen dimensions defined in Graph_Set.c */

/* point in polygon test by Randolph Franklin */
/* http://www.ecse.rpi.edu/Homepages/wrf/     */
/* adapted for integer coordinates            */

static int pnpoly(int npol, int *xp, int *yp, int x, int y)
{
    int i, j, c = 0;

    for (i = 0, j = npol - 1; i < npol; j = i++) {
	if ((((yp[i] <= y) && (y < yp[j])) ||
	     ((yp[j] <= y) && (y < yp[i]))) &&
	    (x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))
	    c = !c;
    }
    return c;
}



void HTML_Graph_close(void)
{
    struct MapPoly *poly, *test_poly;

    int i;
    int inside;

    /* 
     * exmaine the list of polygons, if a polygon wholly exists inside of
     * another polygon, then remove it.
     *
     */

    for (poly = html.head; poly != NULL; poly = poly->next_poly) {

	for (test_poly = html.head; test_poly != NULL;
	     test_poly = test_poly->next_poly) {
	    if (poly == test_poly) {
		continue;	/* don't check ourselves */
	    }

	    inside = 1;
	    for (i = 0; i < poly->num_pts && inside; i++) {
		inside = pnpoly(test_poly->num_pts,
				test_poly->x_pts, test_poly->y_pts,
				poly->x_pts[i], poly->y_pts[i]);
	    }
	    if (inside) {
		poly->num_pts = 0;	/* mark polygon as having no points */
		break;
	    }
	}

    }


    /*
     * write any beginning prologue appropriate for the map type
     */

    switch (html.type) {

    case APACHE:
	fprintf(html.output, "#base _base_\n#default _default_\n");
	break;

    case RAW:
	break;

    case CLIENT:
	fprintf(html.output, "<MAP NAME=\"map\">\n");
	break;
    }

    /*
     * write the polygons in a specific format
     */

    for (poly = html.head; poly != NULL; poly = poly->next_poly) {
	if (poly->num_pts >= 3) {

	    switch (html.type) {

	    case APACHE:
		fprintf(html.output, "poly %s", poly->url);
		for (i = 0; i < poly->num_pts; i++) {
		    fprintf(html.output, " %d,%d", poly->x_pts[i], poly->y_pts[i]);
		}
		fprintf(html.output, " %d,%d", poly->x_pts[0], poly->y_pts[0]);
		fprintf(html.output, "\n");
		break;

	    case RAW:
		fprintf(html.output, "%s", poly->url);
		for (i = 0; i < poly->num_pts; i++) {
		    fprintf(html.output, " %d %d", poly->x_pts[i], poly->y_pts[i]);
		}
		fprintf(html.output, " %d %d", poly->x_pts[0], poly->y_pts[0]);
		fprintf(html.output, "\n");
		break;

	    case CLIENT:
		fprintf(html.output,
			"<AREA SHAPE=\"POLY\"\n HREF=\"%s\"\n  ALT=\"%s\"\n  COORDS=\"",
			poly->url, poly->url);
		for (i = 0; i < poly->num_pts; i++) {
		    if (i > 0)
			fprintf(html.output, ", ");
		    /* 
		     * don't add newlines, which confuses the weak-minded
		     * i.e., ms internet exploder :-(
		     * was: if (i % 8 == 0 && i != 0) fprintf(html.output,"\n  ");
		     */
		    fprintf(html.output, "%d,%d", poly->x_pts[i], poly->y_pts[i]);
		}
		fprintf(html.output, ", %d,%d", poly->x_pts[0], poly->y_pts[0]);
		fprintf(html.output, "\">\n");
		break;

	    }

	}

    }

    /* final stuff, if needed */

    switch (html.type) {

    case APACHE:
	break;

    case RAW:
	break;

    case CLIENT:
	fprintf(html.output,
		"<AREA SHAPE=\"RECT\" NOHREF COORDS=\"%d,%d %d,%d\">\n",
		0, 0, screen_width, screen_height);
	fprintf(html.output, "</MAP>\n");
	break;

    }

    /*
     * close file 
     */

    fclose(html.output);
}
