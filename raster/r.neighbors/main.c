/****************************************************************************
 *
 * MODULE:       r.neighbors
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, Bob Covill <bcovill tekmap.ns.ca>,
 *               Brad Douglas <rez touchofmadness.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>, Jan-Oliver Wagner <jan intevation.de>,
 *               Radim Blazek <radim.blazek gmail.com>
 *
 * PURPOSE:      Makes each cell category value a function of the category values 
 *               assigned to the cells around it, and stores new cell values in an
 *               output raster map layer
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/stats.h>
#include "ncb.h"
#include "local_proto.h"

typedef int (*ifunc)(void);

struct menu
{
	stat_func *method;	/* routine to compute new value */
	stat_func_w *method_w;	/* routine to compute new value (weighted) */
	ifunc cat_names;	/* routine to make category names */
	int copycolr;		/* flag if color table can be copied */
	int half;		/* whether to add 0.5 to result */
	char *name;		/* method name */
	char *text;		/* menu display - full description */
};

#define NO_CATS 0

/* modify this table to add new methods */
static struct menu menu[] =
{
	{c_ave,    w_ave,    NO_CATS,	1, 1, "average", "average value"},
	{c_median, w_median, NO_CATS,	1, 0, "median","median value"},
	{c_mode,   w_mode,   NO_CATS,	1, 0, "mode",  "most frequently occuring value"},
	{c_min,    NULL,     NO_CATS,	1, 0, "minimum", "lowest value"},
	{c_max,    NULL,     NO_CATS,	1, 0, "maximum", "highest value"},
	{c_stddev, w_stddev, NO_CATS,	0, 1, "stddev", "standard deviation"},
	{c_sum,    w_sum,    NO_CATS,	1, 0, "sum", "sum of values"},
	{c_var,    w_var,    NO_CATS,	0, 1, "variance", "statistical variance"},
	{c_divr,   NULL,     divr_cats,	0, 0, "diversity", "number of different values"},
	{c_intr,   NULL,     intr_cats,	0, 0, "interspersion", "number of values different than center value"},
	{0,0,0,0,0,0,0}
};

struct ncb ncb;

int main (int argc, char *argv[])
{
	char *p;
	int method;
	int in_fd;
	int out_fd;
	DCELL *result;
	RASTER_MAP_TYPE map_type;
	int row, col;
	int readrow;
	int nrows, ncols;
	int n;
	int copycolr;
	int half;
	stat_func *newvalue;
	stat_func_w *newvalue_w;
	ifunc cat_names;
	struct Colors colr;
	struct Cell_head cellhd;
	struct Cell_head window;
	struct History history;
	struct GModule *module;
	struct
	{
		struct Option *input, *output;
		struct Option *method, *size;
		struct Option *title;
		struct Option *weight;
	} parm;
	struct
	{
                /* please, remove before GRASS 7 released */
		struct Flag *quiet, *align, *circle;
	} flag;

	DCELL *values;   /* list of neighborhood values */
	DCELL (*values_w)[2];   /* list of neighborhood values and weights */

	G_gisinit (argv[0]);

	module = G_define_module();
	module->keywords = _("raster");
	module->description =
		_("Makes each cell category value a "
		  "function of the category values assigned to the cells "
		  "around it, and stores new cell values in an output raster "
		  "map layer.");

	parm.input = G_define_standard_option(G_OPT_R_INPUT);

	parm.output = G_define_standard_option(G_OPT_R_OUTPUT) ;

	parm.method = G_define_option() ;
	parm.method->key        = "method" ;
	parm.method->type       = TYPE_STRING ;
	parm.method->required   = NO ;
	parm.method->answer     = "average";
	p = parm.method->options  = G_malloc(1024);
	for (n = 0; menu[n].name; n++)
	{
		if (n)
			strcat (p, ",");
		else
			*p = 0;
		strcat (p, menu[n].name);
	}
	parm.method->description= _("Neighborhood operation") ;

	parm.size = G_define_option() ;
	parm.size->key        = "size" ;
	parm.size->type       = TYPE_INTEGER ;
	parm.size->required   = NO ;
	parm.size->description= _("Neighborhood size") ;
	parm.size->answer     = "3";

	parm.title = G_define_option() ;
	parm.title->key        = "title" ;
	parm.title->key_desc   = "\"phrase\"" ;
	parm.title->type       = TYPE_STRING ;
	parm.title->required   = NO ;
	parm.title->description= _("Title of the output raster map") ;

	parm.weight = G_define_option() ;
	parm.weight->key        = "weight" ;
	parm.weight->type       = TYPE_STRING ;
	parm.weight->required   = NO ;
	parm.weight->gisprompt  = "old_file,file,input";
	parm.weight->description= _("File containing weights") ;

	flag.align = G_define_flag();
	flag.align->key = 'a';
	flag.align->description = _("Do not align output with the input");

        /* please, remove before GRASS 7 released */
	flag.quiet = G_define_flag();
	flag.quiet->key = 'q';
	flag.quiet->description = _("Run quietly");

	flag.circle = G_define_flag();
	flag.circle->key = 'c';
	flag.circle->description = _("Use circular neighborhood");

	if (G_parser(argc, argv))
		exit(EXIT_FAILURE);

	sscanf(parm.size->answer, "%d", &ncb.nsize);
	if (ncb.nsize <= 0)
		G_fatal_error(_("Neighborhood size must be positive"));
	if (ncb.nsize % 2 == 0)
		G_fatal_error(_("Neighborhood size must be odd"));
	ncb.dist = ncb.nsize/2;

	if (parm.weight->answer && flag.circle->answer)
		G_fatal_error(_("weight= and -c are mutually exclusive"));

	p = ncb.oldcell.name = parm.input->answer;
	if(NULL == (ncb.oldcell.mapset = G_find_cell2(p,"")))
	{
		G_fatal_error(_("Raster map <%s> not found"), p);
	}
	p = ncb.newcell.name = parm.output->answer;
	if (G_legal_filename(p) < 0)
	{
		G_fatal_error (_("<%s> is an illegal file name"), p);
	}
	ncb.newcell.mapset = G_mapset();

	if(!flag.align->answer)
	{
		if (G_get_cellhd (ncb.oldcell.name, ncb.oldcell.mapset, &cellhd) < 0)
			exit(EXIT_FAILURE);
		G_get_window (&window);
		G_align_window (&window, &cellhd);
		G_set_window (&window);
	}

	nrows = G_window_rows();
	ncols = G_window_cols();

	/* open raster maps */
	if ((in_fd = G_open_cell_old (ncb.oldcell.name, ncb.oldcell.mapset)) < 0)
		G_fatal_error (_("Unable to open raster map <%s> in mapset <%s>"),
			ncb.oldcell.name, ncb.oldcell.mapset);

	map_type = G_get_raster_map_type(in_fd);

	/* get the method */
	for (method = 0; (p = menu[method].name); method++)
		if ((strcmp(p, parm.method->answer) == 0))
			break;
	if (!p)
	{
		G_warning (_("<%s=%s> unknown %s"),
			 parm.method->key, parm.method->answer, parm.method->key);
		G_usage();
		exit(EXIT_FAILURE);
	}

	half = (map_type == CELL_TYPE) ? menu[method].half : 0;

	/* establish the newvalue routine */
	newvalue = menu[method].method;
	newvalue_w = menu[method].method_w;

	/* copy color table? */
	copycolr = menu[method].copycolr;
	if (copycolr)
	{
		G_suppress_warnings (1);
		copycolr = (G_read_colors (ncb.oldcell.name, ncb.oldcell.mapset, &colr) > 0);
		G_suppress_warnings (0);
	}

	/* read the weights */
	if (parm.weight->answer)
	{
		read_weights(parm.weight->answer);
		if (!newvalue_w)
			weights_mask();
	}
	else
		newvalue_w = NULL;

	/* allocate the cell buffers */
	allocate_bufs ();
	result = G_allocate_d_raster_buf();

	/* get title, initialize the category and stat info */
	if (parm.title->answer)
		strcpy (ncb.title, parm.title->answer);
	else
		sprintf (ncb.title,"%dx%d neighborhood: %s of %s",
			 ncb.nsize, ncb.nsize, menu[method].name, ncb.oldcell.name);


	/* initialize the cell bufs with 'dist' rows of the old cellfile */

	readrow = 0;
	for (row = 0; row < ncb.dist; row++)
		readcell (in_fd, readrow++, nrows, ncols);

	/* open raster map */
	in_fd = G_open_cell_old (ncb.oldcell.name, ncb.oldcell.mapset);
	if (in_fd < 0)
		exit(EXIT_FAILURE);

	/*open the new raster map */
	out_fd = G_open_raster_new (ncb.newcell.name, map_type);
	if (out_fd < 0)
		exit(EXIT_FAILURE);

        /* please, remove before GRASS 7 released */
        if(flag.quiet->answer) {
            putenv("GRASS_VERBOSE=0");
            G_warning(_("The '-q' flag is superseded and will be removed "
                "in future. Please use '--quiet' instead."));
        }

	if (flag.circle->answer)
	    circle_mask();

	if (newvalue_w)
		values_w = (DCELL (*)[2]) G_malloc(ncb.nsize * ncb.nsize * 2 * sizeof (DCELL));
	else
		values = (DCELL *) G_malloc (ncb.nsize * ncb.nsize * sizeof (DCELL));

	for (row = 0; row < nrows; row++)
	{
                G_percent (row, nrows, 2);
		readcell (in_fd, readrow++, nrows, ncols);
		for (col = 0; col < ncols; col++)
		{
			DCELL *rp = &result[col];

			if (newvalue_w)
				n = gather_w(values_w, col);
			else
				n = gather(values, col);

			if (n < 0)
				G_set_d_null_value (rp, 1);
			else
			{
				if (newvalue_w)
					newvalue_w(rp, values_w, n);
				else
					newvalue(rp, values, n);

				if (half && !G_is_d_null_value(rp))
					*rp += 0.5;
			}
		}
		G_put_d_raster_row(out_fd, result);
	}
        G_percent (row, nrows, 2);

	G_close_cell (out_fd);
	G_close_cell (in_fd);

	/* put out category info */
	null_cats () ;
	if ((cat_names = menu[method].cat_names))
		cat_names();

	G_write_cats (ncb.newcell.name, &ncb.cats);

	if(copycolr)
		G_write_colors (ncb.newcell.name, ncb.newcell.mapset, &colr);

	G_short_history(ncb.newcell.name, "raster", &history);
	G_command_history(&history);
	G_write_history(ncb.newcell.name, &history);


	exit(EXIT_SUCCESS);
}
