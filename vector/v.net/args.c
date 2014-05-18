#include <stdlib.h>
#include <string.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "proto.h"

void define_options(struct opt *opt)
{
    char *desc;
    
    opt->input = G_define_standard_option(G_OPT_V_INPUT);
    opt->input->required = NO;
    opt->input->label = _("Name of input vector line map (arcs)");
    opt->input->description = _("Required for operation 'nodes', 'connect', "
				"'report' and 'nreport'");
    opt->input->guisection = _("Arcs");

    opt->points = G_define_standard_option(G_OPT_V_INPUT);
    opt->points->key = "points";
    opt->points->label = _("Name of input vector point map (nodes)");
    opt->points->description =
	_("Required for operation 'connect' and 'arcs'");
    opt->points->required = NO;
    opt->points->guisection = _("Nodes");

    opt->output = G_define_standard_option(G_OPT_V_OUTPUT);
    opt->output->required = NO;

    opt->action = G_define_option();
    opt->action->key = "operation";
    opt->action->type = TYPE_STRING;
    opt->action->required = YES;
    opt->action->multiple = NO;
    opt->action->options = "nodes,connect,arcs,report,nreport";
    opt->action->description = _("Operation to be performed");
    desc = NULL;
    G_asprintf(&desc,
	       "nodes;%s;connect;%s;arcs;%s;report;%s;nreport;%s",
	       _("new point is placed on each node (line end) "
		 "if doesn't exist"),
	       _("connect still unconnected points to vector network "
		 "by inserting new line(s)"),
	       _("new line is created from start point "
		 "to end point"),
	       _("print to standard output "
		 "{line_category start_point_category end_point_category}"),
	       _("print to standard output "
		 "{point_category line_category[,line_category...]}"));
    opt->action->descriptions = desc;

    opt->afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    opt->afield_opt->key = "alayer";
    opt->afield_opt->gisprompt = "new,layer,layer";
    opt->afield_opt->label = _("Arc layer");
    opt->afield_opt->guisection = _("Arcs");

    opt->nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    opt->nfield_opt->key = "nlayer";
    opt->nfield_opt->answer = "2";
    opt->nfield_opt->gisprompt = "new,layer,layer";
    opt->nfield_opt->label = _("Node layer");
    opt->nfield_opt->guisection = _("Nodes");

    opt->thresh_opt = G_define_option();
    opt->thresh_opt->key = "thresh";
    opt->thresh_opt->type = TYPE_DOUBLE;
    opt->thresh_opt->required = NO;
    opt->thresh_opt->multiple = NO;
    opt->thresh_opt->label = "Threshold";
    opt->thresh_opt->description =
	_("Required for operation 'connect'. Connect points in given threshold.");

    opt->file = G_define_standard_option(G_OPT_F_INPUT);
    opt->file->key = "file";
    opt->file->label = _("Name of input file");
    opt->file->description =
	_("Required for operation 'arcs'. '-' for standard input.");
    opt->file->required = NO;

    opt->cats_flag = G_define_flag();
    opt->cats_flag->key = 'c';
    opt->cats_flag->label = _("Assign unique categories to new points");
    opt->cats_flag->description = _("For operation 'nodes'");
    opt->cats_flag->guisection = _("Nodes");

    opt->snap_flag = G_define_flag();
    opt->snap_flag->key = 's';
    opt->snap_flag->label = _("Snap points to network");
    opt->snap_flag->description =
	_("For operation 'connect'. By default, a new line from the point to the network is created.");
    opt->snap_flag->guisection = _("Nodes");
}

void parse_arguments(const struct opt *opt,
		     int *afield, int *nfield, double *thresh, int *act)
{
    *afield = atoi(opt->afield_opt->answer);
    *nfield = atoi(opt->nfield_opt->answer);
    *thresh = 0.0;

    if (strcmp(opt->action->answer, "nodes") == 0)
	*act = TOOL_NODES;
    else if (strcmp(opt->action->answer, "connect") == 0)
	*act = TOOL_CONNECT;
    else if (strcmp(opt->action->answer, "report") == 0)
	*act = TOOL_REPORT;
    else if (strcmp(opt->action->answer, "nreport") == 0)
	*act = TOOL_NREPORT;
    else if (strcmp(opt->action->answer, "arcs") == 0)
	*act = TOOL_ARCS;
    else
	G_fatal_error(_("Unknown operation"));

    if (*act == TOOL_NODES || *act == TOOL_CONNECT ||
	*act == TOOL_REPORT || *act == TOOL_NREPORT) {
	if (opt->input->answer == NULL)
	    G_fatal_error(_("Required parameter <%s> not set"),
			  opt->input->key);
    }

    if (*act == TOOL_NODES || *act == TOOL_CONNECT) {
	if (opt->output->answer == NULL)
	    G_fatal_error(_("Required parameter <%s> not set"),
			  opt->output->key);
    }

    if (*act == TOOL_CONNECT) {
	if (opt->points->answer == NULL)
	    G_fatal_error(_("Required parameter <%s> not set"),
			  opt->points->key);

	if (opt->thresh_opt->answer == NULL)
	    G_fatal_error(_("Required parameter <%s> not set"),
			  opt->thresh_opt->key);

	*thresh = atof(opt->thresh_opt->answer);

	if (*thresh < 0.0)
	    G_fatal_error(_("Threshold value must be >= 0"));
    }

    if (*act == TOOL_ARCS && !opt->file->answer) {
	G_fatal_error(_("Required parameter <%s> not set"), opt->file->key);
    }
}
