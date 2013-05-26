/*
 ****************************************************************************
 *
 * MODULE:       r.out.png
 * AUTHOR(S):    Bill Brown - USA-CERL
 *               Alex Shevlakov - sixote@yahoo.com
 *		 Hamish Bowman
 * PURPOSE:      Export GRASS raster as non-georeferenced PNG image.
 * COPYRIGHT:    (C) 2000-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* 
 * Alex Shevlakov, sixote@yahoo.com, 03/2000 
 * based on r.out.ppm by
 * Written by Bill Brown, USA-CERL March 21, 1994
 * 
 * Use to convert grass raster map to PNG
 * uses currently selected region
 *
 */

#include <string.h>
#include <stdlib.h>
#include <float.h>

#ifndef _MYINCLUDE_H
#define _MYINCLUDE_H
#include <png.h>
#include "pngfunc.h"
/* #include <pnm.h> this is already included from pngfunc.h */
#endif /* _MYINCLUDE_H */

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/colors.h>
#include <grass/glocale.h>


typedef int FILEDESC;

/* global functions */
static int write_wld(const char *, const struct Cell_head *);


int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *rast, *png_file, *compr; /* , *bgcolor; */
    struct Flag *alpha, *wld_flag;
    char *rastermap;
    char *basename = NULL, *outfile = NULL;
    unsigned char *set, *ored, *ogrn, *oblu;
    int def_red, def_grn, def_blu;
    CELL *cell_buf;
    FCELL *fcell_buf;
    DCELL *dcell_buf;
    void *voidc;
    int rtype, row, col, do_stdout = 0;
    size_t rsize;
    int png_compr, ret, do_alpha;
    struct Cell_head win;
    FILEDESC cellfile = 0;
    FILE *fp;

    /* now goes from pnmtopng.c* -A.Sh */
    /*
     * pnmtopng.c -
     * read a portable anymap and produce a Portable Network Graphics file
     *
     * derived from pnmtorast.c (c) 1990,1991 by Jef Poskanzer and some
     * parts derived from ppmtogif.c by Marcel Wijkstra <wijkstra@fwi.uva.nl>
     * thanks to Greg Roelofs <newt@pobox.com> for contributions and bug-fixes
     *
     * Copyright (C) 1995-1998 by Alexander Lehmann <alex@hal.rhein-main.de>
     *  		      and Willem van Schaik <willem@schaik.com>
     *
     * Permission to use, copy, modify, and distribute this software and its
     * documentation for any purpose and without fee is hereby granted, provided
     * that the above copyright notice appear in all copies and that both that
     * copyright notice and this permission notice appear in supporting
     * documentation.  This software is provided "as is" without express or
     * implied warranty.
     */

    png_struct *png_ptr;
    png_info *info_ptr;

    png_byte *line;
    png_byte *pp;

    /* these variables are declared static because gcc wasn't kidding
     * about "variable XXX might be clobbered by `longjmp' or `vfork'"
     * (stack corruption observed on Solaris 2.6 with gcc 2.8.1, even
     * in the absence of any other error condition) */
    static xelval maxmaxval;

    static int depth;
    static int filter;

    /* these guys are initialized to quiet compiler warnings: */
    maxmaxval = 255;
    depth = 0;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword("PNG");
    module->description =
	_("Export a GRASS raster map as a non-georeferenced PNG image.");

    rast = G_define_standard_option(G_OPT_R_INPUT);

    png_file = G_define_standard_option(G_OPT_F_OUTPUT);
    png_file->required = YES;
    png_file->description = _("Name for new PNG file (use out=- for stdout)");

    compr = G_define_option();
    compr->key = "compression";
    compr->type = TYPE_INTEGER;
    compr->required = NO;
    compr->multiple = NO;
    compr->options = "0-9";
    compr->label = _("Compression level of PNG file");
    compr->description = _("(0 = none, 1 = fastest, 9 = best)");
    compr->answer = "6";

/*    bgcolor = G_define_standard_option(G_OPT_C_BG); */

    alpha = G_define_flag();
    alpha->key = 't';
    alpha->description = _("Make NULL cells transparent");

    wld_flag = G_define_flag();
    wld_flag->key = 'w';
    wld_flag->description = _("Output world file");

    /* see what can be done to convert'em -A.Sh.
     * gscale = G_define_flag ();
     * gscale->key = 'g';
     * gscale->description = "Output greyscale instead of color";
     */

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    rastermap = rast->answer;

    do_alpha = alpha->answer ? TRUE : FALSE;

    if (strcmp(png_file->answer, "-") != 0)
	basename = G_store(png_file->answer);
    else
	do_stdout = TRUE;

    if (basename) {
	G_basename(basename, "png");
	outfile = G_malloc(strlen(basename) + 5);
	sprintf(outfile, "%s.png", basename);
    }

    png_compr = atoi(compr->answer);

#ifdef MAYBE_LATER
    /* ... if at all */
    ret = G_str_to_color(bgcolor->answer, &def_red, &def_grn, &def_blu);
    if (ret == 0)
	G_fatal_error(_("[%s]: No such color"), bgcolor->answer);
    else if (ret == 2) {  /* (ret==2) is "none" */
	if(!do_alpha)
	    do_alpha = TRUE;
    }
#else
    ret = G_str_to_color(DEFAULT_BG_COLOR, &def_red, &def_grn, &def_blu);
#endif

    /*G_get_set_window (&win); *//* 10/99 MN: check for current region */
    G_get_window(&win);

    G_debug(1, "rows = %d, cols = %d", win.rows, win.cols);

    /* open raster map for reading */
    cellfile = Rast_open_old(rast->answer, "");

    cell_buf = Rast_allocate_c_buf();
    fcell_buf = Rast_allocate_f_buf();
    dcell_buf = Rast_allocate_d_buf();

    ored = G_malloc(win.cols);
    ogrn = G_malloc(win.cols);
    oblu = G_malloc(win.cols);
    set  = G_malloc(win.cols);

    /* open png file for writing */
    if (do_stdout)
	fp = stdout;
    else if (NULL == (fp = fopen(outfile, "w")))
	G_fatal_error(_("Unable to open output file <%s>"), outfile);


    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
				      &pnmtopng_jmpbuf_struct,
				      pnmtopng_error_handler, NULL);
    if (png_ptr == NULL) {
	fclose(fp);
	G_fatal_error("cannot allocate LIBPNG structure");
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
	png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
	fclose(fp);
	G_fatal_error("cannot allocate LIBPNG structure");
    }

    if (setjmp(pnmtopng_jmpbuf_struct.jmpbuf)) {
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	G_fatal_error("setjmp returns error condition (1)");
    }

    depth = 8;			/*really??? */

#ifdef OLDPNG
    png_write_init(png_ptr);
    png_info_init(info_ptr);
#endif
    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, win.cols, win.rows, depth,
		 do_alpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
		 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		 PNG_FILTER_TYPE_DEFAULT);

    /* explicit filter-type (or none) required */
    if ((filter >= 0) && (filter <= 4)) {
	png_set_filter(png_ptr, 0, filter);
    }

    png_set_compression_level(png_ptr, png_compr);

    if (do_alpha) {
	png_color_16 background_color;
	background_color.red = (png_uint_16)def_red;
	background_color.green = (png_uint_16)def_grn;
	background_color.blue = (png_uint_16)def_blu;
	png_set_bKGD(png_ptr, info_ptr, &background_color);
    }

    G_verbose_message(_("Converting <%s>..."), rast->answer);

    {
	struct Colors colors;

	Rast_read_colors(rast->answer, "", &colors);

	rtype = Rast_get_map_type(cellfile);
	if (rtype == CELL_TYPE)
	    voidc = (CELL *) cell_buf;
	else if (rtype == FCELL_TYPE)
	    voidc = (FCELL *) fcell_buf;
	else if (rtype == DCELL_TYPE)
	    voidc = (DCELL *) dcell_buf;
	else
	    G_fatal_error(_("Raster <%s> type mismatch"), rast->answer);

	rsize = Rast_cell_size(rtype);

	/*if(!gscale->answer){ *//* 24BIT COLOR IMAGE */

	if (TRUE) {
	    /* write the png-info struct */
	    png_write_info(png_ptr, info_ptr);

	    /* let libpng take care of, e.g., bit-depth conversions */
	    png_set_packing(png_ptr);

	    /* max: 3 color channels, one alpha channel, 16-bit */
	    line = (png_byte *) G_malloc(win.cols * 8 * sizeof(char));

	    for (row = 0; row < win.rows; row++) {

		G_percent(row, win.rows, 5);
		Rast_get_row(cellfile, (void *)voidc, row, rtype);
		Rast_lookup_colors((void *)voidc, ored, ogrn, oblu, set,
				   win.cols, &colors, rtype);

		pp = line;

		for (col = 0; col < win.cols; col++) {

		    if (set[col]) {
			*pp++ = ored[col];
			*pp++ = ogrn[col];
			*pp++ = oblu[col];
			if (do_alpha) {
			    if (Rast_is_null_value(
				   G_incr_void_ptr(voidc, col * rsize), rtype))
				*pp++ = 0;
			    else
				*pp++ = 255;
			}
		    }
		    else {
			if (do_alpha) {
			    *pp++ = ored[col];
			    *pp++ = ogrn[col];
			    *pp++ = oblu[col];
			    *pp++ = 0;
			}
			else {
			    *pp++ = (unsigned char)def_red;
			    *pp++ = (unsigned char)def_grn;
			    *pp++ = (unsigned char)def_blu;
			}
		    }
		}

		png_write_row(png_ptr, line);

	    }
	    G_percent(row, win.rows, 5); /* finish it off */
	}
	else {			/* GREYSCALE IMAGE */

	    /*    
	     * info_ptr->color_type = PNG_COLOR_TYPE_GRAY;
	     * 
	     */


	    /* pm_message ("don't know yet how to write grey - yumm!!"); */
	    G_warning("don't know how to write grey scale!");
	}

	Rast_free_colors(&colors);

    }

    G_free(cell_buf);
    G_free(fcell_buf);
    G_free(dcell_buf);
    G_free(ored);
    G_free(ogrn);
    G_free(oblu);
    G_free(set);
    Rast_close(cellfile);

    png_write_end(png_ptr, info_ptr);
    /* png_write_destroy (png_ptr); this is no longer supported with libpng, al 11/2000 */
    /* flush first because G_free (png_ptr) can segfault due to jmpbuf problems
     * in png_write_destroy */

    fflush(stdout);
    /* G_free (png_ptr); */
    /* G_free (info_ptr); */
    png_destroy_write_struct(&png_ptr, &info_ptr);	/* al 11/2000 */

    fclose(fp);

    if (wld_flag->answer) {
	if(do_stdout)
	    outfile = G_store("png_map.wld");
	else
	    sprintf(outfile, "%s.wld", basename);

	write_wld(outfile, &win);
    }

    if(basename)
	G_free(basename);
    if(outfile)
	G_free(outfile);

    exit(EXIT_SUCCESS);
}


#ifdef __STDC__
static void pnmtopng_error_handler(png_structp png_ptr, png_const_charp msg)
#else
static void pnmtopng_error_handler(png_ptr, msg)
     png_structp png_ptr;
     png_const_charp msg;
#endif
{
    jmpbuf_wrapper *jmpbuf_ptr;

    /* this function, aside from the extra step of retrieving the "error
     * pointer" (below) and the fact that it exists within the application
     * rather than within libpng, is essentially identical to libpng's
     * default error handler.  The second point is critical:  since both
     * setjmp() and longjmp() are called from the same code, they are
     * guaranteed to have compatible notions of how big a jmp_buf is,
     * regardless of whether _BSD_SOURCE or anything else has (or has not)
     * been defined. */

    G_warning("pnmtopng:  fatal libpng error: [%s]", msg);

    jmpbuf_ptr = png_get_error_ptr(png_ptr);
    if (jmpbuf_ptr == NULL) {	/* we are completely hosed now */
	G_fatal_error
	    ("pnmtopng:  EXTREMELY fatal error: jmpbuf unrecoverable; terminating.");
    }

    longjmp(jmpbuf_ptr->jmpbuf, 1);
}


static int write_wld(const char *fname, const struct Cell_head *win)
{
    int width = DBL_DIG;
    FILE *ofile;

    G_verbose_message(_("Writing world file"));

    if (fname == NULL)
	G_fatal_error(_("Got null file name"));
    if (win == NULL)
	G_fatal_error(_("Got null region struct"));
    if ((ofile = fopen(fname, "w")) == NULL)
	G_fatal_error(_("Unable to open world file for writing"));

    fprintf(ofile, "%36.*f \n", width, win->ew_res);
    fprintf(ofile, "%36.*f \n", width, 0.0);
    fprintf(ofile, "%36.*f \n", width, 0.0);
    fprintf(ofile, "%36.*f \n", width, -1 * win->ns_res);
    fprintf(ofile, "%36.*f \n", width, win->west + win->ew_res / 2.0);
    fprintf(ofile, "%36.*f \n", width, win->north - win->ns_res / 2.0);

    fclose(ofile);
    return 0;
}
