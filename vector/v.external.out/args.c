#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char **argv,
		struct _options *options, struct _flags *flags)
{
    options->dir = G_define_option();
    options->dir->key = "directory";
    options->dir->description = _("Name of output directory");
    options->dir->required = YES;
    options->dir->type = TYPE_STRING;
    options->dir->key_desc = "path";

    options->ext = G_define_option();
    options->ext->key = "extension";
    options->ext->description = _("Extension for output files");
    options->ext->required = NO;
    options->ext->type = TYPE_STRING;

    options->format = G_define_option();
    options->format->key = "format";
    options->format->description = _("Format of output files");
    options->format->required = YES;
    options->format->type = TYPE_STRING;
    options->format->options = format_list();
    options->format->answer = "ESRI Shapefile";

    options->opts = G_define_option();
    options->opts->key = "options";
    options->opts->description = _("Creation options");
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
    flags->r->description = _("Cease using OGR and revert to native output");
    flags->r->suppress_required = YES;

    flags->p = G_define_flag();
    flags->p->key = 'p';
    flags->p->description = _("Print current status");
    flags->p->guisection = _("Print");
    flags->p->suppress_required = YES;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
}
