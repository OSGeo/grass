
/***************************************************************************
*
* MODULE:       r3.info
*
* AUTHOR(S):    Roman Waupotitsch, Michael Shapiro, Helena Mitasova, Bill Brown,
*               Lubos Mitas, Jaro Hofierka
*
* PURPOSE:      Outputs basic information about a user-specified 3D raster map layer.
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

/* \todo
 *    History support still not full implemented.
 *    Only parts of the timestep functionality are implemented, the timzone is missed ;).
 */

/*local prototype */
int format_double(double value, char *buf);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

#define printline(x) fprintf (out," | %-74.74s |\n",x)
#define divider(x) \
    fprintf (out," %c",x);\
    for (i = 0; i < 76; i++)\
        fprintf(out,"-");\
    fprintf (out,"%c\n",x)

#define TMP_LENGTH 100

static char *name;

/**************************************************************************/
int main(int argc, char *argv[])
{
    const char *mapset;
    char *line = NULL;
    char tmp1[TMP_LENGTH], tmp2[TMP_LENGTH], tmp3[TMP_LENGTH];
    char timebuff[256];
    int i;
    FILE *out;
    RASTER3D_Region cellhd;
    RASTER3D_Map *g3map;
    struct Categories cats;
    struct History hist;
    struct TimeStamp ts;
    int head_ok;
    int cats_ok;
    int hist_ok;
    int time_ok = 0, first_time_ok = 0, second_time_ok = 0;
    struct Option *opt1;
    struct Flag *rflag;
    struct Flag *gflag;
    struct Flag *hflag;
    int data_type;

    struct GModule *module;
    double dmin, dmax;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("metadata"));
    G_add_keyword(_("extent"));
    G_add_keyword(_("voxel"));
    module->description =
	_("Outputs basic information about a user-specified 3D raster map layer.");

    opt1 = G_define_standard_option(G_OPT_R3_MAP);

    gflag = G_define_flag();
    gflag->key = 'g';
    gflag->description = _("Print raster3d information in shell style");

    rflag = G_define_flag();
    rflag->key = 'r';
    rflag->description = _("Print range in shell style only");

    hflag = G_define_flag();
    hflag->key = 'h';
    hflag->description = _("Print raster history instead of info");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = G_store(opt1->answer);
    mapset = G_find_raster3d(name, "");

    if (mapset == NULL)
	G_fatal_error(_("3D Raster map <%s> not found"), name);

    /*We need to open the map */
    g3map =
	Rast3d_open_cell_old(name, mapset, RASTER3D_DEFAULT_WINDOW,
			RASTER3D_TILE_SAME_AS_FILE, RASTER3D_NO_CACHE);
    if (NULL == g3map)
	G_fatal_error(_("Unable to open 3D raster map <%s>"), name);

    /*Get the maptype */
    data_type = Rast3d_file_type_map(g3map);
    head_ok = Rast3d_read_region_map(name, mapset, &cellhd) >= 0;
    hist_ok = Rast3d_read_history(name, mapset, &hist) >= 0;
    cats_ok = Rast3d_read_cats(name, mapset, &cats) >= 0;
    /*Check the Timestamp */
    time_ok = G_read_raster3d_timestamp(name, mapset, &ts) > 0;

    /*Check for valid entries, show none if no entire available! */
    if (time_ok) {
	if (ts.count > 0)
	    first_time_ok = 1;
	if (ts.count > 1)
	    second_time_ok = 1;
    }

    out = stdout;

    /*Show the info if no flag is set */
    if (!rflag->answer && !gflag->answer && !hflag->answer) {
	divider('+');

	if (G_asprintf(&line, "Layer:    %-29.29s  Date: %s", name,
		       hist_ok ? Rast_get_history(&hist, HIST_MAPID) : "??") > 0)
	    printline(line);
	else
	    G_fatal_error(_("Cannot allocate memory for string"));


	if (G_asprintf(&line, "Mapset:   %-29.29s  Login of Creator: %s", mapset,
	     hist_ok ? Rast_get_history(&hist, HIST_CREATOR) : "??") > 0)
	    printline(line);
	else
	    G_fatal_error(_("Cannot allocate memory for string"));


	if (G_asprintf(&line, "Location: %s", G_location()) > 0)
	    printline(line);
	else
	    G_fatal_error(_("Cannot allocate memory for string"));

	if (G_asprintf(&line, "DataBase: %s", G_gisdbase()) > 0)
	    printline(line);
	else
	    G_fatal_error(_("Cannot allocate memory for string"));

	if (G_asprintf(&line, "Title:    %s", 
                hist_ok ? Rast_get_history(&hist, HIST_TITLE) : "??") > 0)
	    printline(line);
	else
	    G_fatal_error(_("Cannot allocate memory for string"));

	if (G_asprintf(&line, "Units:    %s", Rast3d_get_unit(g3map)))
	    printline(line);
	else
	    G_fatal_error(_("Cannot allocate memory for string"));

	if (G_asprintf(&line, "Vertical unit: %s", Rast3d_get_vertical_unit(g3map)))
	    printline(line);
	else
	    G_fatal_error(_("Cannot allocate memory for string"));

	/*This shows the TimeStamp */
	if (time_ok && (first_time_ok || second_time_ok)) {

	    G_format_timestamp(&ts, timebuff);

	    /*Create the r.info timestamp string */
	    if (G_asprintf(&line, "Timestamp: %s", timebuff) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));
	}
	else {
	    if (G_asprintf(&line, "Timestamp: none") > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));
	}


	divider('|');
	printline("");

	if (cats_ok) {
	    format_double((double)cats.num, tmp1);
	}

	if (G_asprintf(&line, "  Type of Map:  %-20.20s Number of Categories: %-9s",
	     "3d cell", cats_ok ? tmp1 : "??") > 0)
	    printline(line);
	else
	    G_fatal_error(_("Cannot allocate memory for string"));

	if (G_asprintf(&line, "  Data Type:    %s",
		       (data_type == FCELL_TYPE ? "FCELL" :
			(data_type == DCELL_TYPE ? "DCELL" : "??"))) > 0)
	    printline(line);
	else
	    G_fatal_error(_("Cannot allocate memory for string"));


	if (head_ok) {
	    if (G_asprintf(&line, "  Rows:         %d", cellhd.rows) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    if (G_asprintf(&line, "  Columns:      %d", cellhd.cols) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    if (G_asprintf(&line, "  Depths:       %d", cellhd.depths) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    if (G_asprintf
		(&line, "  Total Cells:  %ld",
		 (long)cellhd.rows * cellhd.cols * cellhd.depths) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

            double totalSize = 0;
            for(i = 0; i < g3map->nTiles; i++)
                totalSize += g3map->tileLength[i];

	    if (G_asprintf(&line, "  Total size:           %ld Bytes",
                (long)(totalSize)) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    if (G_asprintf(&line, "  Number of tiles:      %d",
                g3map->nTiles) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    if (G_asprintf(&line, "  Mean tile size:       %ld Bytes",
                (long)(totalSize/g3map->nTiles)) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

            int tileSize = 0;

            if(data_type == FCELL_TYPE)
                tileSize = sizeof(FCELL) * g3map->tileX * g3map->tileY *
                        ((RASTER3D_Map* )g3map)->tileZ;

            if(data_type == DCELL_TYPE)
                tileSize = sizeof(DCELL) * g3map->tileX * g3map->tileY *
                        g3map->tileZ;

	    if (G_asprintf(&line, "  Tile size in memory:  %ld Bytes",
                (long)(tileSize)) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    if (G_asprintf(&line, "  Number of tiles in x, y and  z:   %d, %d, %d",
                                  g3map->nx, g3map->ny,
                                  g3map->nz) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    if (G_asprintf(&line, "  Dimension of a tile in x, y, z:   %d, %d, %d",
                                  g3map->tileX, g3map->tileY,
                                  g3map->tileZ) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

            printline("");

	    if (G_asprintf
		(&line, "       Projection: %s (zone %d)",
		 G_database_projection_name(), G_zone()) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    G_format_northing(cellhd.north, tmp1, cellhd.proj);
	    G_format_northing(cellhd.south, tmp2, cellhd.proj);
	    G_format_resolution(cellhd.ns_res, tmp3, cellhd.proj);
	    if (G_asprintf
		(&line, "           N: %10s    S: %10s   Res: %5s", tmp1,
		 tmp2, tmp3) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    G_format_easting(cellhd.east, tmp1, cellhd.proj);
	    G_format_easting(cellhd.west, tmp2, cellhd.proj);
	    G_format_resolution(cellhd.ew_res, tmp3, cellhd.proj);
	    if (G_asprintf
		(&line, "           E: %10s    W: %10s   Res: %5s", tmp1,
		 tmp2, tmp3) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    format_double(cellhd.top, tmp1);
	    format_double(cellhd.bottom, tmp2);
	    format_double(cellhd.tb_res, tmp3);
	    if (G_asprintf
		(&line, "           T: %10s    B: %10s   Res: %5s", tmp1,
		 tmp2, tmp3) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));
	    if (0 == Rast3d_range_load(g3map))
		G_fatal_error(_("Unable to read range of 3D raster map <%s>"), name);

	    Rast3d_range_min_max(g3map, &dmin, &dmax);
            
            if(dmin != dmin)
                sprintf(tmp1, "%s", "NULL");
            else
	        format_double(dmin, tmp1);
            if(dmax != dmax)
                sprintf(tmp2, "%s", "NULL");
            else
	        format_double(dmax, tmp2);

	    if (G_asprintf
		(&line, "  Range of data:   min = %10s max = %10s", tmp1,
		 tmp2) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));
	}

	printline("");

	if (hist_ok) {
	    printline("  Data Source:");
	    if (G_asprintf(&line, "   %s", Rast_get_history(&hist, HIST_DATSRC_1)) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    if (G_asprintf(&line, "   %s", Rast_get_history(&hist, HIST_DATSRC_2)) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    printline("");

	    printline("  Data Description:");
	    if (G_asprintf(&line, "   %s", Rast_get_history(&hist, HIST_KEYWRD)) > 0)
		printline(line);
	    else
		G_fatal_error(_("Cannot allocate memory for string"));

	    printline("");
	    if (Rast_history_length(&hist)) {
		printline("  Comments:  ");

		for (i = 0; i < Rast_history_length(&hist); i++)

	    /**************************************/
		{
		    if (G_asprintf(&line, "   %s", Rast_history_line(&hist, i)) > 0)
			printline(line);
		    else
			G_fatal_error(_("Cannot allocate memory for string"));

		}
	    }

	    printline("");
	}

	divider('+');

	fprintf(out, "\n");
    }
    else {/* Print information in shell style*/
	if (gflag->answer) {
	    sprintf(tmp1, "%f", cellhd.north);
	    sprintf(tmp2, "%f", cellhd.south);
	    G_trim_decimal(tmp1);
	    G_trim_decimal(tmp2);
	    fprintf(out, "north=%s\n", tmp1);
	    fprintf(out, "south=%s\n", tmp2);

	    sprintf(tmp1, "%f", cellhd.east);
	    sprintf(tmp2, "%f", cellhd.west);
	    G_trim_decimal(tmp1);
	    G_trim_decimal(tmp2);
	    fprintf(out, "east=%s\n", tmp1);
	    fprintf(out, "west=%s\n", tmp2);

	    fprintf(out, "bottom=%g\n", cellhd.bottom);
	    fprintf(out, "top=%g\n", cellhd.top);

	    G_format_resolution(cellhd.ns_res, tmp3, cellhd.proj);
	    fprintf(out, "nsres=%s\n", tmp3);

	    G_format_resolution(cellhd.ew_res, tmp3, cellhd.proj);
	    fprintf(out, "ewres=%s\n", tmp3);

	    fprintf(out, "tbres=%g\n", cellhd.tb_res);

	    fprintf(out, "datatype=\"%s\"\n",
		    data_type == FCELL_TYPE ? "FCELL" :
		    data_type == DCELL_TYPE ? "DCELL" :
		    "??");	   
            
            if (time_ok && (first_time_ok || second_time_ok)) {
		G_format_timestamp(&ts, timebuff);
		fprintf(out, "timestamp=\"%s\"\n", timebuff);
	    }
	    else {
		fprintf(out, "timestamp=\"none\"\n");
	    }
	    fprintf(out, "units=\"%s\"\n", Rast3d_get_unit(g3map));
	    fprintf(out, "vertical_units=\"%s\"\n", Rast3d_get_vertical_unit(g3map));
	    fprintf(out, "tilenumx=%d\n", g3map->nx);
	    fprintf(out, "tilenumy=%d\n", g3map->ny);
	    fprintf(out, "tilenumz=%d\n", g3map->nz);
	    fprintf(out, "tiledimx=%d\n", g3map->tileX);
	    fprintf(out, "tiledimy=%d\n", g3map->tileY);
	    fprintf(out, "tiledimz=%d\n", g3map->tileZ);
	}
	if (rflag->answer) {
	    if (0 == Rast3d_range_load(g3map))
		G_fatal_error(_("Unable to read range of 3D raster map <%s>"), name);

	    Rast3d_range_min_max(g3map, &dmin, &dmax);
            if(dmin != dmin)
	        fprintf(out, "min=NULL\n");
            else
	        fprintf(out, "min=%f\n", dmin);
            if(dmax != dmax)
	        fprintf(out, "max=NULL\n");
            else
	        fprintf(out, "max=%f\n", dmax);
	}
	if (hflag->answer) {
	    if (hist_ok) {
		fprintf(out, "Title:\n");
		fprintf(out, "   %s\n", Rast_get_history(&hist, HIST_TITLE));
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
	    else {
		G_fatal_error(_("Error while reading history file"));
	    }
	}
    }

    /*Close the opened map */
    if (!Rast3d_close(g3map))
	G_fatal_error(_("Unable to close 3D raster map <%s>"), name);


    return 0;
}

/**************************************************************************/
int format_double(double value, char *buf)
{

    sprintf(buf, "%.8lf", value);
    G_trim_decimal(buf);
    return 0;
}
