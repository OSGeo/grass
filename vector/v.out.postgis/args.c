#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void define_options(struct params *params, struct flags *flags)
{
    params->input = G_define_standard_option(G_OPT_V_INPUT);
    params->input->description = NULL;

    params->type = G_define_standard_option(G_OPT_V3_TYPE);
    params->type->options =
	"point,line,boundary,centroid,area,face,kernel,auto";
    params->type->answer = "auto";
    params->type->guisection = _("Selection");

    params->layer = G_define_standard_option(G_OPT_V_FIELD);
    params->layer->description = NULL;
    params->layer->guisection = _("Selection");

    params->dsn = G_define_option();
    params->dsn->key = "dsn";
    params->dsn->type = TYPE_STRING;
    params->dsn->required = YES;
    params->dsn->label = _("Name for output PostGIS datasource");
    params->dsn->description =
        _("Starts with 'PG' prefix, eg. 'PG:dbname=grass'");
    
    params->olayer = G_define_option();
    params->olayer->key = "olayer";
    params->olayer->type = TYPE_STRING;
    params->olayer->required = NO;
    params->olayer->key_desc = "name";
    params->olayer->label =
        _("Name for output PostGIS layer");
    params->olayer->description = 
        _("If not specified, input name is used");
    params->olayer->guisection = _("Creation");

    params->olink = G_define_standard_option(G_OPT_V_OUTPUT);
    params->olink->key = "olink";
    params->olink->required = NO;
    params->olink->label = 
        _("Name for output vector map defined as a link to the PostGIS feature table");
    params->olink->description = 
        _("If not specified, the vector link is not created. "
          "The link can be also manually created by 'v.external' module.");

    params->opts = G_define_option();
    params->opts->key = "options";
    params->opts->label = _("Creation options");
    params->opts->description = _("Examples:\n"
                                  "\t\t'FID=cat': define feature id column 'cat'\n"
                                  "\t\t'GEOMETRY_NAME=wkb_geometry': define geometry column 'wkb_geometry'\n"
                                  "\t\t'SPATIAL_INDEX=NO': do not create spatial index on geometry column");
    params->opts->required = NO;
    params->opts->multiple = YES;
    params->opts->type = TYPE_STRING;
    params->opts->key_desc = "key=value";
    params->opts->guisection = _("Creation");

    flags->table = G_define_flag();
    flags->table->key = 't';
    flags->table->description =
        _("Don't export attribute table");
    flags->table->guisection = _("Creation");

    flags->topo = G_define_flag();
    flags->topo->key = 'l';
    flags->topo->description =
        _("Export PostGIS topology instead of simple features");
    flags->topo->guisection = _("Creation");

    flags->force2d = G_define_flag();
    flags->force2d->key = '2';
    flags->force2d->label = _("Force 2D output even if input is 3D ");
    flags->force2d->description = _("Useful if input is 3D but all z coordinates are identical");
    flags->force2d->guisection = _("Creation");

}

