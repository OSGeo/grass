
/****************************************************************************
 *
 * MODULE:       m.measure
 * AUTHOR(S):    Created from d.measure by Glynn Clements, 2010
 *               James Westervelt and Michael Shapiro 
 *                (CERL - original contributors)
 *               Markus Neteler <neteler itc.it>, 
 *               Reinhard Brunzema <r.brunzema@web.de>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      distance and area measurement
 * COPYRIGHT:    (C) 1999-2006,2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *coords;
    double *x, *y;
    double length, area;
    int npoints;
    int i;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->description = _("Measures the lengths and areas of features.");
    G_add_keyword(_("miscellaneous"));
    G_add_keyword(_("measurement"));

    coords = G_define_option();
    coords->key = "coords";
    coords->description = _("Vertex coordinates");
    coords->type = TYPE_DOUBLE;
    coords->required = YES;
    coords->multiple = YES;
    coords->key_desc = "x,y";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    npoints = 0;
    for (i = 0; coords->answers[i]; i += 2)
	npoints++;
    x = G_malloc(npoints * sizeof(double));
    y = G_malloc(npoints * sizeof(double));

    for (i = 0; i < npoints; i++)
    {
	x[i] = atof(coords->answers[2*i+0]);
	y[i] = atof(coords->answers[2*i+1]);
    }

    G_begin_distance_calculations();
    length = 0;
    for (i = 1; i < npoints; i++)
	length += G_distance(x[i-1], y[i-1], x[i], y[i]);

    printf("LEN:   %10.2f meters\n", length);
    printf("LEN:   %10.4f kilometers\n", length / 1e3);
    printf("LEN:   %10.4f miles\n", length / 1609.344);

    if (npoints > 3) {
	G_begin_polygon_area_calculations();
	area = G_area_of_polygon(x, y, npoints);
	printf("AREA:  %10.2f square meters\n", area);
	printf("AREA:  %10.2f hectares\n", area / 1e4);
	printf("AREA:  %10.4f square kilometers\n", area / 1e6);
	printf("AREA:  %10.4f square miles\n", area / 2589988.11);
    }

    exit(EXIT_SUCCESS);
}
