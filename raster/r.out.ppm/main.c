
/****************************************************************************
 *
 * MODULE:       r.out.ppm
 * AUTHOR(S):    Bill Brown, USA-CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jachym Cepicky <jachym les-ejk.cz>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      converts a GRASS raster map into a PPM image (obeying REGION)
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define DEF_RED 255
#define DEF_GRN 255
#define DEF_BLU 255

typedef int FILEDESC;

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *rast, *ppm_file;
    struct Flag *gscale, *header;
    char *map, *p, ofile[1000];
    unsigned char *set, *ored, *ogrn, *oblu;
    CELL *cell_buf;
    FCELL *fcell_buf;
    DCELL *dcell_buf;
    void *voidc;
    int rtype, row, col, do_stdout = 0;
    struct Cell_head w;
    FILEDESC cellfile = 0;
    FILE *fp;
    char *tmpstr1, *tmpstr2;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    module->description = _("Converts a GRASS raster map to a PPM image file.");


    rast = G_define_standard_option(G_OPT_R_INPUT);

    ppm_file = G_define_standard_option(G_OPT_F_OUTPUT);
    ppm_file->required = NO;
    ppm_file->answer = "<rasterfilename>.ppm";
    ppm_file->description = _("Name for new PPM file (use '-' for stdout)");

    gscale = G_define_flag();
    gscale->key = 'g';
    gscale->description = _("Output greyscale instead of color");

    header = G_define_flag();
    header->key = 'h';
    header->description = _("Suppress printing of PPM header");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* kludge to work with r.out.mpeg */
    if (rast->answer[0] == '/')
	rast->answer++;

    if (strcmp(ppm_file->answer, "<rasterfilename>.ppm")) {
	if (strcmp(ppm_file->answer, "-"))
	    strcpy(ofile, ppm_file->answer);
	else
	    do_stdout = 1;
    }
    else {
	map = p = rast->answer;
	/* knock off any GRASS location suffix */
	if ((char *)NULL != (p = strrchr(map, '@'))) {
	    if (p != map)
		*p = '\0';
	}
	strcpy(ofile, map);
	strcat(ofile, ".ppm");
    }

    /*G_get_set_window (&w); *//* 10/99 MN: check for current region */
    G_get_window(&w);

    G_asprintf(&tmpstr1, n_("row = %d", "rows = %d", w.rows), w.rows);
    G_asprintf(&tmpstr2, n_("column = %d", "columns = %d", w.cols), w.cols);
    G_message("%s, %s", tmpstr1, tmpstr2);
    G_free(tmpstr1);
    G_free(tmpstr2); 

    /* open raster map for reading */
    cellfile = Rast_open_old(rast->answer, "");

    cell_buf = Rast_allocate_c_buf();
    fcell_buf = Rast_allocate_f_buf();
    dcell_buf = Rast_allocate_d_buf();

    ored = G_malloc(w.cols);
    ogrn = G_malloc(w.cols);
    oblu = G_malloc(w.cols);
    set  = G_malloc(w.cols);

    /* open ppm file for writing */
    {
	if (do_stdout)
	    fp = stdout;
	else if (NULL == (fp = fopen(ofile, "w"))) {
	    G_fatal_error(_("Unable to open file <%s> for write"), ofile);
	}
    }
    /* write header info */
    if (!header->answer) {
	if (!gscale->answer)
	    fprintf(fp, "P6\n");
	/* Magic number meaning rawbits, 24bit color to ppm format */
	else
	    fprintf(fp, "P5\n");
	/* Magic number meaning rawbits, 8bit greyscale to ppm format */

	if (!do_stdout) {
	    fprintf(fp, "# CREATOR: %s from GRASS raster map \"%s\"\n",
	            G_program_name(), rast->answer);
	    fprintf(fp, "# east-west resolution: %f\n", w.ew_res);
	    fprintf(fp, "# north-south resolution: %f\n", w.ns_res);
	    fprintf(fp, "# South edge: %f\n", w.south);
	    fprintf(fp, "# West edge: %f\n", w.west);
	    /* comments */
	}

	fprintf(fp, "%d %d\n", w.cols, w.rows);
	/* width & height */

	fprintf(fp, "255\n");
	/* max intensity val */
    }


    G_important_message(_("Converting..."));

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
	    exit(1);

	if (!gscale->answer) {	/* 24BIT COLOR IMAGE */
	    for (row = 0; row < w.rows; row++) {
		G_percent(row, w.rows, 5);
		Rast_get_row(cellfile, (void *)voidc, row, rtype);
		Rast_lookup_colors((void *)voidc, ored, ogrn, oblu, set,
				   w.cols, &colors, rtype);

		for (col = 0; col < w.cols; col++) {
		    if (set[col]) {
			putc(ored[col], fp);
			putc(ogrn[col], fp);
			putc(oblu[col], fp);
		    }
		    else {
			putc(DEF_RED, fp);
			putc(DEF_GRN, fp);
			putc(DEF_BLU, fp);
		    }
		}
	    }
	}
	else {			/* GREYSCALE IMAGE */
	    for (row = 0; row < w.rows; row++) {

		G_percent(row, w.rows, 5);
		Rast_get_row(cellfile, (void *)voidc, row, rtype);
		Rast_lookup_colors((void *)voidc, ored, ogrn, oblu, set,
				   w.cols, &colors, rtype);

		for (col = 0; col < w.cols; col++) {

#ifdef XV_STYLE
		    /*.33R+ .5G+ .17B */
		    putc((((ored[col]) * 11 + (ogrn[col]) * 16
			   + (oblu[col]) * 5) >> 5), fp);
#else
		    /*NTSC Y equation: .30R+ .59G+ .11B */
		    putc((((ored[col]) * 19 + (ogrn[col]) * 38
			   + (oblu[col]) * 7) >> 6), fp);
#endif
		}
	    }
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
    /*
       if(!do_stdout)
     */
    fclose(fp);

    if (do_stdout)
	G_done_msg("%s", "");
    else
	G_done_msg(_("File <%s> created"), ofile);

    return (EXIT_SUCCESS);
}
