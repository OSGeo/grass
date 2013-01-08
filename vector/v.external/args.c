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
    options->dsn->label = _("Name of input OGR or PostGIS data source");
    options->dsn->description = _("Examples:\n"
				  "\t\tESRI Shapefile: directory containing a shapefile\n"
				  "\t\tMapInfo File: directory containing a mapinfo file\n"
				  "\t\tPostGIS database: connection string, eg. 'PG:dbname=db user=grass'");
    options->dsn->required = YES;

    options->layer = G_define_option();
    options->layer->key = "layer";
    options->layer->type = TYPE_STRING;
    options->layer->required = NO;
    options->layer->multiple = NO;
    options->layer->label = _("Name of OGR layer or PostGIS feature table to be linked");
    options->layer->description = _("Examples:\n"
				    "\t\tESRI Shapefile: shapefile name\n"
				    "\t\tMapInfo File: mapinfo file name\n"
				    "\t\tPostGIS database: table name");
    options->layer->required = YES;
    options->layer->key_desc = "name";
    
    options->output = G_define_standard_option(G_OPT_V_OUTPUT);
    options->output->required = NO;
    options->output->description = _("Name for output GRASS vector map (default: input layer)");

    flags->format = G_define_flag();
    flags->format->key = 'f';
    flags->format->description = _("List supported formats and exit");
    flags->format->guisection = _("Print");
    flags->format->suppress_required = YES;

    flags->list = G_define_flag();
    flags->list->key = 'l';
    flags->list->description = _("List available layers in data source and exit");
    flags->list->guisection = _("Print");
    flags->list->suppress_required = YES;

    flags->tlist = G_define_flag();
    flags->tlist->key = 't';
    flags->tlist->label = _("List available layers including feature type "
			    "in data source and exit");
    flags->tlist->description = _("Format: layer name,type,projection check");
    flags->tlist->guisection = _("Print");
    flags->tlist->suppress_required = YES;

    flags->topo = G_define_standard_flag(G_FLG_V_TOPO);
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
}
