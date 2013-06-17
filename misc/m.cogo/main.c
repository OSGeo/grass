/* ====================================================================
 * PROGRAM: m.cogo
 * AUTHOR:  Eric G. Miller <egm2@jps.net>
 * DATE:    September 29, 2001
 * PURPOSE: Translates simple COordinate GeOmetry with a format of
 *          "[label] <bearing> <distance>" to "X Y [label]".
 *          
 *          Example:  "P0001 S 88-44-56 W 6.7195"
 *                    "-6.7178980970 -0.1467153972 P0001"
 *                   
 *          The input formats are very limited.  
 * --------------------------------------------------------------------
 * COPYRIGHT: (C) 2000 by the GRASS Development Team
 *
 *         This program is free software under the GNU General Public
 *         License (>=v2). Read the file COPYING that comes with GRASS
 *         for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#define DEG2RAD(a) ((a) * M_PI / 180.0)
#define RAD2DEG(a) ((a) * 180.0 / M_PI)
#define DMS2DD(d,m,s) ((d) + ((m) / 60.0) + ((s) / 3600.0))
#define FORMAT_1 " %s %1[NS] %d%c%d%c%lf %1[EW] %lf "
#define FORMAT_2 " %1[NS] %d%c%d%c%lf %1[EW] %lf "
#define FORMAT_3 " %lf %lf %s "


struct survey_record
{
    char label[20];
    int haslabel;
    char n_s[2];
    char e_w[2];
    int deg;
    int min;
    double sec;
    double dist;
    double rads;
    double dd;
    double x;
    double y;
};


static void print_coordinates(FILE *outfile, struct survey_record *in)
{
    if (in->haslabel == YES)
	fprintf(outfile, "%.15g %.15g %s\n", in->x, in->y, in->label);
    else
	fprintf(outfile, "%.15g %.15g\n", in->x, in->y);
}


static void print_cogo(FILE *outfile, struct survey_record *in)
{
    if (in->haslabel == YES)
	fprintf(outfile, "%s %s %02d:%02d:%02.9g %s %.13g\n",
		in->label, in->n_s, in->deg, in->min, in->sec,
		in->e_w, in->dist);
    else
	fprintf(outfile, "%s %02d:%02d:%02.9g %s %.13g\n",
		in->n_s, in->deg, in->min, in->sec, in->e_w, in->dist);

}


static const char *next_line(FILE *infile)
{
    static char line[512];
    const char *cptr;

    memset(line, 0, sizeof(line));
    /* TODO: update to G_getl2(), but this fn needs to return pointer to string not ok/EOF int */
    cptr = fgets(line, 512, infile);
    return cptr;
}


static int parse_forward(const char *in, struct survey_record *out)
{
    char dummy1;
    int status;

    if (out->haslabel == YES) {
	status =
	    sscanf(in, FORMAT_1, out->label, out->n_s, &out->deg, &dummy1,
		   &out->min, &dummy1, &out->sec, out->e_w, &out->dist);
    }
    else {
	status = sscanf(in, FORMAT_2, out->n_s, &out->deg, &dummy1,
			&out->min, &dummy1, &out->sec, out->e_w, &out->dist);
    }

    if ((status != 9 && out->haslabel == YES) ||
	(status != 8 && out->haslabel == NO))
	return 0;

    out->dd = DMS2DD(out->deg, out->min, out->sec);
    if (out->n_s[0] == 'N') {
	if (out->e_w[0] == 'E') {
	    out->dd = 90.0 - out->dd;
	}
	else if (out->e_w[0] == 'W') {
	    out->dd = 90.0 + out->dd;
	}
	else {
	    return 0;
	}
    }
    else if (out->n_s[0] == 'S') {
	if (out->e_w[0] == 'E') {
	    out->dd = 270 + out->dd;
	}
	else if (out->e_w[0] == 'W') {
	    out->dd = 270 - out->dd;
	}
	else {
	    return 0;
	}
    }
    else {
	return 0;
    }
    out->rads = DEG2RAD(out->dd);
    out->x += out->dist * cos(out->rads);
    out->y += out->dist * sin(out->rads);

    return status;
}


static int parse_reverse(const char *in, struct survey_record *out)
{
    double x, y;
    int status;

    status = sscanf(in, FORMAT_3, &x, &y, out->label);
    if (status < 2)
	return 0;
    else if (status == 2)
	out->haslabel = NO;
    else
	out->haslabel = YES;

    G_debug(5, "IN:  x=%f  y=%f  out->x=%f  out->y=%f", x, y, out->x, out->y);

    out->rads = atan2(y - out->y, x - out->x);
    out->dist = hypot(x - out->x, y - out->y);
    out->x = x;
    out->y = y;
    out->dd = RAD2DEG(out->rads);

    G_debug(5, "OUT: out->dd=%f  out->dist=%f", out->dd, out->dist);

    if (out->rads >= 0.0) {
	out->n_s[0] = 'N';
	out->n_s[1] = '\0';
    }
    else {
	out->n_s[0] = 'S';
	out->n_s[1] = '\0';
    }

    if (fabs(out->rads) >= M_PI_2) {
	out->e_w[0] = 'W';
	out->e_w[1] = '\0';
    }
    else {
	out->e_w[0] = 'E';
	out->e_w[1] = '\0';
    }

    if (out->n_s[0] == 'N') {
	if (out->e_w[0] == 'W') {
	    out->dd = out->dd - 90.0;
	}
	else {
	    out->dd = 90.0 - out->dd;
	}
    }
    else {
	if (out->e_w[0] == 'W') {
	    out->dd = fabs(out->dd) - 90.0;
	}
	else {
	    out->dd = 90.0 - fabs(out->dd);
	}
    }

    out->deg = (int)(out->dd);
    out->min = (int)((out->dd - out->deg) * 60.0);
    out->sec = (out->dd - out->deg - out->min / 60.0) * 3600.0;

    return status;
}


int main(int argc, char **argv)
{
    struct Option *input;
    struct Option *output;
    struct Option *coords;
    struct Flag *format;
    struct Flag *quiet;
    struct Flag *reverse;
    struct Flag *close;
    struct GModule *module;
    FILE *infile, *outfile;
    struct survey_record record, first_record;
    const char *cptr;
    char *ss;
    int verbose = TRUE;
    unsigned long linenum = 0, dataline = 0;
    int (*parse_line) (const char *, struct survey_record *);
    void (*print_func) (FILE *, struct survey_record *);

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("miscellaneous"));
    G_add_keyword(_("distance"));
    module->label = _("A simple utility for converting bearing and "
		      "distance measurements to coordinates and vice versa.");
    module->description = _("It assumes a cartesian coordinate system");

    format = G_define_flag();
    format->key = 'l';
    format->description = _("Lines are labelled");

    quiet = G_define_flag();
    quiet->key = 'q';
    quiet->description = _("Suppress warnings");

    reverse = G_define_flag();
    reverse->key = 'r';
    reverse->description =
	_("Convert from coordinates to bearing and distance");

    close = G_define_flag();
    close->key = 'c';
    close->description =
	_("Repeat the starting coordinate at the end to close a loop");

    input = G_define_standard_option(G_OPT_F_INPUT);
    input->required = NO;
    input->answer = "-";

    output = G_define_standard_option(G_OPT_F_OUTPUT);
    output->required = NO;
    output->answer = "-";

    coords = G_define_option();
    coords->key = "coord";
    coords->key_desc = "x,y";
    coords->type = TYPE_DOUBLE;
    coords->required = NO;
    coords->description = _("Starting coordinate pair");
    coords->answer = "0.0,0.0";

    if (G_parser(argc, argv) != 0)
	exit(EXIT_FAILURE);


    if (input->answer && input->answer[0] != '-') {
	infile = fopen(input->answer, "r");
	if (infile == NULL)
	    G_fatal_error(_("Couldn't open COGO file <%s>"), input->answer);
    }
    else {
	infile = stdin;
    }

    if (output->answer && output->answer[0] != '-') {
	outfile = fopen(output->answer, "w");
	if (outfile == NULL)
	    G_fatal_error(_("Couldn't open output file <%s>"),
			  output->answer);
    }
    else {
	outfile = stdout;
    }

    record.label[0] = '\0';
    if (format->answer) {
	record.haslabel = YES;
    }
    else {
	record.haslabel = NO;
    }

    if (quiet->answer)
	verbose = FALSE;

    if (reverse->answer) {
	parse_line = parse_reverse;
	print_func = print_cogo;
    }
    else {
	parse_line = parse_forward;
	print_func = print_coordinates;
    }

    if (coords->answer) {
	record.x = strtod(coords->answers[0], &ss);
	if (ss == coords->answers[0])
	    G_fatal_error(_("Converting starting coordinate pair"));
	record.y = strtod(coords->answers[1], &ss);
	if (ss == coords->answers[1])
	    G_fatal_error(_("Converting starting coordinate pair"));
    }
    else {
	record.x = record.y = 0.0;
    }

    while ((cptr = next_line(infile))) {
	linenum++;

	if ((cptr[0] == '#') || (cptr[0] == '\0') || (cptr[0] == '\n')) {
	    /* remove \n check once module is updated to use G_getl2() */
	    continue;		/* line is a comment or blank */
	}

	if (!parse_line(cptr, &record)) {
	    if (verbose)
		G_warning(_("Input parse error on line %d"), linenum);
	    continue;
	}

	dataline++;

	if (dataline == 1)
	    first_record = record;

	print_func(outfile, &record);
    }

    if (close->answer)
	print_func(outfile, &first_record);


    if (infile != stdin)
	fclose(infile);
    if (outfile != stdout)
	fclose(stdout);

    exit(EXIT_SUCCESS);
}
