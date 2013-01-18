
/****************************************************************************
 *
 * MODULE:       r.out.tiff
 *   
 * AUTHOR(S):    Sam Leffler
 *               Updated by Marco Valagussa <marco duffy.crcc.it>,
 *               Markus Neteler, Eric G. Miller, Luca Cristelli
 *
 * PURPOSE:      Exports a GRASS raster map to a 8/24bit TIFF image file
 *               at the pixel resolution of the currently defined region.

 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* 
 * Added support for Tiled TIFF output ( -l switch )
 * Luca Cristelli (luca.cristelli ies.it) 1/2001
 * 
 * Added flag to write a TIFF World file like r.out.arctiff
 * Eric G. Miller 4-Nov-2000
 *
 * Corrected Rast_set_window to G_get_window to make r.out.tiff sensitive
 * to region settings.   - Markus Neteler  (neteler geog.uni-hannover.de
 * 8/98        
 *
 * This r.tiff version uses the standard libtiff from your system.
 *  8. June 98 Marco Valagussa <marco duffy.crcc.it>
 *
 * Original version:
 * Portions Copyright (c) 1988, 1990 by Sam Leffler.
 * All rights reserved.
 *
 * This file is provided for unrestricted use provided that this
 * legend is included on all tape media and as a part of the
 * software program in whole or part.  Users may copy, modify or
 * distribute this file at will.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <sys/types.h>
#include <tiffio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "rasterfile.h"

/* global variables */

#define MAX_TILE_LENGTH 512

#define	howmany(x, y)	(((x)+((y)-1))/(y))
#define	streq(a,b)	(strcmp(a,b) == 0)

unsigned short config = PLANARCONFIG_CONTIG;
unsigned short compression = -1;
unsigned short rowsperstrip = 0;

/* global functions */
static int write_tfw(const char *, const struct Cell_head *);

int main(int argc, char *argv[])
{
    unsigned char *buf, *tmpptr;
    int row, linebytes;
    TIFF *out;
    int in;
    struct rasterfile h;
    struct Option *inopt, *outopt, *compopt;
    struct Flag *pflag, *lflag, *tflag, *wflag;
    CELL *cell, *cellptr, *cells[MAX_TILE_LENGTH];
    struct Cell_head cellhd;
    struct GModule *module;
    int col, tfw, palette, tiled;
    char *basename, *filename;
    struct Colors colors;
    int red, grn, blu, mapsize, isfp;
/*    int do_alpha; */

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    module->description =
	_("Exports a GRASS raster map to a 8/24bit TIFF image file.");

    inopt = G_define_standard_option(G_OPT_R_INPUT);

    outopt = G_define_standard_option(G_OPT_F_OUTPUT);
    outopt->required = YES;
    outopt->gisprompt = "new,bin,file";
    outopt->description = _("Name for output TIFF file");

    compopt = G_define_option();
    compopt->key = "compression";
    compopt->type = TYPE_STRING;
    compopt->required = NO;
    compopt->options = "none,packbit,deflate,lzw";
    compopt->description = _("TIFF file compression");
    compopt->answer = "none";

    pflag = G_define_flag();
    pflag->key = 'p';
    pflag->description = _("TIFF Palette output (8bit instead of 24bit).");

/* TODO (copy method from r.out.png)
    tflag = G_define_flag();
    tflag->key = 't';
    tflag->description = _("Make NULL cells transparent");
*/
    wflag = G_define_flag();
    wflag->key = 'w';
    wflag->description = _("Output TIFF world file");

    lflag = G_define_flag();
    lflag->key = 'l';
    lflag->description = _("Output Tiled TIFF");

/*todo?   bgcolor = G_define_standard_option(G_OPT_C_BG); */

    /*todo:
     * gscale = G_define_flag ();
     * gscale->key = 'g';
     * gscale->description = "Output greyscale instead of color";
     */

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (strncmp(compopt->answer, "packbit", 7) == 0)
	compression = COMPRESSION_PACKBITS;
    else if (strncmp(compopt->answer, "deflate", 7) == 0)
	compression = COMPRESSION_DEFLATE;
    else if (strncmp(compopt->answer, "lzw", 3) == 0)
	compression = COMPRESSION_LZW;
    else
	compression = COMPRESSION_NONE;

    tiled = lflag->answer;
    palette = pflag->answer;
    tfw = wflag->answer;

/*
    if(alpha->answer && pflag->answer)
	G_fatal_error(_("Palletted images do not support transparency."));
    do_alpha = alpha->answer ? TRUE : FALSE;
*/
#ifdef MAYBE_LATER
    /* ... if at all */
    ret = G_str_to_color(bgcolor->answer, &def_red, &def_grn, &def_blu);
    if (ret == 0)
	G_fatal_error(_("[%s]: No such color"), bgcolor->answer);
    else if (ret == 2) {  /* (ret==2) is "none" */
	if(!do_alpha)
	    do_alpha = TRUE;
    }
#endif

    Rast_get_cellhd(inopt->answer, "", &cellhd);

    G_get_window(&cellhd);

    Rast_read_colors(inopt->answer, "", &colors);
    if ((isfp = Rast_map_is_fp(inopt->answer, "")))
	G_warning(_("Raster map <%s>> is a floating point "
		    "map. Fractional values will be rounded to integer"),
		  inopt->answer);

    Rast_set_null_value_color(255, 255, 255, &colors);
    if (palette && (colors.cmax - colors.cmin > 255))
	G_fatal_error(_("Color map for palette must have less "
			"than 256 colors for the available range of data"));

    cell = Rast_allocate_c_buf();
    in = Rast_open_old(inopt->answer, "");

    basename = G_store(outopt->answer);
    G_basename(basename, "tiff");
    G_basename(basename, "tif");
    filename = G_malloc(strlen(basename) + 5);
    sprintf(filename, "%s.tif", basename);

    out = TIFFOpen(filename, "w");
    if (out == NULL)
	G_fatal_error(_("Unable to open TIFF file <%s>"), filename);

    h.ras_width = cellhd.cols;
    h.ras_height = cellhd.rows;
    h.ras_depth = 24;
    if (pflag->answer)
	h.ras_depth = 8;

    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, h.ras_depth > 8 ? 3 : 1);
/*
    if(do_alpha) {
	h.ras_depth = 32;
	uint16 extras[] = { EXTRASAMPLE_ASSOCALPHA };
	TIFFSetField(out, TIFFTAG_EXTRASAMPLES, 1, extras);
	// ? -or- ?
	TIFFSetField(out, TIFFTAG_EXTRASAMPLES, EXTRASAMPLE_ASSOCALPHA);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 4);
    }
*/
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, h.ras_width);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, h.ras_height);
    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, h.ras_depth > 1 ? 8 : 1);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
    mapsize = 1 << h.ras_depth;

    if (palette) {
	unsigned short *redp, *grnp, *blup, *mapptr;
	int i;

	G_debug(1, "max %f min %f mapsize %d",
		colors.cmax, colors.cmin, mapsize);

	mapptr = (unsigned short *) G_calloc(mapsize * 3, sizeof(unsigned short));
	redp = mapptr;
	grnp = redp + mapsize;
	blup = redp + mapsize * 2;

	/* XXX -- set pointers up before we step through arrays */
#define	SCALE(x)	(((x)*((1L<<16)-1))/255)

	for (i = colors.cmin; i <= colors.cmax; i++, redp++, grnp++, blup++) {
	    Rast_get_c_color(&i, &red, &grn, &blu, &colors);
	    *redp = (unsigned short) (SCALE(red));
	    *grnp = (unsigned short) (SCALE(grn));
	    *blup = (unsigned short) (SCALE(blu));

	    G_debug(1, " %d : %d %d %d   %d %d %d",
		    i, red, grn, blu, *redp, *grnp, *blup);
	}

	TIFFSetField(out, TIFFTAG_COLORMAP,
		     mapptr, mapptr + mapsize, mapptr + mapsize * 2);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
    }
    else {
	/* XXX this is bogus... */
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, h.ras_depth == 24 ?
		     PHOTOMETRIC_RGB : PHOTOMETRIC_MINISBLACK);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
    }

    if (tiled) {
	int tilewidth = 128;
	int tilelength = 128;
	int imagewidth, imagelength;
	int spp;
	char *obuf;
	char *tptr;
	uint32 col, oskew, width, i, j;

	imagewidth = h.ras_width;
	imagelength = h.ras_height;
	spp = h.ras_depth;

	TIFFSetField(out, TIFFTAG_TILEWIDTH, tilewidth);
	TIFFSetField(out, TIFFTAG_TILELENGTH, tilelength);
	obuf = (char *)_TIFFmalloc(TIFFTileSize(out));

	G_debug(1, "Tile buff size: %d", TIFFTileSize(out));

	/* allocate cell buffers */
	for (i = 0; i < tilelength; i++)
	    cells[i] = Rast_allocate_c_buf();

	/* build tiff tiles from grass buffer */
	for (row = 0; row < imagelength; row += tilelength) {
	    uint32 nrow =
		(row + tilelength >
		 imagelength) ? imagelength - row : tilelength;
	    uint32 colb = 0;

	    for (i = 0; i < nrow; i++)
		Rast_get_c_row(in, cells[i], row + i);

	    for (col = 0; col < imagewidth; col += tilewidth) {
		tsample_t s;

		i = nrow;
		tptr = obuf;
		spp = 1;
		s = 0;
		oskew = 0;
		width = tilewidth;

		G_debug(1, "Tile #: r %d, c %d, s %d", row, col, s);

		/*
		 * Tile is clipped horizontally.  Calculate
		 * visible portion and skewing factors.
		 */
		if (colb + tilewidth > imagewidth) {
		    width = (imagewidth - colb);
		    oskew = tilewidth - width;
		}

		for (i = 0; i < nrow; i++) {
		    cellptr = cells[i];

		    if (palette) {
			cellptr += col;
			for (j = 0; j < width; j++)
			    *tptr++ = (unsigned char) * cellptr++;

			tptr += oskew;
		    }
		    else {
			for (j = 0; j < width; j++) {
			    Rast_get_c_color(&(cellptr[col + j]), &red, &grn, &blu,
					     &colors);
			    *tptr++ = (unsigned char) red;
			    *tptr++ = (unsigned char) grn;
			    *tptr++ = (unsigned char) blu;
			}

			tptr += oskew * 3;
		    }

		    G_debug(3, "row #: i %d tptr %lx", i, (long)tptr);
		}

		G_debug(1, "Write Tile #: col %d row %d s %d", col, row, s);

		if (TIFFWriteTile(out, obuf, col, row, 0, s) < 0) {
		    _TIFFfree(obuf);
		    return (-1);
		}

		G_percent(row, h.ras_height, 1);
	    }

	    colb += tilewidth;
	}
    }
    else {
	linebytes = ((h.ras_depth * h.ras_width + 15) >> 3) & ~1;

	G_debug(1, "linebytes = %d, TIFFscanlinesize = %d", linebytes,
		TIFFScanlineSize(out));

	if (TIFFScanlineSize(out) > linebytes)
	    buf = (unsigned char *) G_malloc(linebytes);
	else
	    buf = (unsigned char *) G_malloc(TIFFScanlineSize(out));

	if (rowsperstrip != (unsigned short) - 1)
	    rowsperstrip = (unsigned short) (8 * 1024 / linebytes);

	G_debug(1, "rowsperstrip = %d", rowsperstrip);

	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
		     rowsperstrip == 0 ? 1 : rowsperstrip);

	for (row = 0; row < h.ras_height; row++) {
	    tmpptr = buf;

	    G_percent(row, h.ras_height, 2);

	    Rast_get_c_row(in, cell, row);

	    cellptr = cell;
	    if (palette) {
		for (col = 0; col < h.ras_width; col++)
		    *tmpptr++ = (unsigned char) (*cellptr++ - colors.cmin);
	    }
	    else {
		for (col = 0; col < h.ras_width; col++) {
		    Rast_get_c_color(&(cell[col]), &red, &grn, &blu, &colors);
		    *tmpptr++ = (unsigned char) red;
		    *tmpptr++ = (unsigned char) grn;
		    *tmpptr++ = (unsigned char) blu;
		}
	    }

	    if (TIFFWriteScanline(out, buf, row, 0) < 0)
		break;
	}

	G_percent(row, h.ras_height, 2);
    }

    (void)TIFFClose(out);

    if (tfw) {
	sprintf(filename, "%s.tfw", basename);
	write_tfw(filename, &cellhd);
    }

    G_free(filename);
    G_free(basename);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}


static int write_tfw(const char *fname, const struct Cell_head *win)
{
    int width = DBL_DIG;
    FILE *outfile;

    G_message(_("Writing TIFF World file"));

    if (fname == NULL)
	G_fatal_error(_("Got null file name"));
    if (win == NULL)
	G_fatal_error(_("Got null region struct"));

    if ((outfile = fopen(fname, "w")) == NULL)
	G_fatal_error(_("Unable to open TIFF world file for writing"));

    fprintf(outfile, "%36.*f \n", width, win->ew_res);
    fprintf(outfile, "%36.*f \n", width, 0.0);
    fprintf(outfile, "%36.*f \n", width, 0.0);
    fprintf(outfile, "%36.*f \n", width, -1 * win->ns_res);
    fprintf(outfile, "%36.*f \n", width, win->west + win->ew_res / 2.0);
    fprintf(outfile, "%36.*f \n", width, win->north - win->ns_res / 2.0);

    fclose(outfile);
    return 0;
}
