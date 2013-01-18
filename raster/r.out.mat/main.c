/*
 * r.out.mat
 *
 * Output a GRASS raster map to a MAT-File (version 4).
 *
 *   Copyright (C) 2004 by the GRASS Development Team
 *   Author: Hamish Bowman, University of Otago, New Zealand
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *
 *   Code follows r.out.bin to a certain extent, which in turn
 *   follows r.out.ascii.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{

    int i, row, col;		/* counters */
    unsigned long filesize;

    int endianness;		/* 0=little, 1=big */
    int data_format;		/* 0=double  1=float  2=32bit signed int  5=8bit unsigned int (ie text) */
    int data_type;		/* 0=numbers  1=text */
    int format_block;		/* combo of endianness, 0, data_format, and type */
    int realflag = 0;		/* 0=only real values used */

    /* should type be specifically uint32 ??? */

    char array_name[32];	/* variable names must start with a letter (case 
				   sensitive) followed by letters, numbers, or 
				   underscores. 31 chars max. */
    int name_len;
    int mrows, ncols;		/* text/data/map array dimensions */

    int val_i;			/* for misc use */
    float val_f;		/* for misc use */
    double val_d;		/* for misc use */

    char *infile, *outfile, *maptitle, *basename;
    struct Cell_head region;
    void *raster, *ptr;
    RASTER_MAP_TYPE map_type;

    struct Option *inputfile, *outputfile;
    struct GModule *module;

    int fd;
    FILE *fp1;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    module->description = _("Exports a GRASS raster to a binary MAT-File.");

    /* Define the different options */

    inputfile = G_define_standard_option(G_OPT_R_INPUT);

    outputfile = G_define_standard_option(G_OPT_F_OUTPUT);
    outputfile->required = YES;
    outputfile->gisprompt = "new,bin,file";
    outputfile->description = _("Name for output binary MAT file");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    infile = inputfile->answer;
    basename = G_store(outputfile->answer);
    G_basename(basename, "mat");
    outfile = G_malloc(strlen(basename) + 5);
    sprintf(outfile, "%s.mat", basename);

    fd = Rast_open_old(infile, "");

    map_type = Rast_get_map_type(fd);

    /* open bin file for writing */
    fp1 = fopen(outfile, "wb");
    if (NULL == fp1)
	G_fatal_error(_("Unable to open output file <%s>"), outfile);


    /* Check Endian State of Host Computer */
    if (G_is_little_endian())
	endianness = 0;		/* ie little endian */
    else
	endianness = 1;		/* ie big endian */
    G_debug(1, "Machine is %s endian.\n", endianness ? "big" : "little");

    G_get_window(&region);


    /********** Write map **********/

    /** write text element (map name) **/
    strncpy(array_name, "map_name", 31);
    mrows = 1;
    ncols = strlen(infile);
    data_format = 5;		/* 0=double  1=float  2=32bit signed int  5=8bit unsigned int(text) */
    data_type = 1;		/* 0=numbers  1=text */

    G_verbose_message(_("Exporting <%s>"), infile);

    /* 4 byte data format */
    format_block = endianness * 1000 + data_format * 10 + data_type;
    fwrite(&format_block, sizeof(int), 1, fp1);
    /* fprintf(stderr, "name data format is [%04ld]\n", format_block); */

    /* 4 byte number of rows & columns */
    fwrite(&mrows, sizeof(int), 1, fp1);
    fwrite(&ncols, sizeof(int), 1, fp1);

    /* 4 byte real/imag flag   0=real vals only */
    fwrite(&realflag, sizeof(int), 1, fp1);

    /* length of array_name+1 */
    name_len = strlen(array_name) + 1;
    fwrite(&name_len, sizeof(int), 1, fp1);

    /* array name */
    fprintf(fp1, "%s%c", array_name, '\0');

    /* array data */
    fprintf(fp1, "%s", infile);


    /********** Write title (if there is one) **********/
    maptitle = Rast_get_cell_title(infile, "");
    if (strlen(maptitle) >= 1) {

	/** write text element (map title) **/
	strncpy(array_name, "map_title", 31);
	mrows = 1;
	ncols = strlen(maptitle);
	data_format = 5;	/* 0=double  1=float  2=32bit signed int  5=8bit unsigned int(text) */
	data_type = 1;		/* 0=numbers  1=text */

	/* 4 byte data format */
	format_block = endianness * 1000 + data_format * 10 + data_type;
	fwrite(&format_block, sizeof(int), 1, fp1);

	/* 4 byte number of rows & columns */
	fwrite(&mrows, sizeof(int), 1, fp1);
	fwrite(&ncols, sizeof(int), 1, fp1);

	/* 4 byte real/imag flag   0=real vals only */
	fwrite(&realflag, sizeof(int), 1, fp1);

	/* length of array_name+1 */
	name_len = strlen(array_name) + 1;
	fwrite(&name_len, sizeof(int), 1, fp1);

	/* array name */
	fprintf(fp1, "%s%c", array_name, '\0');

	/* array data */
	fprintf(fp1, "%s", maptitle);
    }

    /***** Write bounds *****/
    G_verbose_message("");
    G_verbose_message(_("Using the Current Region settings:"));
    G_verbose_message(_("northern edge=%f"), region.north);
    G_verbose_message(_("southern edge=%f"), region.south);
    G_verbose_message(_("eastern edge=%f"), region.east);
    G_verbose_message(_("western edge=%f"), region.west);
    G_verbose_message(_("nsres=%f"), region.ns_res);
    G_verbose_message(_("ewres=%f"), region.ew_res);
    G_verbose_message(_("rows=%d"), region.rows);
    G_verbose_message(_("cols=%d"), region.cols);
    G_verbose_message("");

    for (i = 0; i < 4; i++) {
	switch (i) {
	case 0:
	    strncpy(array_name, "map_northern_edge", 31);
	    val_d = region.north;
	    break;
	case 1:
	    strncpy(array_name, "map_southern_edge", 31);
	    val_d = region.south;
	    break;
	case 2:
	    strncpy(array_name, "map_eastern_edge", 31);
	    val_d = region.east;
	    break;
	case 3:
	    strncpy(array_name, "map_western_edge", 31);
	    val_d = region.west;
	    break;
	default:
	    fclose(fp1);
	    G_fatal_error("please contact development team");
	    break;
	}

	/** write data element **/
	data_format = 0;	/* 0=double  1=float  2=32bit signed int  5=8bit unsigned int(text) */
	data_type = 0;		/* 0=numbers  1=text */
	mrows = 1;
	ncols = 1;

	/* 4 byte data format */
	format_block = endianness * 1000 + data_format * 10 + data_type;
	fwrite(&format_block, sizeof(int), 1, fp1);
	/* fprintf(stderr, "bounds data format is [%04ld]\n", format_block); */

	/* 4 byte number of rows , 4 byte number of colums */
	fwrite(&mrows, sizeof(int), 1, fp1);
	fwrite(&ncols, sizeof(int), 1, fp1);

	/* 4 byte real/imag flag   0=only real */
	fwrite(&realflag, sizeof(int), 1, fp1);

	/* length of array_name+1 */
	name_len = strlen(array_name) + 1;
	fwrite(&name_len, sizeof(int), 1, fp1);

	/* array name */
	fprintf(fp1, "%s%c", array_name, '\0');

	/* write array data, by increasing column */
	fwrite(&val_d, sizeof(double), 1, fp1);

	/** end of data element **/
    }



    /***** Write map data *****/
    strncpy(array_name, "map_data", 31);

    switch (map_type) {		/* data_format: 0=double  1=float  2=32bit signed int  5=8bit unsigned int (ie text) */

    case CELL_TYPE:
	data_format = 2;
	G_verbose_message(_("Exporting raster as integer values"));
	break;

    case FCELL_TYPE:
	data_format = 1;
	G_verbose_message(_("Exporting raster as floating point values"));
	break;

    case DCELL_TYPE:
	data_format = 0;
	G_verbose_message(_("Exporting raster as double FP values"));
	break;

    default:
	fclose(fp1);
	G_fatal_error("Please contact development team");
	break;
    }

    data_type = 0;		/* 0=numbers  1=text */

    mrows = region.rows;
    ncols = region.cols;

    /* 4 byte data format */
    format_block = (endianness * 1000) + (data_format * 10) + data_type;
    fwrite(&format_block, sizeof(int), 1, fp1);

    G_debug(3, "map data format is [%04d]\n", format_block);

    /* 4 byte number of rows & columns */
    fwrite(&mrows, sizeof(int), 1, fp1);
    fwrite(&ncols, sizeof(int), 1, fp1);

    /* 4 byte real/imag flag   0=only real */
    fwrite(&realflag, sizeof(int), 1, fp1);

    /* length of array_name+1 */
    name_len = strlen(array_name) + 1;
    fwrite(&name_len, sizeof(int), 1, fp1);

    /* array name */
    fprintf(fp1, "%s%c", array_name, '\0');

    /* data array, by increasing column */
    raster =
	G_calloc((Rast_window_rows() + 1) * (Rast_window_cols() + 1),
		 Rast_cell_size(map_type));

    G_debug(1, "mem alloc is %d bytes\n",	/* I think _cols()+1 is unneeded? */
	    Rast_cell_size(map_type) * (Rast_window_rows() +
				       1) * (Rast_window_cols() + 1));

    G_verbose_message(_("Reading in map ... "));

    /* load entire map into memory */
    for (row = 0, ptr = raster; row < mrows; row++,
	 ptr =
	 G_incr_void_ptr(ptr,
			 (Rast_window_cols() + 1) * Rast_cell_size(map_type))) {
	Rast_get_row(fd, ptr, row, map_type);
	G_percent(row, mrows, 2);
    }
    G_percent(row, mrows, 2);	/* finish it off */


    G_verbose_message(_("Writing out map..."));

    /* then write it to disk */
    /* NoGood: fwrite(raster, Rast_cell_size(map_type), mrows*ncols, fp1); */
    for (col = 0; col < ncols; col++) {
	for (row = 0; row < mrows; row++) {

	    ptr = raster;
	    ptr =
		G_incr_void_ptr(ptr,
				(col +
				 row * (ncols +
					1)) * Rast_cell_size(map_type));

	    if (!Rast_is_null_value(ptr, map_type)) {
		if (map_type == CELL_TYPE) {
		    val_i = *((CELL *) ptr);
		    fwrite(&val_i, sizeof(int), 1, fp1);
		}
		else if (map_type == FCELL_TYPE) {
		    val_f = *((FCELL *) ptr);
		    fwrite(&val_f, sizeof(float), 1, fp1);
		}
		else if (map_type == DCELL_TYPE) {
		    val_d = *((DCELL *) ptr);
		    fwrite(&val_d, sizeof(double), 1, fp1);
		}
	    }
	    else {		/* ie if NULL cell -> write IEEE NaN value */
		if (map_type == CELL_TYPE) {
		    val_i = *((CELL *) ptr);	/* int has no NaN value, so use whatever GRASS uses */
		    fwrite(&val_i, sizeof(int), 1, fp1);
		}
		else if (map_type == FCELL_TYPE) {
		    if (endianness)	/* ie big */
			fprintf(fp1, "%c%c%c%c", 0xff, 0xf8, 0, 0);
		    else	/* ie little */
			fprintf(fp1, "%c%c%c%c", 0, 0, 0xf8, 0xff);
		}
		else if (map_type == DCELL_TYPE) {
		    if (endianness)
			fprintf(fp1, "%c%c%c%c%c%c%c%c", 0xff, 0xf8, 0, 0, 0,
				0, 0, 0);
		    else
			fprintf(fp1, "%c%c%c%c%c%c%c%c", 0, 0, 0, 0, 0, 0,
				0xf8, 0xff);
		}
	    }
	}
	G_percent(col, ncols, 2);
    }
    G_percent(col, ncols, 2);	/* finish it off */

    /*** end of data element ***/


    /* done! */
    filesize = G_ftell(fp1);
    fclose(fp1);

    G_verbose_message(_("%ld bytes written to '%s'"), filesize, outfile);

    G_done_msg("");

    G_free(basename);
    G_free(outfile);

    exit(EXIT_SUCCESS);
}
