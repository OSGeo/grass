
/****************************************************************************
 *
 * MODULE:       front end
 * AUTHOR(S):    Charles Ehlschlaeger, CERL (original contributor)
 *               Brad Douglas <rez touchofmadness.com>,
 *               Hamish Bowman <hamish_b yahoo.com>
 *               Markus Metz <markus.metz.giswork gmail.com>
 * PURPOSE:      Hydrological analysis
 * COPYRIGHT:    (C) 1999-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

static void write_hist(char *, char *, char *, int, int);

static const char *new_argv[22];
static int new_argc;

static void do_opt(const struct Option *opt)
{
    char *buf;
    if (!opt->answer)
	return;
    buf = G_malloc(strlen(opt->key) + 1 + strlen(opt->answer) + 1);
    sprintf(buf, "%s=%s", opt->key, opt->answer);
    new_argv[new_argc++] = buf;
}

int main(int argc, char *argv[])
{
    char command[GPATH_MAX];
    int err, ret;
    struct Option *opt1;
    struct Option *opt2;
    struct Option *opt3;
    struct Option *opt4;
    struct Option *opt5;
    struct Option *opt6;
    struct Option *opt7;
    struct Option *opt8;
    struct Option *opt9;
    struct Option *opt10;
    struct Option *opt11;
    struct Option *opt12;
    struct Option *opt13;
    struct Option *opt14;
    struct Option *opt15;
    struct Option *opt16;
    struct Option *opt17;
    struct Flag *flag_sfd;
    struct Flag *flag_flow;
    struct Flag *flag_seg;
    struct Flag *flag_abs;
    struct Flag *flag_flat;
    struct GModule *module;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    module->description = _("Calculates hydrological parameters and RUSLE factors.");

    opt1 = G_define_standard_option(G_OPT_R_ELEV);
    opt1->guisection = _("Inputs");

    opt2 = G_define_standard_option(G_OPT_R_INPUT);
    opt2->key = "depression";
    opt2->label = _("Name of input depressions raster map");
    opt2->description = _("All non-NULL and non-zero cells are considered as real depressions");
    opt2->required = NO;
    opt2->guisection = _("Inputs");

    opt3 = G_define_standard_option(G_OPT_R_INPUT);
    opt3->key = "flow";
    opt3->description = _("Name of input raster representing amount of overland flow per cell");
    opt3->required = NO;
    opt3->guisection = _("Inputs");

    opt4 = G_define_standard_option(G_OPT_R_INPUT);
    opt4->key = "disturbed_land";
    opt4->label = _("Name of input raster map percent of disturbed land");
    opt4->description = _("For USLE");
    opt4->required = NO;
    opt4->guisection = _("Inputs");

    opt5 = G_define_standard_option(G_OPT_R_INPUT);
    opt5->key = "blocking";
    opt5->label =
	_("IName of input raster map blocking overland surface flow");
    opt5->description =
	_("For USLE. All non-NULL and non-zero cells are considered as blocking terrain.");
    opt5->required = NO;
    opt5->guisection = _("Inputs");

    opt6 = G_define_option();
    opt6->key = "threshold";
    opt6->description =
	_("Minimum size of exterior watershed basin");
    opt6->required = NO;
    opt6->type = TYPE_INTEGER;
    opt6->guisection = _("Inputs");

    opt7 = G_define_option();
    opt7->key = "max_slope_length";
    opt7->label =
	_("Maximum length of surface flow in map units");
    opt7->description = _("For USLE");
    opt7->required = NO;
    opt7->type = TYPE_DOUBLE;
    opt7->guisection = _("Inputs");

    opt8 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt8->key = "accumulation";
    opt8->label =
	_("Name for output accumulation raster map");
    opt8->description =
    _("Number of cells that drain through each cell");
    opt8->required = NO;
    opt8->guisection = _("Outputs");

    opt17 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt17->key = "tci";
    opt17->label =
	_("Topographic index ln(a / tan(b))");
    opt17->required = NO;
    opt17->guisection = _("Outputs");

    opt9 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt9->key = "drainage";
    opt9->description = _("Name for output drainage direction raster map");
    opt9->required = NO;
    opt9->guisection = _("Outputs");

    opt10 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt10->key = "basin";
    opt10->description =
	_("Name for basins raster map");
    opt10->description = _("Unique label for each watershed basin");
    opt10->required = NO;
    opt10->guisection = _("Outputs");

    opt11 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt11->key = "stream";
    opt11->description = _("Name for output stream segments raster map");
    opt11->required = NO;
    opt11->guisection = _("Outputs");

    opt12 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt12->key = "half_basin";
    opt12->label = _("Name for output half basins raster map");
    opt12->description =
	_("Each half-basin is given a unique value");
    opt12->required = NO;
    opt12->guisection = _("Outputs");

    opt13 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt13->key = "length_slope";
    opt13->label = _("Name for output slope length raster map");
    opt13->description =
	_("Slope length and steepness (LS) factor for USLE");
    opt13->required = NO;
    opt13->guisection = _("Outputs");

    opt14 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt14->key = "slope_steepness";
    opt14->label = _("Name for output slope steepness raster map");
    opt14->description = _("Slope steepness (S) factor for USLE");
    opt14->required = NO;
    opt14->guisection = _("Outputs");

    opt15 = G_define_option();
    opt15->key = "convergence";
    opt15->type = TYPE_INTEGER;
    opt15->required = NO;
    opt15->answer = "5";
    opt15->label = _("Convergence factor for MFD (1-10)");
    opt15->description =
	_("1 = most diverging flow, 10 = most converging flow. Recommended: 5");

    opt16 = G_define_option();
    opt16->key = "memory";
    opt16->type = TYPE_INTEGER;
    opt16->required = NO;
    opt16->answer = "300";	/* 300MB default value, please keep r.terraflow in sync */
    opt16->description = _("Maximum memory to be used with -m flag (in MB)");

    flag_sfd = G_define_flag();
    flag_sfd->key = 's';
    flag_sfd->label = _("SFD (D8) flow (default is MFD)");
    flag_sfd->description =
	_("SFD: single flow direction, MFD: multiple flow direction");

    flag_flow = G_define_flag();
    flag_flow->key = '4';
    flag_flow->description =
	_("Allow only horizontal and vertical flow of water");

    flag_seg = G_define_flag();
    flag_seg->key = 'm';
    flag_seg->label =
	_("Enable disk swap memory option: Operation is slow");
    flag_seg->description =
	_("Only needed if memory requirements exceed available RAM; see manual on how to calculate memory requirements");

    flag_abs = G_define_flag();
    flag_abs->key = 'a';
    flag_abs->label =
	_("Use positive flow accumulation even for likely underestimates");
    flag_abs->description =
	_("See manual for a detailed description of flow accumulation output");

    flag_flat = G_define_flag();
    flag_flat->key = 'b';
    flag_flat->label =
	_("Beautify flat areas");
    flag_flat->description =
	_("Flow direction in flat areas is modified to look prettier");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* Check option combinations */

    /* Check for some output map */
    if ((opt8->answer == NULL)
	&& (opt9->answer == NULL)
	&& (opt10->answer == NULL)
	&& (opt11->answer == NULL)
	&& (opt12->answer == NULL)
	&& (opt14->answer == NULL)
	&& (opt15->answer == NULL)) {
	G_fatal_error(_("Sorry, you must choose an output map."));
    }

    /* basin threshold */
    if (opt6->answer) {
	if (atoi(opt6->answer) <= 0)
	    G_fatal_error(_("The basin threshold must be a positive number."));
    }

    err = 0;
    /* basin and basin threshold */
    err += (opt10->answer != NULL && opt6->answer == NULL);
    /* stream and basin threshold */
    err += (opt11->answer != NULL && opt6->answer == NULL);
    /* half_basin and basin threshold */
    err += (opt12->answer != NULL && opt6->answer == NULL);
    /* LS factor and basin threshold */
    err += (opt13->answer != NULL && opt6->answer == NULL);
    /* S factor and basin threshold */
    err += (opt14->answer != NULL && opt6->answer == NULL);

    if (err) {
	G_message(_("Sorry, if any of the following options are set:\n"
		    "    basin, stream, half_basin, length_slope, or slope_steepness\n"
		    "    you MUST provide a value for the basin "
		    "threshold parameter."));
	G_usage();
	exit(EXIT_FAILURE);
    }

    /* Build command line */
    sprintf(command, "%s/etc/r.watershed/%s",
	    G_gisbase(),
	    flag_seg->answer ? "seg" : "ram");
    new_argv[new_argc++] = command;

    if (flag_sfd->answer)
	new_argv[new_argc++] = "-s";

    if (flag_flow->answer)
	new_argv[new_argc++] = "-4";

    if (flag_abs->answer)
	new_argv[new_argc++] = "-a";

    if (flag_flat->answer && !flag_seg->answer)
	new_argv[new_argc++] = "-b";

    if (flag_flat->answer && flag_seg->answer)
	G_message(_("Beautify flat areas is not yet supported for disk swap mode"));

    do_opt(opt1);
    do_opt(opt2);
    do_opt(opt3);
    do_opt(opt4);
    do_opt(opt5);
    do_opt(opt6);
    do_opt(opt7);
    do_opt(opt8);
    do_opt(opt17);
    do_opt(opt9);
    do_opt(opt10);
    do_opt(opt11);
    do_opt(opt12);
    do_opt(opt13);
    do_opt(opt14);
    do_opt(opt15);
    if (flag_seg->answer)
	do_opt(opt16);
    new_argv[new_argc++] = NULL;

    G_debug(1, "Mode: %s", flag_seg->answer ? "Segmented" : "All in RAM");

    ret = G_vspawn_ex(new_argv[0], new_argv);

    if (ret != EXIT_SUCCESS)
	G_warning(_("Subprocess failed with exit code %d"), ret);

    /* record map metadata/history info */
    if (opt8->answer)
	write_hist(opt8->answer,
		   "Watershed accumulation: overland flow that traverses each cell",
		   opt1->answer, flag_seg->answer, flag_sfd->answer);
    if (opt17->answer)
	write_hist(opt17->answer,
		   "Watershed accumulation: topographic index ln(a / tan b)",
		   opt1->answer, flag_seg->answer, flag_sfd->answer);
    if (opt9->answer)
	write_hist(opt9->answer,
		   "Watershed drainage direction (CCW from East divided by 45deg)",
		   opt1->answer, flag_seg->answer, flag_sfd->answer);
    if (opt10->answer)
	write_hist(opt10->answer,
		   "Watershed basins", opt1->answer, flag_seg->answer, 
		   flag_sfd->answer);
    if (opt11->answer)
	write_hist(opt11->answer,
		   "Watershed stream segments", opt1->answer,
		   flag_seg->answer, flag_sfd->answer);
    if (opt12->answer)
	write_hist(opt12->answer,
		   "Watershed half-basins", opt1->answer, flag_seg->answer, 
		   flag_sfd->answer);
    if (opt13->answer)
	write_hist(opt13->answer,
		   "Watershed slope length and steepness (LS) factor",
		   opt1->answer, flag_seg->answer, flag_sfd->answer);
    if (opt14->answer)
	write_hist(opt14->answer,
		   "Watershed slope steepness (S) factor",
		   opt1->answer, flag_seg->answer, flag_sfd->answer);

    exit(ret);
}

/* record map history info */
static void write_hist(char *map_name, char *title, char *source_name, int mode, int sfd)
{
    struct History history;

    Rast_put_cell_title(map_name, title);

    Rast_short_history(map_name, "raster", &history);
    Rast_set_history(&history, HIST_DATSRC_1, source_name);
    Rast_append_format_history(
	&history, "Processing mode: %s", sfd ? "SFD (D8)" : "MFD");
    Rast_append_format_history(
	&history, "Memory mode: %s", mode ? "Segmented" : "All in RAM");
    Rast_command_history(&history);

    Rast_write_history(map_name, &history);
}
