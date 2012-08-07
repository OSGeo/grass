#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void define_options(struct params *params, struct flags *flags)
{
    params->input = G_define_standard_option(G_OPT_V_INPUT);
    params->input->description = NULL;
    
    params->layer = G_define_standard_option(G_OPT_V_FIELD);
    params->layer->description = NULL;
    
    params->dsn = G_define_option();
    params->dsn->key = "dsn";
    params->dsn->type = TYPE_STRING;
    params->dsn->required = YES;
    params->dsn->label = _("PostGIS output datasource name");
    params->dsn->description =
        _("Starts with 'PG' prefix, eg. 'PG:dbname=grass'");
    
    params->schema = G_define_option();
    params->schema->key = "schema";
    params->schema->type = TYPE_STRING;
    params->schema->required = NO;
    params->schema->description = _("Database schema");
    params->schema->answer = "public";
    
    params->olayer = G_define_option();
    params->olayer->key = "olayer";
    params->olayer->type = TYPE_STRING;
    params->olayer->required = NO;
    params->olayer->label =
        _("Name for output PostGIS layer");
    params->olayer->description = 
        _("If not specified, input name is used");

    flags->table = G_define_flag();
    flags->table->key = 't';
    flags->table->description =
        _("Don't export attribute table");

    flags->topo = G_define_flag();
    flags->topo->key = 'l';
    flags->topo->description =
        _("Export PostGIS topology instead of simple features");
}

