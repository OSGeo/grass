#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char **argv,
		struct _options *options, struct _flags* flags)
{
    options->dsn = G_define_option();
    options->dsn->key = "dsn";
    options->dsn->type = TYPE_STRING;
    options->dsn->gisprompt = "old_file,file,dsn";
    options->dsn->label = _("OGR data source name");
    options->dsn->description = _("Examples:\n"
			     "\t\tESRI Shapefile: directory containing shapefiles\n"
			     "\t\tMapInfo File: directory containing mapinfo files");
    options->dsn->guisection = _("Data source");

    options->layer = G_define_option();
    options->layer->key = "layer";
    options->layer->type = TYPE_STRING;
    options->layer->required = NO;
    options->layer->multiple = NO;
    options->layer->label = _("OGR layer name");
    options->layer->description = _("Examples:\n"
				   "\t\tESRI Shapefile: shapefile name\n"
				   "\t\tMapInfo File: mapinfo file name");
    options->layer->guisection = _("Data source");
    
    options->output = G_define_standard_option(G_OPT_V_OUTPUT);
    options->output->required = NO;
    options->output->description = _("Name for GRASS vector map");

    flags->format = G_define_flag();
    flags->format->key = 'f';
    flags->format->description = _("List supported OGR formats and exit");
    flags->format->guisection = _("Print");

    flags->layer = G_define_flag();
    flags->layer->key = 'l';
    flags->layer->description = _("List available OGR layers in dsn and exit");
    flags->layer->guisection = _("Print");

    flags->topo = G_define_flag();
    flags->topo->key = 'b';
    flags->topo->description = _("Do not build topology");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
}

void get_args(const struct _options *options, const struct _flags *flags,
	      char **dsn, char **layer, char **output,
	      int *list_format, int *list_layers, int *topo)
{
    *dsn    = options->dsn->answer;
    *layer  = options->layer->answer;
    *output = options->output->answer;
    *list_format = flags->format->answer ? 1 : 0;
    *list_layers = flags->layer->answer ? 1 : 0;
    *topo        = flags->topo-> answer ? 1 : 0;
}
