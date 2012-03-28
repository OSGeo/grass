#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char **argv,
		struct _options *options, struct _flags *flags)
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


    options->format = G_define_option();
    options->format->key = "format";
    options->format->description = _("Format for output vector data");
    options->format->required = YES;
    options->format->type = TYPE_STRING;
    options->format->options = format_options();
#ifdef HAVE_OGR
    options->format->answer = "ESRI_Shapefile";
#else
#ifdef HAVE_POSTGRES
    options->format->answer = "PostgreSQL";
#endif /* HAVE_POSTGRES */
#endif /* HAVE_OGR */
    
    options->opts = G_define_option();
    options->opts->key = "options";
    options->opts->label = _("Creation options");
    options->opts->description = _("Examples:\n"
				  "\t\t'SHPT=POINTZ': create 3D point Shapefile data\n"
				  "\t\t'GEOM_TYPE=geography': use geography PostGIS data\n"
				  "\t\t'SCHEMA=grass': create new PostGIS tables in 'grass' schema");
    options->opts->required = NO;
    options->opts->multiple = YES;
    options->opts->type = TYPE_STRING;

    flags->f = G_define_flag();
    flags->f->key = 'f';
    flags->f->description = _("List supported formats and exit");
    flags->f->guisection = _("Print");
    flags->f->suppress_required = YES;

    flags->r = G_define_flag();
    flags->r->key = 'r';
    flags->r->description = _("Cease using OGR/PostGIS, revert to native output and exit");
    flags->r->suppress_required = YES;
    flags->r->guisection = _("Native");
    
    flags->p = G_define_flag();
    flags->p->key = 'p';
    flags->p->description = _("Print current status");
    flags->p->guisection = _("Print");
    flags->p->suppress_required = YES;

    flags->g = G_define_flag();
    flags->g->key = 'g';
    flags->g->description = _("Print current status in shell script style");
    flags->g->guisection = _("Print");
    flags->g->suppress_required = YES;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
}
