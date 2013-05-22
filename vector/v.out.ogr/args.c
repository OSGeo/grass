#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char **argv,
		struct Options *options, struct Flags *flags)
{
    options->input = G_define_standard_option(G_OPT_V_INPUT);

    options->type = G_define_standard_option(G_OPT_V3_TYPE);
    options->type->options =
	"point,line,boundary,centroid,area,face,kernel,auto";
    options->type->answer = "auto";

    options->type->label = _("Feature type(s)");
    options->type->description =
	_("Combination of types is not supported "
	  "by all output formats. Default is to use first type found in input vector map.");
    options->type->guisection = _("Selection");

    options->field = G_define_standard_option(G_OPT_V_FIELD);
    options->field->guisection = _("Selection");

    options->dsn = G_define_option();
    options->dsn->key = "dsn";
    options->dsn->type = TYPE_STRING;
    options->dsn->required = YES;
    options->dsn->label = _("OGR output datasource name");
    options->dsn->description =
	_("For example: ESRI Shapefile: filename or directory for storage");

    options->layer = G_define_option();
    options->layer->key = "olayer";
    options->layer->type = TYPE_STRING;
    options->layer->required = NO;
    options->layer->label =
	_("OGR layer name. If not specified, input name is used.");
    options->layer->description =
	_("For example: ESRI Shapefile: shapefile name");
    options->layer->guisection = _("Creation");

    options->format = G_define_option();
    options->format->key = "format";
    options->format->type = TYPE_STRING;
    options->format->required = YES;
    options->format->multiple = NO;
    options->format->answer = "ESRI_Shapefile";
    options->format->options = OGR_list_write_drivers();
    options->format->description = _("Data format to write");
    
    options->dsco = G_define_option();
    options->dsco->key = "dsco";
    options->dsco->type = TYPE_STRING;
    options->dsco->required = NO;
    options->dsco->multiple = YES;
    options->dsco->answer = "";
    options->dsco->description =
	_("OGR dataset creation option (format specific, NAME=VALUE)");
    options->dsco->guisection = _("Creation");

    options->lco = G_define_option();
    options->lco->key = "lco";
    options->lco->type = TYPE_STRING;
    options->lco->required = NO;
    options->lco->multiple = YES;
    options->lco->answer = "";
    options->lco->description =
	_("OGR layer creation option (format specific, NAME=VALUE)");
    options->lco->guisection = _("Creation");

    flags->update = G_define_flag();
    flags->update->key = 'u';
    flags->update->description = _("Open an existing datasource for update");

    flags->append = G_define_flag();
    flags->append->key = 'a';
    flags->append->description = _("Append to existing layer instead of creating new if it exists");
    
    flags->nocat = G_define_flag();
    flags->nocat->key = 's';
    flags->nocat->description =
	_("Skip export of GRASS category ID ('cat') attribute");
    flags->nocat->guisection = _("Creation");
    
    flags->cat = G_define_flag();
    flags->cat->key = 'c';
    flags->cat->description =
	_("Also export features without category (not labeled). "
	  "Otherwise only features with category are exported.");
    flags->cat->guisection = _("Selection");

    flags->esristyle = G_define_flag();
    flags->esristyle->key = 'e';
    flags->esristyle->description = _("Use ESRI-style .prj file format "
				      "(applies to Shapefile output only)");
    flags->esristyle->guisection = _("Creation");

    flags->force2d = G_define_flag();
    flags->force2d->key = '2';
    flags->force2d->label = _("Force 2D output even if input is 3D "
                              "(applies to Shapefile output only)");
    flags->force2d->description = _("Useful if input is 3D but all z coordinates are identical");
    flags->force2d->guisection = _("Creation");

    flags->poly = G_define_flag();
    flags->poly->key = 'p';
    flags->poly->description = _("Export lines as polygons");
    flags->poly->guisection = _("Creation");

    flags->new = G_define_flag();
    flags->new->key = 'n';
    flags->new->description =
	_("Create a new empty OGR layer in defined OGR datasource "
	  "and exit. Nothing is read from input.");
    flags->new->guisection = _("Creation");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
}
