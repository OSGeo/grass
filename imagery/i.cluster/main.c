
/****************************************************************************
 *
 * MODULE:       i.cluster
 * AUTHOR(S):    Michael Shapiro (USACERL) and Tao Wen (UIUC)
 *                    (original contributors)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      builds pixel clusters based on multi-image pixel values
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#define GLOBAL
#include "global.h"
#include "local_proto.h"


struct Cluster C;
struct Signature in_sig;

int maxclass;
double conv;
double sep;
int iters;
int mcs;
char *group;
char *subgroup;
struct Ref ref;
char *outsigfile;
char *insigfile;
char *reportfile;
DCELL **cell;
int *cellfd;
FILE *report;
int sample_rows, sample_cols;
time_t start_time;

static int interrupted = 0;


int main(int argc, char *argv[])
{
    int count;
    int n;
    int row, nrows;
    int col, ncols;
    DCELL *x;
    struct Cell_head window;
    FILE *fd;

    struct GModule *module;
    struct
    {
	struct Option *group_name, *subgroup_name, *out_sig, *seed_sig,
	    *class, *sample_interval, *iterations, *separation,
	    *convergence, *min_size, *report_file;
    } parm;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("signatures"));
    module->label =
	_("Generates spectral signatures for land cover "
	  "types in an image using a clustering algorithm.");
    module->description =
	_("The resulting signature file is used as input for i.maxlik, "
	  "to generate an unsupervised image classification.");

    parm.group_name = G_define_standard_option(G_OPT_I_GROUP);

    parm.subgroup_name = G_define_standard_option(G_OPT_I_SUBGROUP);
    
    parm.out_sig = G_define_option();
    parm.out_sig->key = "signaturefile";
    parm.out_sig->type = TYPE_STRING;
    parm.out_sig->key_desc = "name";
    parm.out_sig->required = YES;
    parm.out_sig->gisprompt = "new,sig,sigfile";
    parm.out_sig->description = _("Name for output file containing result signatures");

    parm.class = G_define_option();
    parm.class->key = "classes";
    parm.class->type = TYPE_INTEGER;
    parm.class->options = "1-255";
    parm.class->required = YES;
    parm.class->description = _("Initial number of classes");
    parm.class->guisection = _("Settings");

    parm.seed_sig = G_define_option();
    parm.seed_sig->key = "seed";
    parm.seed_sig->required = NO;
    parm.seed_sig->type = TYPE_STRING;
    parm.seed_sig->key_desc = "name";
    parm.seed_sig->gisprompt = "old,sig,sigfile";
    parm.seed_sig->description = _("Name of file containing initial signatures");

    parm.sample_interval = G_define_option();
    parm.sample_interval->key = "sample";
    parm.sample_interval->key_desc = "rows,cols";
    parm.sample_interval->type = TYPE_INTEGER;
    parm.sample_interval->required = NO;
    parm.sample_interval->description =
	_("Number of rows and columns over which a sample pixel is taken");
    parm.sample_interval->guisection = _("Settings");

    parm.iterations = G_define_option();
    parm.iterations->key = "iterations";
    parm.iterations->type = TYPE_INTEGER;
    parm.iterations->required = NO;
    parm.iterations->description = _("Maximum number of iterations");
    parm.iterations->answer = "30";
    parm.iterations->guisection = _("Settings");

    parm.convergence = G_define_option();
    parm.convergence->key = "convergence";
    parm.convergence->type = TYPE_DOUBLE;
    parm.convergence->required = NO;
    parm.convergence->options = "0-100";
    parm.convergence->description = _("Percent convergence");
    parm.convergence->answer = "98.0";
    parm.convergence->guisection = _("Settings");

    parm.separation = G_define_option();
    parm.separation->key = "separation";
    parm.separation->type = TYPE_DOUBLE;
    parm.separation->required = NO;
    parm.separation->description = _("Cluster separation");
    parm.separation->answer = "0.0";
    parm.separation->guisection = _("Settings");

    parm.min_size = G_define_option();
    parm.min_size->key = "min_size";
    parm.min_size->type = TYPE_INTEGER;
    parm.min_size->required = NO;
    parm.min_size->description = _("Minimum number of pixels in a class");
    parm.min_size->answer = "17";
    parm.min_size->guisection = _("Settings");

    parm.report_file = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.report_file->key = "reportfile";
    parm.report_file->required = NO;
    parm.report_file->description = _("Name for output file containing final report");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    group = parm.group_name->answer;	/* a required parameter */
    subgroup = parm.subgroup_name->answer;	/* required */
    outsigfile = parm.out_sig->answer;
    
    /* check all the inputs */
    if (!I_find_group(group)) {
        G_fatal_error(_("Group <%s> not found in current mapset"), group);
    }
    if (!I_find_subgroup(group, subgroup)) {
        G_fatal_error(_("Subgroup <%s> in group <%s> not found"), subgroup, group);
    }
    
    /* GRASS parser fails to detect existing signature files as
     * detection needs answers from other parameters as group and subgroup.
     * Thus check is performed only now. */
    if (!G_get_overwrite() && I_find_signature_file(group, subgroup, "sig", outsigfile)) {
        G_fatal_error(_("option <%s>: <%s> exists. To overwrite, use the --overwrite flag"),
                        parm.out_sig->key, parm.out_sig->answer);
    }

    G_get_window(&window);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    I_cluster_clear(&C);


    if (sscanf(parm.class->answer, "%d", &maxclass) != 1 || maxclass < 1
	|| maxclass > 255) {
	G_fatal_error(_("Illegal number of initial classes (%s)"),
		      parm.class->answer);
    }

    insigfile = parm.seed_sig->answer;

    if (parm.sample_interval->answer) {
	if (sscanf
	    (parm.sample_interval->answer, "%d,%d", &sample_rows,
	     &sample_cols) != 2 || sample_rows < 1 || sample_cols < 1 ||
	    sample_rows > nrows || sample_cols > ncols) {
	    G_fatal_error(_("Illegal value(s) of sample intervals (%s)"),
			  parm.sample_interval->answer);
	}
    }
    else {
	sample_rows = nrows / 100;
	if (sample_rows < 1)
	    sample_rows = 1;
	sample_cols = ncols / 100;
	if (sample_cols < 1)
	    sample_cols = 1;
    }

    if (sscanf(parm.iterations->answer, "%d", &iters) != 1 || iters < 1) {
	G_fatal_error(_("Illegal value of iterations (%s)"),
		      parm.iterations->answer);
    }

    if (sscanf(parm.convergence->answer, "%lf", &conv) != 1 || conv < 0.0 ||
	conv > 100.0) {
	G_fatal_error(_("Illegal value of convergence (%s)"),
		      parm.convergence->answer);
    }

    if (sscanf(parm.separation->answer, "%lf", &sep) != 1 || sep < 0.0) {
	G_fatal_error(_("Illegal value of separation (%s)"),
		      parm.separation->answer);
    }

    if (sscanf(parm.min_size->answer, "%d", &mcs) != 1 || mcs < 2) {
	G_fatal_error(_("Illegal value of min_size (%s)"),
		      parm.min_size->answer);
    }
    
    if ((reportfile = parm.report_file->answer) == NULL)
	report = fopen(G_DEV_NULL, "w");
    else
	report = fopen(reportfile, "w");
    if (report == NULL) {
	G_fatal_error(_("Unable to create report file <%s>"),
		      reportfile);
    }

    open_files();


    fprintf(report,
	    _("#################### CLUSTER (%s) ####################%s%s"),
	    G_date(), HOST_NEWLINE, HOST_NEWLINE);
    fprintf(report, _("Location: %s%s"), G_location(), HOST_NEWLINE);
    fprintf(report, _("Mapset:   %s%s"), G_mapset(), HOST_NEWLINE);
    fprintf(report, _("Group:    %s%s"), group, HOST_NEWLINE);
    fprintf(report, _("Subgroup: %s%s"), subgroup, HOST_NEWLINE);
    for (n = 0; n < ref.nfiles; n++) {
	fprintf(report, _(" %s%s"),
		G_fully_qualified_name(ref.file[n].name, ref.file[n].mapset), HOST_NEWLINE);
    }
    fprintf(report, _("Result signature file: %s%s"), outsigfile, HOST_NEWLINE);
    fprintf(report, "%s", HOST_NEWLINE);
    fprintf(report, _("Region%s"), HOST_NEWLINE);
    fprintf(report, _("  North: %12.2f  East: %12.2f%s"), window.north,
	    window.east, HOST_NEWLINE);
    fprintf(report, _("  South: %12.2f  West: %12.2f%s"), window.south,
	    window.west, HOST_NEWLINE);
    fprintf(report, _("  Res:   %12.2f  Res:  %12.2f%s"), window.ns_res,
	    window.ew_res, HOST_NEWLINE);
    fprintf(report, _("  Rows:  %12d  Cols: %12d  Cells: %d%s"), nrows, ncols,
	    nrows * ncols, HOST_NEWLINE);
    fprintf(report, _("Mask: %s%s"), Rast_mask_info(), HOST_NEWLINE);
    fprintf(report, "%s", HOST_NEWLINE);
    fprintf(report, _("Cluster parameters%s"), HOST_NEWLINE);
    fprintf(report, _(" Number of initial classes:    %d"), maxclass);
    if (insigfile)
	fprintf(report, _(" [from signature file %s]"), insigfile);
    fprintf(report, "%s", HOST_NEWLINE);
    fprintf(report, _(" Minimum class size:           %d%s"), mcs, HOST_NEWLINE);
    fprintf(report, _(" Minimum class separation:     %f%s"), sep, HOST_NEWLINE);
    fprintf(report, _(" Percent convergence:          %f%s"), conv, HOST_NEWLINE);
    fprintf(report, _(" Maximum number of iterations: %d%s"), iters, HOST_NEWLINE);
    fprintf(report, "%s", HOST_NEWLINE);
    fprintf(report, _(" Row sampling interval:        %d%s"), sample_rows, HOST_NEWLINE);
    fprintf(report, _(" Col sampling interval:        %d%s"), sample_cols, HOST_NEWLINE);
    fprintf(report, "%s", HOST_NEWLINE);
    fflush(report);

    x = (DCELL *) G_malloc(ref.nfiles * sizeof(DCELL));

    I_cluster_begin(&C, ref.nfiles);

    count = 0;
    G_message(_("Reading raster maps..."));
    for (row = sample_rows - 1; row < nrows; row += sample_rows) {
	G_percent(row, nrows, 2);
	for (n = 0; n < ref.nfiles; n++)
	    Rast_get_d_row(cellfd[n], cell[n], row);
	for (col = sample_cols - 1; col < ncols; col += sample_cols) {
	    count++;
	    for (n = 0; n < ref.nfiles; n++)
		x[n] = cell[n][col];
	    if (I_cluster_point(&C, x) < 0)
		G_fatal_error(_("Out of Memory. Please run again and choose a "
				"smaller sample size."));
	}
    }
    G_percent(nrows, nrows, 2);

    fprintf(report, _("Sample size: %d points%s"), C.npoints, HOST_NEWLINE);
    fprintf(report, "%s", HOST_NEWLINE);
    if (count < 2)
	G_fatal_error(_("Not enough sample points. Please run again and "
			"choose a larger sample size."));

    if (C.npoints < 2)
	G_fatal_error(_("Not enough non-zero sample data points. Check "
			"your current region (and mask)."));

    for (n = 0; n < ref.nfiles; n++) {
	G_free(cell[n]);
	Rast_close(cellfd[n]);
    }
    G_free(x);

    start_time = time(NULL);
    I_cluster_exec(&C, maxclass, iters, conv, sep, mcs, checkpoint,
		   &interrupted);

    fprintf(report, _("%s########## final results #############%s"), HOST_NEWLINE, HOST_NEWLINE);
    fprintf(report, _("%d classes (convergence=%.1f%%)%s"),
	    I_cluster_nclasses(&C, mcs), (double)C.percent_stable, HOST_NEWLINE);
    print_separability(report, &C);
    print_class_means(report, &C);

    if ((fd =
	 I_fopen_signature_file_new(group, subgroup, outsigfile)) != NULL) {
	I_write_signatures(fd, &C.S);
	fclose(fd);
    }
    else {
	G_fatal_error(_("Unable to create signature file <%s> for group "
			"<%s>, subsgroup <%s>"), outsigfile, group, subgroup);
    }

    fprintf(report,
	    _("%s%s#################### CLASSES ####################%s"),
            HOST_NEWLINE, HOST_NEWLINE, HOST_NEWLINE);
    fprintf(report, _("%s%d classes, %.2f%% points stable%s"),
	    HOST_NEWLINE, I_cluster_nclasses(&C, 1), (double)C.percent_stable, HOST_NEWLINE);
    fprintf(report, _("%s######## CLUSTER END (%s) ########%s"),
	    HOST_NEWLINE, G_date(), HOST_NEWLINE);
    fclose(report);

    G_done_msg(_("File <%s> created."),
	       outsigfile);
    
    exit(EXIT_SUCCESS);

}
