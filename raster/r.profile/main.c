/*
 * Copyright (C) 2000 by the GRASS Development Team
 * Author: Bob Covill <bcovill@tekmap.ns.ca>
 * 
 * This Program is free software under the GPL (>=v2)
 * Read the file COPYING coming with GRASS for details
 *
 */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

int clr;
struct Colors colors;

static double dist, e, n;


int main(int argc, char *argv[])
{
    char *name, *outfile;
    int fd, projection;
    FILE *fp;
    double res;
    char *null_string;
    char ebuf[256], nbuf[256], label[512], formatbuff[256];
    char b1[100], b2[100];
    int n;
    int havefirst = FALSE;
    int coords = 0, i, k = -1;
    double e1, e2, n1, n2;
    RASTER_MAP_TYPE data_type;
    struct Cell_head window;
    struct
    {
	struct Option *opt1, *profile, *res, *output, *null_str;
	struct Flag *g, *c;
    }
    parm;
    struct GModule *module;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("profile"));
    module->description =
	_("Outputs the raster map layer values lying on user-defined line(s).");

    parm.opt1 = G_define_standard_option(G_OPT_R_INPUT);

    parm.output = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.output->required = NO;
    parm.output->answer = "-";
    parm.output->description =
	_("Name of file for output (use output=- for stdout)");

    parm.profile = G_define_option();
    parm.profile->key = "profile";
    parm.profile->type = TYPE_STRING;
    parm.profile->required = NO;
    parm.profile->multiple = YES;
    parm.profile->key_desc = "east,north";
    parm.profile->description = _("Profile coordinate pairs");

    parm.res = G_define_option();
    parm.res->key = "res";
    parm.res->type = TYPE_DOUBLE;
    parm.res->required = NO;
    parm.res->description =
	_("Resolution along profile (default = current region resolution)");

    parm.null_str = G_define_option();
    parm.null_str->key = "null";
    parm.null_str->type = TYPE_STRING;
    parm.null_str->required = NO;
    parm.null_str->answer = "*";
    parm.null_str->description = _("Character to represent no data cell");

    parm.g = G_define_flag();
    parm.g->key = 'g';
    parm.g->description =
	_("Output easting and northing in first two columns of four column output");

    parm.c = G_define_flag();
    parm.c->key = 'c';
    parm.c->description =
	_("Output RRR:GGG:BBB color values for each profile point");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    clr = 0;
    if (parm.c->answer)
	clr = 1;		/* color output */

    null_string = parm.null_str->answer;

    G_get_window(&window);
    projection = G_projection();
    if (parm.res->answer) {
	res = atof(parm.res->answer);
	/* Catch bad resolution ? */
	if (res <= 0)
	    G_fatal_error(_("Illegal resolution! [%g]"), res);
    }
    else {
	/* Do average of EW and NS res */
	res = (window.ew_res + window.ns_res) / 2;
    }

    G_message(_("Using resolution [%g]"), res);

    G_begin_distance_calculations();

    /* Open Input File for reading */
    /* Get Input Name */
    name = parm.opt1->answer;
    if (parm.g->answer)
	coords = 1;

    /* Open Raster File */
    fd = Rast_open_old(name, "");

    /* initialize color structure */
    if (clr)
	Rast_read_colors(name, "", &colors);

    /* Open ASCII file for output or stdout */
    outfile = parm.output->answer;

    if ((strcmp("-", outfile)) == 0) {
	fp = stdout;
    }
    else if (NULL == (fp = fopen(outfile, "w")))
	G_fatal_error(_("Unable to open file <%s>"), outfile);

    /* Get Raster Type */
    data_type = Rast_get_map_type(fd);
    /* Done with file */

    /* Show message giving output format */
    G_message(_("Output Format:"));
    if (coords == 1)
	sprintf(formatbuff,
		_("[Easting] [Northing] [Along Track Dist.(m)] [Elevation]"));
    else
	sprintf(formatbuff, _("[Along Track Dist.(m)] [Elevation]"));
    if (clr)
	strcat(formatbuff, _(" [RGB Color]"));
    G_message(formatbuff);

    /* Get Profile Start Coords */
    if (!parm.profile->answer) {
	/* Assume input from stdin */
	for (n = 1; input(b1, ebuf, b2, nbuf, label); n++) {
	    G_debug(4, "stdin line %d: ebuf=[%s]  nbuf=[%s]", n, ebuf, nbuf);
	    if (!G_scan_easting(ebuf, &e2, G_projection()) ||
		!G_scan_northing(nbuf, &n2, G_projection()))
		G_fatal_error(_("Invalid coordinates %s %s"), ebuf, nbuf);

	    if (havefirst)
		do_profile(e1, e2, n1, n2, name, coords, res, fd, data_type,
			   fp, null_string);
	    e1 = e2;
	    n1 = n2;
	    havefirst = TRUE;
	}
    }
    else {
	/* Coords from Command Line */
	for (i = 0; parm.profile->answers[i]; i += 2) {
	    /* Test for number coordinate pairs */
	    k = i;
	}

	if (k == 0) {
	    /* Only one coordinate pair supplied */
	    G_scan_easting(parm.profile->answers[0], &e1, G_projection());
	    G_scan_northing(parm.profile->answers[1], &n1, G_projection());
	    e2 = e1;
	    n2 = n1;

	    /* Get profile info */
	    do_profile(e1, e2, n1, n2, name, coords, res, fd, data_type, fp,
		       null_string);
	}
	else {
	    for (i = 0; i <= k - 2; i += 2) {
		G_scan_easting(parm.profile->answers[i], &e1, G_projection());
		G_scan_northing(parm.profile->answers[i + 1], &n1,
				G_projection());
		G_scan_easting(parm.profile->answers[i + 2], &e2,
			       G_projection());
		G_scan_northing(parm.profile->answers[i + 3], &n2,
				G_projection());

		/* Get profile info */
		do_profile(e1, e2, n1, n2, name, coords, res, fd, data_type,
			   fp, null_string);

	    }
	}
    }

    Rast_close(fd);
    fclose(fp);

    if (clr)
	Rast_free_colors(&colors);

    exit(EXIT_SUCCESS);
}				/* Done with main */

/* Calculate the Profile Now */
/* Establish parameters */
int do_profile(double e1, double e2, double n1, double n2, char *name,
	       int coords, double res, int fd, int data_type, FILE * fp,
	       char *null_string)
{
    float rows, cols, LEN;
    double Y, X, AZI;

    cols = e1 - e2;
    rows = n1 - n2;

    LEN = G_distance(e1, n1, e2, n2);
    G_message(_("Approx. transect length [%f] m"), LEN);

    if (!G_point_in_region(e2, n2))
	G_warning(_("Endpoint coordinates are outside of current region settings"));

    /* Calculate Azimuth of Line */
    if (rows == 0 && cols == 0) {
	/* Special case for no movement */
	e = e1;
	n = n1;
	read_rast(e, n, dist, fd, coords, data_type, fp, null_string);
    }

    if (rows >= 0 && cols < 0) {
	/* SE Quad or due east */
	AZI = atan((rows / cols));
	Y = res * sin(AZI);
	X = res * cos(AZI);
	if (Y < 0)
	    Y = Y * -1.;
	if (X < 0)
	    X = X * -1.;
	if (e != 0.0 && (e != e1 || n != n1)) {
	    dist -= G_distance(e, n, e1, n1);
	}
	for (e = e1, n = n1; e < e2 || n > n2; e += X, n -= Y) {
	    read_rast(e, n, dist, fd, coords, data_type, fp, null_string);
	    /* d+=res; */
	    dist += G_distance(e - X, n + Y, e, n);
	}
    }

    if (rows < 0 && cols <= 0) {
	/* NE Quad  or due north */
	AZI = atan((cols / rows));
	X = res * sin(AZI);
	Y = res * cos(AZI);
	if (Y < 0)
	    Y = Y * -1.;
	if (X < 0)
	    X = X * -1.;
	if (e != 0.0 && (e != e1 || n != n1)) {
	    dist -= G_distance(e, n, e1, n1);
	    /*
	     * read_rast (e1, n1, dist, fd, coords, data_type, fp, null_string);
	     */
	}
	for (e = e1, n = n1; e < e2 || n < n2; e += X, n += Y) {
	    read_rast(e, n, dist, fd, coords, data_type, fp, null_string);
	    /* d+=res; */
	    dist += G_distance(e - X, n - Y, e, n);
	}
    }

    if (rows > 0 && cols >= 0) {
	/* SW Quad or due south */
	AZI = atan((rows / cols));
	X = res * cos(AZI);
	Y = res * sin(AZI);
	if (Y < 0)
	    Y = Y * -1.;
	if (X < 0)
	    X = X * -1.;
	if (e != 0.0 && (e != e1 || n != n1)) {
	    dist -= G_distance(e, n, e1, n1);
	}
	for (e = e1, n = n1; e > e2 || n > n2; e -= X, n -= Y) {
	    read_rast(e, n, dist, fd, coords, data_type, fp, null_string);
	    /* d+=res; */
	    dist += G_distance(e + X, n + Y, e, n);
	}
    }

    if (rows <= 0 && cols > 0) {
	/* NW Quad  or due west */
	AZI = atan((rows / cols));
	X = res * cos(AZI);
	Y = res * sin(AZI);
	if (Y < 0)
	    Y = Y * -1.;
	if (X < 0)
	    X = X * -1.;
	if (e != 0.0 && (e != e1 || n != n1)) {
	    dist -= G_distance(e, n, e1, n1);
	}
	for (e = e1, n = n1; e > e2 || n < n2; e -= X, n += Y) {
	    read_rast(e, n, dist, fd, coords, data_type, fp, null_string);
	    /* d+=res; */
	    dist += G_distance(e + X, n - Y, e, n);
	}
    }
    /*
     * return dist;
     */
    return 0;
}				/* done with do_profile */

