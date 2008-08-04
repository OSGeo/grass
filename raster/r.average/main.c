
/****************************************************************************
 *
 * MODULE:       r.average
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Finds the average of values in a cover map within 
 *               areas assigned the same category value in a
 *               user-specified base map.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>


#define STATS "r.stats"
#define RECODE "r.recode"


static int out(FILE *, DCELL, DCELL, double, double);


int main(int argc, char *argv[])
{
    char command[1024];
    char *mapset;
    struct GModule *module;
    struct Option *basemap, *covermap, *outputmap;
    struct Flag *flag_c;
    struct Categories cats;
    DCELL cur_val1, cur_val2, baseval, coverval, c1, c2, b1, b2;
    double x, area, sum1, sum2;
    int stat, usecats;
    char *tempfile1, *tempfile2, in_buf[500];
    FILE *fd1, *fd2;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Finds the average of values in a cover map within "
	  "areas assigned the same category value in a "
	  "user-specified base map.");

    basemap = G_define_standard_option(G_OPT_R_BASE);

    covermap = G_define_standard_option(G_OPT_R_COVER);

    outputmap = G_define_standard_option(G_OPT_R_OUTPUT);

    flag_c = G_define_flag();
    flag_c->key = 'c';
    flag_c->description =
	_("Cover values extracted from the category labels of the cover map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!G_find_cell(basemap->answer, ""))
	G_fatal_error(_("Raster map <%s> not found"), basemap->answer);

    if (!(mapset = G_find_cell(covermap->answer, "")))
	G_fatal_error(_("Raster map <%s> not found"), covermap->answer);

    if (G_legal_filename(outputmap->answer) < 0)
	G_fatal_error(_("<%s> is an illegal file name"), outputmap->answer);

    if ((usecats = flag_c->answer)) {
	if (G_read_cats(covermap->answer, mapset, &cats) < 0)
	    G_fatal_error(_("Error reading category file for <%s>"),
			  covermap->answer);
    }

    tempfile1 = G_convert_dirseps_to_host(G_tempfile());
    tempfile2 = G_convert_dirseps_to_host(G_tempfile());
    G_message("%s:", STATS);
    sprintf(command, "%s -anC input=%s,%s fs=space > \"%s\"",
	    STATS, basemap->answer, covermap->answer, tempfile1);
    if ((stat = system(command))) {
	unlink(tempfile1);
	G_fatal_error(_("%s: ERROR running %s command"), G_program_name(),
		      STATS);
    }

    fd1 = fopen(tempfile1, "r");
    fd2 = fopen(tempfile2, "w");
    if (fd1 == NULL || fd2 == NULL) {
	unlink(tempfile1);
	unlink(tempfile2);
	G_fatal_error(_("%s: unable to open temporary file"),
		      G_program_name());
    }
    out(fd2, 0L, 0L, 0.0, 1.0);	/* force at least one reclass rule */

    G_set_d_null_value(&cur_val1, 1);
    G_set_d_null_value(&cur_val2, 1);
    sum1 = 0.0;
    sum2 = 0.0;
    while (G_getl(in_buf, sizeof(in_buf), fd1)) {
	if (sscanf(in_buf, "%lf-%lf %lf-%lf %lf", &b1, &b2,
		   &c1, &c2, &area) == 5)
	    coverval = (c1 + c2) / 2.;
	else if (sscanf(in_buf, "%lf %lf-%lf %lf", &baseval,
			&c1, &c2, &area) == 4) {
	    coverval = (c1 + c2) / 2.;
	    b1 = b2 = baseval;
	}
	else if (sscanf(in_buf, "%lf-%lf %lf %lf", &b1, &b2,
			&coverval, &area) != 4) {
	    if (sscanf(in_buf, "%lf %lf %lf", &baseval, &coverval, &area) ==
		3)
		b1 = b2 = baseval;
	    else
		break;
	}
	if (cur_val1 != b1 || cur_val2 != b2) {
	    out(fd2, cur_val1, cur_val2, sum1, sum2);
	    sum1 = 0.0;
	    sum2 = 0.0;
	    cur_val1 = b1;
	    cur_val2 = b2;
	}
	if (usecats)
	    sscanf(G_get_d_raster_cat(&coverval, &cats), "%lf", &x);
	else
	    x = coverval;
	sum1 += x * area;
	sum2 += area;
    }
    out(fd2, b1, b2, sum1, sum2);
    fclose(fd1);
    fclose(fd2);
    G_message("%s:", RECODE);
    sprintf(command, "%s input=%s output=%s < \"%s\"",
	    RECODE, basemap->answer, outputmap->answer, tempfile2);
    stat = system(command);
    unlink(tempfile1);
    unlink(tempfile2);

    exit(stat);
}


static int out(FILE * fd, DCELL val1, DCELL val2, double sum1, double sum2)
{
    char b1[80], b2[80];
    DCELL tmp = val1;

    if (sum2 == 0)
	return 0;
    if (G_is_d_null_value(&tmp))
	return 0;
    tmp = val2;
    if (G_is_d_null_value(&tmp))
	return 0;
    sprintf(b1, "%.10f", val1);
    G_trim_decimal(b1);
    sprintf(b2, "%.10f", val2);
    G_trim_decimal(b2);
    fprintf(fd, "%s:%s:%.10f\n", b1, b2, sum1 / sum2);

    return 0;
}
