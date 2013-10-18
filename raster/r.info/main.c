
/***************************************************************************
*
* MODULE:       r.info
*
* AUTHOR(S):    Michael O'Shea
*
* PURPOSE:      Outputs basic information about a user-specified raster map layer.
*
* COPYRIGHT:    (C) 2005-2011 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/config.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define printline(x) fprintf (out," | %-74.74s |\n",x)
#define divider(x) \
    fprintf (out," %c",x);\
    for (i = 0; i < 76; i++)\
        fprintf(out,"-");\
    fprintf (out,"%c\n",x)


/* local prototypes */
static void format_double(const double, char *);
static void compose_line(FILE *, const char *, ...);


int main(int argc, char **argv)
{
    const char *name, *mapset;
    char tmp1[100], tmp2[100], tmp3[100];
    char timebuff[256];
    char *units, *vdatum;
    int i;
    CELL mincat = 0, maxcat = 0, cat;
    double zmin, zmax;		/* min and max data values */
    FILE *out;
    struct Range crange;
    struct FPRange range;
    struct Cell_head cellhd;
    struct Categories cats;
    struct History hist;
    struct TimeStamp ts;
    int time_ok = 0, first_time_ok = 0, second_time_ok = 0;
    int cats_ok, hist_ok;
    int is_reclass;
    RASTER_MAP_TYPE data_type;
    struct Reclass reclass;
    struct GModule *module;
    struct Option *opt1;
    struct Flag *gflag, *rflag, *eflag, *hflag;

    /* Initialize GIS Engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("metadata"));
    G_add_keyword(_("extent"));
    G_add_keyword(_("history"));
    module->description =
	_("Outputs basic information about a raster map.");

    opt1 = G_define_standard_option(G_OPT_R_MAP);

    gflag = G_define_flag();
    gflag->key = 'g';
    gflag->description = _("Print raster array information only");

    rflag = G_define_flag();
    rflag->key = 'r';
    rflag->description = _("Print range only");

    eflag = G_define_flag();
    eflag->key = 'e';
    eflag->description = _("Print extended metadata information only");

    hflag = G_define_flag();
    hflag->key = 'h';
    hflag->description = _("Print raster history instead of info");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    name = G_store(opt1->answer);
    if ((mapset = G_find_raster2(name, "")) == NULL)
	G_fatal_error(_("Raster map <%s> not found"), name);

    Rast_get_cellhd(name, "", &cellhd);
    cats_ok = Rast_read_cats(name, "", &cats) >= 0;
    hist_ok = Rast_read_history(name, "", &hist) >= 0;
    is_reclass = Rast_get_reclass(name, "", &reclass);
    data_type = Rast_map_type(name, "");

    units = Rast_read_units(name, "");

    vdatum = Rast_read_vdatum(name, "");

    /*Check the Timestamp */
    time_ok = G_read_raster_timestamp(name, "", &ts) > 0;
    /*Check for valid entries, show none if no timestamp available */
    if (time_ok) {
	if (ts.count > 0)
	    first_time_ok = 1;
	if (ts.count > 1)
	    second_time_ok = 1;
    }

    if (Rast_read_fp_range(name, "", &range) < 0)
	G_fatal_error(_("Unable to read range file"));
    Rast_get_fp_range_min_max(&range, &zmin, &zmax);

    out = stdout;

    if (!gflag->answer && !rflag->answer &&
	!eflag->answer && !hflag->answer) {
	divider('+');

	compose_line(out, "Layer:    %-29.29s  Date: %s", name,
		     hist_ok ? Rast_get_history(&hist, HIST_MAPID) : "??");
	compose_line(out, "Mapset:   %-29.29s  Login of Creator: %s",
		     mapset, hist_ok ? Rast_get_history(&hist, HIST_CREATOR) : "??");
	compose_line(out, "Location: %s", G_location());
	compose_line(out, "DataBase: %s", G_gisdbase());
	compose_line(out, "Title:    %s ( %s )",
		     cats_ok ? cats.title : "??",
		     hist_ok ? Rast_get_history(&hist, HIST_TITLE) : "??");

	/*This shows the TimeStamp */
	if (time_ok && (first_time_ok || second_time_ok)) {
	    G_format_timestamp(&ts, timebuff);
	    compose_line(out, "Timestamp: %s", timebuff);
	}
	else {
	    compose_line(out, "Timestamp: none");
	}

	divider('|');
	printline("");

	if (cats_ok)
	    format_double((double)cats.num, tmp1);

	compose_line(out,
		     "  Type of Map:  %-20.20s Number of Categories: %-9s",
		     hist_ok ? Rast_get_history(&hist, HIST_MAPTYPE) : "??", cats_ok ? tmp1 : "??");

	compose_line(out, "  Data Type:    %s",
		     (data_type == CELL_TYPE ? "CELL" :
		      (data_type == DCELL_TYPE ? "DCELL" :
		       (data_type == FCELL_TYPE ? "FCELL" : "??"))));

	/* For now hide these unless they exist to keep the noise low. In
	 *   future when the two are used more widely they can be printed
	 *   along with the standard set. */
	if (units || vdatum)
	    compose_line(out, "  Data Units:   %-20.20s Vertical datum: %s",
			 units ? units : "(none)", vdatum ? vdatum : "(none)");

	{
	    compose_line(out, "  Rows:         %d", cellhd.rows);
	    compose_line(out, "  Columns:      %d", cellhd.cols);
#ifdef HAVE_LONG_LONG_INT
	    compose_line(out, "  Total Cells:  %llu",
			 (unsigned long long)cellhd.rows * cellhd.cols);
#else
	    compose_line(out,
			 "  Total Cells:  %lu (accuracy - see r.info manual)",
			 (unsigned long)cellhd.rows * cellhd.cols);
#endif

	    /* This is printed as a guide to what the following eastings and
	     * northings are printed in. This data is NOT from the values
	     * stored in the map's Cell_head */
	    if (G_projection() == PROJECTION_UTM) {
		compose_line(out, "       Projection: %s (zone %d)",
			     G_database_projection_name(), G_zone());
	    }
	    else {
		compose_line(out, "       Projection: %s",
			     G_database_projection_name());
	    }

	    G_format_northing(cellhd.north, tmp1, cellhd.proj);
	    G_format_northing(cellhd.south, tmp2, cellhd.proj);
	    G_format_resolution(cellhd.ns_res, tmp3, cellhd.proj);
	    compose_line(out, "           N: %10s    S: %10s   Res: %5s",
			 tmp1, tmp2, tmp3);

	    G_format_easting(cellhd.east, tmp1, cellhd.proj);
	    G_format_easting(cellhd.west, tmp2, cellhd.proj);
	    G_format_resolution(cellhd.ew_res, tmp3, cellhd.proj);
	    compose_line(out, "           E: %10s    W: %10s   Res: %5s",
			 tmp1, tmp2, tmp3);

	    if (data_type == CELL_TYPE) {
		if (2 == Rast_read_range(name, "", &crange))
		    compose_line(out,
				 "  Range of data:    min = NULL  max = NULL");
		else
		    compose_line(out,
				 "  Range of data:    min = %i  max = %i",
				 (CELL) zmin, (CELL) zmax);
	    }
	    else if (data_type == FCELL_TYPE) {
		compose_line(out, "  Range of data:    min = %.7g  max = %.7g",
			     zmin, zmax);
	    }
	    else {
		compose_line(out, "  Range of data:    min = %.15g  max = %.15g",
			     zmin, zmax);
	    }
	}

	printline("");

	if (hist_ok) {
	    if (Rast_get_history(&hist, HIST_DATSRC_1)[0] != '\0' ||
		Rast_get_history(&hist, HIST_DATSRC_2)[0] != '\0') {
		printline("  Data Source:");
		compose_line(out, "   %s", Rast_get_history(&hist, HIST_DATSRC_1));
		compose_line(out, "   %s", Rast_get_history(&hist, HIST_DATSRC_2));
		printline("");
	    }

	    printline("  Data Description:");
	    compose_line(out, "   %s", Rast_get_history(&hist, HIST_KEYWRD));

	    printline("");
	    if (Rast_history_length(&hist)) {
		printline("  Comments:  ");

		for (i = 0; i < Rast_history_length(&hist); i++)
		    compose_line(out, "   %s", Rast_history_line(&hist, i));
	    }

	    printline("");
	}

	if (is_reclass > 0) {
	    int first = 1;

	    divider('|');

	    compose_line(out, "  Reclassification of [%s] in mapset [%s]",
			 reclass.name, reclass.mapset);

	    printline("");
	    printline("        Category        Original categories");
	    printline("");

	    for (i = 0; i < reclass.num; i++) {
		CELL x = reclass.table[i];

		if (Rast_is_c_null_value(&x))
		    continue;
		if (first || x < mincat)
		    mincat = x;
		if (first || x > maxcat)
		    maxcat = x;
		first = 0;
	    }

	    if (!first)
		for (cat = mincat; cat <= maxcat; cat++) {
		    char text[80];
		    char *num;
		    int next;

		    if (cat == 0)
			continue;
		    if (G_asprintf(&num, "%5ld", (long)cat) < 1)
			G_fatal_error(_("Cannot allocate memory for string"));

		    next = 0;
		    do {
			next = reclass_text(text, cat, &reclass, next);
			compose_line(out, "     %5s              %s", num,
				     text);
			*num = 0;
		    }
		    while (next >= 0);
		}
	}
	divider('+');

	fprintf(out, "\n");
    }
    else {	/* g,r,e, or h flags */

	if (gflag->answer) {
	    G_format_northing(cellhd.north, tmp1, -1);
	    G_format_northing(cellhd.south, tmp2, -1);
	    fprintf(out, "north=%s\n", tmp1);
	    fprintf(out, "south=%s\n", tmp2);

	    G_format_easting(cellhd.east, tmp1, -1);
	    G_format_easting(cellhd.west, tmp2, -1);
	    fprintf(out, "east=%s\n", tmp1);
	    fprintf(out, "west=%s\n", tmp2);

	    G_format_resolution(cellhd.ns_res, tmp3, -1);
	    fprintf(out, "nsres=%s\n", tmp3);

	    G_format_resolution(cellhd.ew_res, tmp3, -1);
	    fprintf(out, "ewres=%s\n", tmp3);

            fprintf(out, "rows=%d\n", cellhd.rows);
            fprintf(out, "cols=%d\n", cellhd.cols);
            
            fprintf(out, "cells=%lld\n",
                    (long long)cellhd.rows * cellhd.cols);
            
	    fprintf(out, "datatype=%s\n",
		    (data_type == CELL_TYPE ? "CELL" :
		     (data_type == DCELL_TYPE ? "DCELL" :
		      (data_type == FCELL_TYPE ? "FCELL" : "??"))));
	}

	if (rflag->answer) {
	    if (data_type == CELL_TYPE) {
		if (2 == Rast_read_range(name, "", &crange)) {
		    fprintf(out, "min=NULL\n");
		    fprintf(out, "max=NULL\n");
		}
		else {
		    fprintf(out, "min=%i\n", (CELL) zmin);
		    fprintf(out, "max=%i\n", (CELL) zmax);
		}
	    }
	    else if (data_type == FCELL_TYPE) {
		fprintf(out, "min=%.7g\n", zmin);
		fprintf(out, "max=%.7g\n", zmax);
	    }
	    else {
		fprintf(out, "min=%.15g\n", zmin);
		fprintf(out, "max=%.15g\n", zmax);
	    }
	}

	if (eflag->answer) {
	    fprintf(out, "title=%s (%s)\n", cats_ok ? cats.title :
		    "??", hist_ok ? Rast_get_history(&hist, HIST_TITLE) : "??");

	    fprintf(out, "units=%s\n", units ? units : "\"none\"");
	    fprintf(out, "vertical_datum=%s\n", vdatum ? vdatum : "\"none\"");

	    if (time_ok && (first_time_ok || second_time_ok)) {

		G_format_timestamp(&ts, timebuff);

		/*Create the r.info timestamp string */
		fprintf(out, "timestamp=\"%s\"\n", timebuff);

	    }
	    else {
		fprintf(out, "timestamp=\"none\"\n");
	    }
	}

	if (hflag->answer) {
	    if (hist_ok) {
		fprintf(out, "Data Source:\n");
		fprintf(out, "   %s\n", Rast_get_history(&hist, HIST_DATSRC_1));
		fprintf(out, "   %s\n", Rast_get_history(&hist, HIST_DATSRC_2));
		fprintf(out, "Data Description:\n");
		fprintf(out, "   %s\n", Rast_get_history(&hist, HIST_KEYWRD));
		if (Rast_history_length(&hist)) {
		    fprintf(out, "Comments:\n");
		    for (i = 0; i < Rast_history_length(&hist); i++)
			fprintf(out, "   %s\n", Rast_history_line(&hist, i));
		}
	    }
	}
    }				/* else rflag or sflag or tflag or gflag or hflag or mflag */

    return EXIT_SUCCESS;
}


static void format_double(const double value, char *buf)
{
    sprintf(buf, "%.8lf", value);
    G_trim_decimal(buf);
}


static void compose_line(FILE * out, const char *fmt, ...)
{
    char *line = NULL;
    va_list ap;

    va_start(ap, fmt);

    if (G_vasprintf(&line, fmt, ap) <= 0)
	G_fatal_error(_("Cannot allocate memory for string"));

    va_end(ap);

    printline(line);
    G_free(line);
}
