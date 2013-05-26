/*
 *
 ****************************************************************************
 *
 * MODULE:     r.out.ascii
 * AUTHOR(S):  Michael Shapiro
 *             Markus Neteler: added SURFER support
 *             Roger Miller added MODFLOW support and organization
 * PURPOSE:    r.out.ascii: writes ASCII GRID file
 * COPYRIGHT:  (C) 2000 by the GRASS Development Team
 *
 *             This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 ****************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "localproto.h"
#include <grass/glocale.h>


int main(int argc, char *argv[])
{
    RASTER_MAP_TYPE out_type, map_type;
    char *name;
    char *null_str;
    char surfer_null_str[13] = { "1.70141e+038" };
    int fd;
    int nrows, ncols, dp, width;
    int rc;
    FILE *fp;
    struct GModule *module;
    struct
    {
	struct Option *map;
	struct Option *output;
	struct Option *dp;
	struct Option *width;
	struct Option *null;
    } parm;
    struct
    {
	struct Flag *noheader;
	struct Flag *surfer;
	struct Flag *modflow;
	struct Flag *int_out;
    } flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword("ASCII");
    module->description =
	_("Converts a raster map layer into a GRASS ASCII text file.");

    /* Define the different options */

    parm.map = G_define_option();
    parm.map->key = "input";
    parm.map->type = TYPE_STRING;
    parm.map->required = YES;
    parm.map->gisprompt = "old,cell,raster";
    parm.map->description = _("Name of an existing raster map");

    parm.output = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.output->required = NO;
    parm.output->description =
	_("Name for output ASCII grid map (use out=- for stdout)");

    parm.dp = G_define_option();
    parm.dp->key = "dp";
    parm.dp->type = TYPE_INTEGER;
    parm.dp->required = NO;
    parm.dp->description =
	_("Number of significant digits (floating point only)");

    parm.width = G_define_option();
    parm.width->key = "width";
    parm.width->type = TYPE_INTEGER;
    parm.width->required = NO;
    parm.width->description =
	_("Number of values printed before wrapping a line (only SURFER or MODFLOW format)");

    parm.null = G_define_option();
    parm.null->key = "null";
    parm.null->type = TYPE_STRING;
    parm.null->required = NO;
    parm.null->answer = "*";
    parm.null->description =
	_("String to represent null cell (GRASS grid only)");

    flag.noheader = G_define_flag();
    flag.noheader->key = 'h';
    flag.noheader->description = _("Suppress printing of header information");

    flag.surfer = G_define_flag();
    flag.surfer->key = 's';
    flag.surfer->description = _("Write SURFER (Golden Software) ASCII grid");

    flag.modflow = G_define_flag();
    flag.modflow->key = 'm';
    flag.modflow->description = _("Write MODFLOW (USGS) ASCII array");

    flag.int_out = G_define_flag();
    flag.int_out->key = 'i';
    flag.int_out->description = _("Force output of integer values");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (parm.dp->answer) {
	if (sscanf(parm.dp->answer, "%d", &dp) != 1)
	    G_fatal_error(_("Failed to interpret dp as an integer"));
	if (dp > 20 || dp < 0)
	    G_fatal_error(_("dp has to be from 0 to 20"));
    }

    width = 10;
    if (parm.width->answer) {
	if (sscanf(parm.width->answer, "%d", &width) != 1)
	    G_fatal_error(_("Failed to interpret width as an integer"));
    }

    null_str = parm.null->answer;

    if (flag.surfer->answer && flag.noheader->answer)
	G_fatal_error(_("Both -s and -h doesn't make sense"));

    if (flag.surfer->answer && flag.modflow->answer)
	G_fatal_error(_("Use -M or -s, not both"));

    name = parm.map->answer;

    /* open raster map */
    fd = Rast_open_old(name, "");

    map_type = Rast_get_map_type(fd);

    if (!flag.int_out->answer)
	out_type = map_type;
    else
	out_type = CELL_TYPE;

    if (!parm.dp->answer) {
	dp = 6;
	if (out_type == DCELL_TYPE)
	    dp = 16;
    }

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* open ascii file for writing or use stdout */
    if (parm.output->answer && strcmp("-", parm.output->answer) != 0) {
	if (NULL == (fp = fopen(parm.output->answer, "w")))
	    G_fatal_error(_("Unable to open file <%s>"), parm.output->answer);
    }
    else
	fp = stdout;

    /* process the requested output format */
    if (flag.surfer->answer) {
	if (!flag.noheader->answer) {
	    if (writeGSheader(fp, name))
		G_fatal_error(_("Unable to read fp range for <%s>"), name);
	}
	rc = write_GSGRID(fd, fp, nrows, ncols, out_type, dp, surfer_null_str,
			  width);
    }
    else if (flag.modflow->answer) {
	if (!flag.noheader->answer)
	    writeMFheader(fp, dp, width, out_type);
	rc = write_MODFLOW(fd, fp, nrows, ncols, out_type, dp, width);
    }
    else {
	if (!flag.noheader->answer)
	    writeGRASSheader(fp);
	rc = write_GRASS(fd, fp, nrows, ncols, out_type, dp, null_str);
    }
    if (rc) {
	G_fatal_error(_("Read failed at row %d"), rc);
    }

    /* tidy up and go away */
    Rast_close(fd);
    fclose(fp);
    exit(EXIT_SUCCESS);
}
