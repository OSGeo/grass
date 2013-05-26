
/****************************************************************************
 *
 * MODULE:       r.in.arc
 * AUTHOR(S):    Unknown German author,
 *                updated by Bill Brown to floating point support (original contributors)
 *               Markus Neteler <neteler itc.it>, Huidae Cho <grass4u gmail.com>,
 *               Roberto Flor <flor itc.it>, Jachym Cepicky <jachym les-ejk.cz>,
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      Import an ESRI ARC/INFO ascii raster file
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/config.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

FILE *Tmp_fd = NULL;
char *Tmp_file = NULL;

int main(int argc, char *argv[])
{
    char *input;
    char *output;
    char *title;
    FILE *fd;
    int cf;
    struct Cell_head cellhd;
    CELL *cell;
    FCELL *fcell;
    DCELL *dcell;
    int row, col;
    int nrows, ncols;
    static int missingval;
    int rtype;
    double mult_fact;
    double x;
    struct GModule *module;
    struct History history;
    struct
    {
	struct Option *input, *output, *type, *title, *mult;
    } parm;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    G_add_keyword("ASCII");
    module->description =
	_("Converts an ESRI ARC/INFO ascii raster file (GRID) into a GRASS raster map.");
    
    parm.input = G_define_standard_option(G_OPT_F_INPUT);
    parm.input->description =
	_("Name of ARC/INFO ASCII raster file (GRID) to be imported");
    
    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.type = G_define_option();
    parm.type->key = "type";
    parm.type->type = TYPE_STRING;
    parm.type->required = NO;
    parm.type->options = "CELL,FCELL,DCELL";
    parm.type->answer = "FCELL";
    parm.type->description = _("Storage type for resultant raster map");

    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title for resultant raster map");

    parm.mult = G_define_option();
    parm.mult->key = "mult";
    parm.mult->type = TYPE_DOUBLE;
    parm.mult->answer = "1.0";
    parm.mult->required = NO;
    parm.mult->description = _("Multiplier for ASCII data");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    input = parm.input->answer;
    output = parm.output->answer;
    title = parm.title->answer;
    if (title)
	G_strip(title);

    sscanf(parm.mult->answer, "%lf", &mult_fact);
    if (strcmp("CELL", parm.type->answer) == 0)
	rtype = CELL_TYPE;
    else if (strcmp("DCELL", parm.type->answer) == 0)
	rtype = DCELL_TYPE;
    else
	rtype = FCELL_TYPE;

    if (strcmp("-", input) == 0) {
	Tmp_file = G_tempfile();
	if (NULL == (Tmp_fd = fopen(Tmp_file, "w+")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file);
	unlink(Tmp_file);
	if (0 > file_cpy(stdin, Tmp_fd))
	    exit(EXIT_FAILURE);
	fd = Tmp_fd;
    }
    else
	fd = fopen(input, "r");

    if (fd == NULL)
	G_fatal_error(_("Unable to open input file <%s>"), input);

    if (!gethead(fd, &cellhd, &missingval))
	G_fatal_error(_("Can't get cell header"));

    nrows = cellhd.rows;
    ncols = cellhd.cols;
    Rast_set_window(&cellhd);

    if (nrows != Rast_window_rows())
	G_fatal_error(_("OOPS: rows changed from %d to %d"), nrows,
		      Rast_window_rows());
    if (ncols != Rast_window_cols())
	G_fatal_error(_("OOPS: cols changed from %d to %d"), ncols,
		      Rast_window_cols());

    switch (rtype) {
    case CELL_TYPE:
	cell = Rast_allocate_c_buf();
	break;
    case FCELL_TYPE:
	fcell = Rast_allocate_f_buf();
	break;
    case DCELL_TYPE:
	dcell = Rast_allocate_d_buf();
	break;
    }
    cf = Rast_open_new(output, rtype);

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 5);
	for (col = 0; col < ncols; col++) {
	    if (fscanf(fd, "%lf", &x) != 1) {
		Rast_unopen(cf);
		G_fatal_error(_("Data conversion failed at row %d, col %d"),
			      row + 1, col + 1);
	    }
	    switch (rtype) {
	    case CELL_TYPE:
		if ((int)x == missingval)
		    Rast_set_c_null_value(cell + col, 1);
		else
		    cell[col] = (CELL) x *mult_fact;

		break;
	    case FCELL_TYPE:
		if ((int)x == missingval)
		    Rast_set_f_null_value(fcell + col, 1);
		else
		    fcell[col] = (FCELL) x *mult_fact;

		break;
	    case DCELL_TYPE:
		if ((int)x == missingval)
		    Rast_set_d_null_value(dcell + col, 1);
		else
		    dcell[col] = (DCELL) x *mult_fact;

		break;
	    }
	}
	switch (rtype) {
	case CELL_TYPE:
	    Rast_put_c_row(cf, cell);
	    break;
	case FCELL_TYPE:
	    Rast_put_f_row(cf, fcell);
	    break;
	case DCELL_TYPE:
	    Rast_put_d_row(cf, dcell);
	    break;
	}
    }
    /* G_message(_("CREATING SUPPORT FILES FOR %s"), output); */
    Rast_close(cf);
    if (title)
	Rast_put_cell_title(output, title);
    Rast_short_history(output, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(output, &history);


    exit(EXIT_SUCCESS);
}

int file_cpy(FILE * from, FILE * to)
{
    char buf[BUFSIZ];
    long size;
    int written = 0;

    while (1) {
	size = fread(buf, 1, BUFSIZ, from);
	if (!size) {
	    if (written) {
		fflush(to);
		G_fseek(to, 0l, 0);
	    }
	    return (0);
	}
	if (!fwrite(buf, 1, size, to)) {
	    G_warning(_("Failed to copy file"));
	    return (-1);
	}
	written = 1;
    }
    /* NOTREACHED */
}
