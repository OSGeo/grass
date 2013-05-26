
/****************************************************************************
 *
 * MODULE:       r.in.ascii
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, Hamish Bowman <hamish_b yahoo.com>,
 *               Roberto Flor <flor itc.it>, Roger Miller <rgrmill rt66 com>,
 *               Brad Douglas <rez touchofmadness.com>, Huidae Cho <grass4u gmail.com>,
 *               Glynn Clements <glynn gclements.plus.com>
 *               Jachym Cepicky <jachym les-ejk.cz>, Jan-Oliver Wagner <jan intevation.de>,
 *               Justin Hickey <jhickey hpcc.nectec.or.th>
 * PURPOSE:      Import ASCII or SURFER files
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"


FILE *Tmp_fd = NULL;
char *Tmp_file = NULL;

const float GS_BLANK = 1.70141E+038;

static int file_cpy(FILE *, FILE *);


int main(int argc, char *argv[])
{
    char *input;
    char *output;
    char *title;
    char *temp;
    FILE *fd, *ft;
    int cf, direction, sz;
    struct Cell_head cellhd;
    struct History history;
    void *rast, *rast_ptr;
    int row, col;
    int nrows, ncols;
    double x;
    char y[128];
    struct GModule *module;
    struct
    {
	struct Option *input, *output, *title, *mult, *nv, *type;
    } parm;
    struct
    {
	struct Flag *s;
    } flag;
    char *null_val_str;
    DCELL mult;
    RASTER_MAP_TYPE data_type;
    double atof();

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    G_add_keyword(_("conversion"));
    G_add_keyword("ASCII");
    module->description =
	_("Converts a GRASS ASCII raster file to binary raster map.");

    parm.input = G_define_standard_option(G_OPT_F_INPUT);
    parm.input->label =
	_("Name of input file to be imported");
    parm.input->description = _("-' for standard input");

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.type = G_define_option();
    parm.type->key = "type";
    parm.type->type = TYPE_STRING;
    parm.type->required = NO;
    parm.type->options = "CELL,FCELL,DCELL";
    parm.type->label = _("Storage type for resultant raster map");
    parm.type->description = _("Default: CELL for integer values, DCELL for floating-point values");
    
    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title for resultant raster map");

    parm.mult = G_define_option();
    parm.mult->key = "mult";
    parm.mult->type = TYPE_DOUBLE;
    parm.mult->description = _("Default: read from header");
    parm.mult->required = NO;
    parm.mult->label = _("Multiplier for ASCII data");

    parm.nv = G_define_option();
    parm.nv->key = "nv";
    parm.nv->type = TYPE_STRING;
    parm.nv->required = NO;
    parm.nv->multiple = NO;
    parm.nv->description = _("Default: read from header");
    parm.nv->label = _("String representing NULL value data cell");
    parm.nv->guisection = _("NULL data");
    
    flag.s = G_define_flag();
    flag.s->key = 's';
    flag.s->description =
	_("SURFER (Golden Software) ASCII file will be imported");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    input = parm.input->answer;
    output = parm.output->answer;

    temp = G_tempfile();
    ft = fopen(temp, "w+");
    if (ft == NULL)
	G_fatal_error(_("Unable to open temporary file <%s>"), temp);

    if ((title = parm.title->answer))
	G_strip(title);
    
    if (!parm.mult->answer)
	Rast_set_d_null_value(&mult, 1);
    else if ((sscanf(parm.mult->answer, "%lf", &mult)) != 1)
	G_fatal_error(_("Wrong entry for multiplier: %s"), parm.mult->answer);
    
    null_val_str = parm.nv->answer;

    data_type = -1;
    if (parm.type->answer) {
	switch(parm.type->answer[0]) {
	case 'C':
	    data_type = CELL_TYPE;
	    break;
	case 'F':
	    data_type = FCELL_TYPE;
	    break;
	case 'D':
	    data_type = DCELL_TYPE;
	    break;
	}
    }
    
    if (strcmp(input, "-") == 0) {
	Tmp_file = G_tempfile();
	if (NULL == (Tmp_fd = fopen(Tmp_file, "w+")))
	    G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file);
	unlink(Tmp_file);
	if (0 > file_cpy(stdin, Tmp_fd))
	    G_fatal_error(_("Unable to read input from stdin"));
	fd = Tmp_fd;
    }
    else
	fd = fopen(input, "r");

    if (fd == NULL) {
	G_fatal_error(_("Unable to read input from <%s>"), input);
    }

    direction = 1;
    sz = 0;
    if (flag.s->answer) {
	sz = getgrdhead(fd, &cellhd);
	/* for Surfer files, the data type is always FCELL_TYPE,
	   the multiplier and the null_val_str are never used */
	data_type = FCELL_TYPE;
	mult = 1.;
	null_val_str = "";
	/* rows in surfer files are ordered from bottom to top,
	   opposite of normal GRASS ordering */
	direction = -1;
    }
    else
	sz = gethead(fd, &cellhd, &data_type, &mult, &null_val_str);

    if (!sz)
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


    rast_ptr = Rast_allocate_buf(data_type);
    rast = rast_ptr;
    cf = Rast_open_new(output, data_type);
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	for (col = 0; col < ncols; col++) {
	    if (fscanf(fd, "%s", y) != 1) {
		Rast_unopen(cf);
		G_fatal_error(_("Data conversion failed at row %d, col %d"),
			      row + 1, col + 1);
	    }
	    if (strcmp(y, null_val_str)) {
		x = atof(y);
		if ((float)x == GS_BLANK) {
		    Rast_set_null_value(rast_ptr, 1, data_type);
		}
		else {
		    Rast_set_d_value(rast_ptr,
					 (DCELL) (x * mult), data_type);
		}
	    }
	    else {
		Rast_set_null_value(rast_ptr, 1, data_type);
	    }
	    rast_ptr = G_incr_void_ptr(rast_ptr, Rast_cell_size(data_type));
	}
	fwrite(rast, Rast_cell_size(data_type), ncols, ft);
	rast_ptr = rast;
    }
    G_percent(nrows, nrows, 2);
    G_debug(1, "Creating support files for %s", output);

    sz = 0;
    if (direction < 0) {
	sz = -ncols * Rast_cell_size(data_type);
	G_fseek(ft, sz, SEEK_END);
	sz *= 2;
    }
    else {
	G_fseek(ft, 0L, SEEK_SET);
    }

    for (row = 0; row < nrows; row += 1) {
	fread(rast, Rast_cell_size(data_type), ncols, ft);
	Rast_put_row(cf, rast, data_type);
	G_fseek(ft, sz, SEEK_CUR);
    }
    fclose(ft);
    unlink(temp);

    Rast_close(cf);

    if (title)
	Rast_put_cell_title(output, title);

    Rast_short_history(output, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(output, &history);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}


static int file_cpy(FILE * from, FILE * to)
{
    char buf[BUFSIZ];
    size_t size;
    int written = 0;

    while (1) {
	size = fread(buf, 1, BUFSIZ, from);
	if (!size) {
	    if (written) {
		fflush(to);
		G_fseek(to, 0L, SEEK_SET);
	    }
	    return (0);
	}
	if (!fwrite(buf, 1, size, to)) {
	    G_warning(_("Unable to write to file"));
	    return (-1);
	}
	written = 1;
    }

    /* NOTREACHED */
    return -1;
}
