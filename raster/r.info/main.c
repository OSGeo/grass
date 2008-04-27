/***************************************************************************
*
* MODULE:       r.info
*
* AUTHOR(S):    Michael O'Shea
*
* PURPOSE:      Outputs basic information about a user-specified raster map layer.
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
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
#include <grass/glocale.h>
#include "local_proto.h"

#define printline(x) fprintf (out," | %-74.74s |\n",x)
#define divider(x) \
    fprintf (out," %c",x);\
    for (i = 0; i < 76; i++)\
        fprintf(out,"-");\
    fprintf (out,"%c\n",x)


/* local prototypes */
static void format_double (const double, char *);
static void compose_line (FILE *, const char *, ...);


int main (int argc, char **argv)
{
    char *name, *mapset;
    char tmp1[100], tmp2[100], tmp3[100];
    char timebuff[256];
    char units[GNAME_MAX], vdatum[GNAME_MAX];
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
    int head_ok, cats_ok, hist_ok;
    int is_reclass;
    RASTER_MAP_TYPE data_type;
    struct Reclass reclass;
    struct GModule *module;
    struct Option *opt1;
    struct Flag *rflag, *sflag, *tflag, *gflag, *hflag, *mflag;
    struct Flag *uflag, *dflag, *timestampflag;

    /* Initialize GIS Engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Output basic information about a raster map layer.");

    opt1 = G_define_standard_option(G_OPT_R_MAP);

    rflag = G_define_flag();
    rflag->key = 'r';
    rflag->description = _("Print range only");

    sflag = G_define_flag();
    sflag->key = 's';
    sflag->description = _("Print raster map resolution (NS-res, EW-res) only");

    tflag = G_define_flag();
    tflag->key = 't';
    tflag->description = _("Print raster map type only");

    gflag = G_define_flag();
    gflag->key = 'g';
    gflag->description = _("Print map region only");

    hflag = G_define_flag();
    hflag->key = 'h';
    hflag->description = _("Print raster history instead of info");

    uflag = G_define_flag();
    uflag->key = 'u';
    uflag->description = _("Print raster map data units only");

    dflag = G_define_flag();
    dflag->key = 'd';
    dflag->description = _("Print raster map vertical datum only");

    mflag = G_define_flag();
    mflag->key = 'm';
    mflag->description = _("Print map title only");

    timestampflag = G_define_flag();
    timestampflag->key = 'p';
    timestampflag->description =
	_("Print raster map timestamp (day.month.year hour:minute:seconds) only");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    name = G_store(opt1->answer);
    if ((mapset = G_find_cell2(name, "")) == NULL)
	G_fatal_error(_("Raster map <%s> not found"), name);

    head_ok = G_get_cellhd(name, mapset, &cellhd) >= 0;
    cats_ok = G_read_cats(name, mapset, &cats) >= 0;
    hist_ok = G_read_history(name, mapset, &hist) >= 0;
    is_reclass = G_get_reclass(name, mapset, &reclass);
    data_type = G_raster_map_type(name, mapset);

    if( G_read_raster_units(name, mapset, units) != 0)
	units[0] = '\0';
    if( G_read_raster_vdatum(name, mapset, vdatum) != 0)
	vdatum[0] = '\0';

    /*Check the Timestamp */
    time_ok = G_read_raster_timestamp(name, mapset, &ts) > 0;
    /*Check for valid entries, show none if no timestamp available */
    if (time_ok) {
        if (ts.count > 0)
            first_time_ok = 1;
        if (ts.count > 1)
            second_time_ok = 1;
    }

    if (G_read_fp_range(name, mapset, &range) < 0)
        G_fatal_error (_("Unable to read range file"));
    G_get_fp_range_min_max(&range, &zmin, &zmax);

    out = stdout;

    if (!rflag->answer && !sflag->answer && !tflag->answer &&
	!gflag->answer && !hflag->answer && !timestampflag->answer &&
	!mflag->answer && !uflag->answer && !dflag->answer)
    {
	divider('+');

	compose_line(out, "Layer:    %-29.29s  Date: %s", name,
	     hist_ok ? hist.mapid : "??");
	compose_line(out, "Mapset:   %-29.29s  Login of Creator: %s",
		mapset, hist_ok ? hist.creator : "??");
	compose_line(out, "Location: %s", G_location());
	compose_line(out, "DataBase: %s", G_gisdbase());
	compose_line(out, "Title:    %s ( %s )",
		cats_ok ? cats.title : "??",
		hist_ok ? hist.title : "??");

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

	compose_line(out, "  Type of Map:  %-20.20s Number of Categories: %-9s",
	     hist_ok ? hist.maptype : "??", cats_ok ? tmp1 : "??");

	compose_line(out, "  Data Type:    %s",
		       (data_type == CELL_TYPE ? "CELL" :
			(data_type == DCELL_TYPE ? "DCELL" :
			 (data_type == FCELL_TYPE ? "FCELL" : "??"))));

	/* For now hide these unless they exist to keep the noise low. In
	  *   future when the two are used more widely they can be printed
	  *   along with the standard set. */
	if(units[0] || vdatum[0]) {
	    compose_line(out, "  Data Units:   %-20.20s Vertical datum: %s",
		units, vdatum);
	}

	if (head_ok) {
	    compose_line(out, "  Rows:         %d", cellhd.rows);
	    compose_line(out, "  Columns:      %d", cellhd.cols);
#ifdef HAVE_LONG_LONG_INT
	    compose_line(out, "  Total Cells:  %llu",
			(unsigned long long)cellhd.rows * cellhd.cols);
#else
	    compose_line(out, "  Total Cells:  %lu (accuracy - see r.info manual)",
			(unsigned long)cellhd.rows * cellhd.cols);
#endif

	    /* This is printed as a guide to what the following eastings and
	       * northings are printed in. This data is NOT from the values
	       * stored in the map's Cell_head */
	    if(G_projection() == PROJECTION_UTM)
	    {
		compose_line(out, "       Projection: %s (zone %d)",
				G_database_projection_name(), G_zone());
	    }
	    else
	    {
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
		if( 2 == G_read_range(name, mapset, &crange) )
		    compose_line(out, "  Range of data:    min = NULL  max = NULL");
		else
		    compose_line(out, "  Range of data:    min = %i  max = %i",
			(CELL) zmin, (CELL) zmax);
	    }
	    else {
		compose_line(out, "  Range of data:    min = %f  max = %f",
			zmin, zmax);
	    }
	}

	printline("");

	if (hist_ok) {
	    if( hist.datsrc_1[0] != '\0' || hist.datsrc_2[0] != '\0') {
		printline("  Data Source:");
		compose_line(out, "   %s", hist.datsrc_1);
		compose_line(out, "   %s", hist.datsrc_2);
		printline("");
	    }

	    printline("  Data Description:");
	    compose_line(out, "   %s", hist.keywrd);

	    printline("");
	    if (hist.edlinecnt) {
		printline("  Comments:  ");

		for (i = 0; i < hist.edlinecnt; i++) {
		    compose_line(out, "   %s", hist.edhist[i]);
		}
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
		if (G_is_c_null_value(&x))
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
			compose_line(out, "     %5s              %s", num, text);
			*num = 0;
		    }
		    while (next >= 0);
		}
	}
	divider('+');

	fprintf(out, "\n");
    }
    else {  /* rflag or sflag or tflag or gflag or hflag or mflag */

	if (rflag->answer) {
	    if (data_type == CELL_TYPE) {
		if( 2 == G_read_range(name, mapset, &crange) ) {
		    fprintf(out, "min=NULL\n");
		    fprintf(out, "max=NULL\n");
		}
		else {
		    fprintf(out, "min=%i\n", (CELL) zmin);
		    fprintf(out, "max=%i\n", (CELL) zmax);
		}
	    }
	    else {
		fprintf(out, "min=%f\n", zmin);
		fprintf(out, "max=%f\n", zmax);
	    }
	}

	if (gflag->answer) {
	    G_format_northing(cellhd.north, tmp1, cellhd.proj);
	    G_format_northing(cellhd.south, tmp2, cellhd.proj);
	    fprintf(out, "north=%s\n", tmp1);
	    fprintf(out, "south=%s\n", tmp2);

	    G_format_easting(cellhd.east, tmp1, cellhd.proj);
	    G_format_easting(cellhd.west, tmp2, cellhd.proj);
	    fprintf(out, "east=%s\n", tmp1);
	    fprintf(out, "west=%s\n", tmp2);
	}

	if (sflag->answer) {
	    G_format_resolution(cellhd.ns_res, tmp3, cellhd.proj);
	    fprintf(out, "nsres=%s\n", tmp3);

	    G_format_resolution(cellhd.ew_res, tmp3, cellhd.proj);
	    fprintf(out, "ewres=%s\n", tmp3);
	}

	if (tflag->answer) {
	    fprintf(out, "datatype=%s\n",
		    (data_type == CELL_TYPE ? "CELL" :
		     (data_type == DCELL_TYPE ? "DCELL" :
		      (data_type == FCELL_TYPE ? "FCELL" : "??"))));
	}

	if (mflag->answer) {
	    fprintf(out, "title=%s (%s)\n", cats_ok ? cats.title :
				"??", hist_ok ? hist.title : "??");
	}

	if (timestampflag->answer) {
	    if (time_ok && (first_time_ok || second_time_ok)) {

		G_format_timestamp(&ts, timebuff);

		/*Create the r.info timestamp string */
		fprintf(out, "timestamp=\"%s\"\n", timebuff);

	    }
	    else {
		fprintf(out, "timestamp=\"none\"\n");
	    }
	}

	if (uflag->answer)
	    fprintf(out, "units=%s\n", units);
	if (dflag->answer)
	    fprintf(out, "vertical_datum=%s\n", vdatum);

	if (hflag->answer) {
	    if (hist_ok) {
		fprintf(out, "Data Source:\n");
		fprintf(out, "   %s\n", hist.datsrc_1);
		fprintf(out, "   %s\n", hist.datsrc_2);
		fprintf(out, "Data Description:\n");
		fprintf(out, "   %s\n", hist.keywrd);
		if (hist.edlinecnt) {
		    fprintf(out, "Comments:\n");
		    for (i = 0; i < hist.edlinecnt; i++)
			fprintf(out, "   %s\n", hist.edhist[i]);
		}
	    }
	}
    }	/* else rflag or sflag or tflag or gflag or hflag or mflag*/

    return EXIT_SUCCESS;
}


static void format_double (const double value, char *buf)
{
    sprintf(buf, "%.8lf", value);
    G_trim_decimal(buf);
}


static void compose_line(FILE *out, const char *fmt, ...)
{
    char *line = NULL;
    va_list ap;

    va_start(ap, fmt);

    if (G_vasprintf (&line, fmt, ap) <= 0)
        G_fatal_error(_("Cannot allocate memory for string"));

    va_end(ap);

    printline(line);
    G_free(line);
}
