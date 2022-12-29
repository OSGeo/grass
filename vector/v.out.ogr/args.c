#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char **argv,
		struct Options *options, struct Flags *flags)
{
    options->input = G_define_standard_option(G_OPT_V_INPUT);
    options->input->label = _("Name of input vector map to export");
    
    options->field = G_define_standard_option(G_OPT_V_FIELD);
    options->field->guisection = _("Selection");

    options->type = G_define_standard_option(G_OPT_V3_TYPE);
    options->type->options =
	"point,line,boundary,centroid,area,face,kernel,auto";
    options->type->answer = "auto";
    options->type->label = _("Feature type(s)");
    options->type->description =
	_("Combination of types is not supported "
	  "by all output formats. Default is to use first type found in input vector map.");
    options->type->guisection = _("Selection");

    options->dsn = G_define_standard_option(G_OPT_F_OUTPUT);
    options->dsn->label = _("Name of output OGR datasource");
    options->dsn->description =
	_("For example: ESRI Shapefile: filename or directory for storage\n"
          "\t\t\tPostGIS database: connection string");

    options->format = G_define_option();
    options->format->key = "format";
    options->format->type = TYPE_STRING;
    options->format->required = YES;
    options->format->multiple = NO;
    options->format->options = OGR_list_write_drivers();
    options->format->answer = default_driver();
    options->format->description = _("Data format to write");
    
    options->layer = G_define_option();
    options->layer->key = "output_layer";
    options->layer->type = TYPE_STRING;
    options->layer->required = NO;
    options->layer->label =
	_("Name for output OGR layer. If not specified, input name is used");
    options->layer->description =
	_("For example: ESRI Shapefile: shapefile name\n"
          "\t\t\tPostGIS database: table name");
    options->layer->guisection = _("Creation");

    options->otype = G_define_standard_option(G_OPT_V_TYPE);
    options->otype->key = "output_type";
    options->otype->options = "line,boundary";
    options->otype->answer = "";
    options->otype->description = _("Optionally change default output type");
    G_asprintf((char **) &options->otype->descriptions,
	       "line;%s;boundary;%s",
	       _("export area boundaries as linestrings"),
               _("export lines as polygons"));
    options->otype->guisection = _("Creation");

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
    flags->update->description = _("Open an existing OGR datasource for update");

    flags->append = G_define_flag();
    flags->append->key = 'a';
    flags->append->label = _("Append to existing layer");
    flags->append->description = _("A new OGR layer is created if it does not exist");
    flags->append->suppress_overwrite = YES;
    
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

    flags->multi = G_define_flag();
    flags->multi->key = 'm';
    flags->multi->description = 
	_("Export vector data as multi-features");
    flags->multi->guisection = _("Creation");

    flags->new = G_define_flag();
    flags->new->key = 'n';
    flags->new->description =
	_("Create a new empty layer in defined OGR datasource "
	  "and exit. Nothing is read from input.");
    flags->new->guisection = _("Creation");

    flags->list = G_define_flag();
    flags->list->key = 'l';
    flags->list->description = 
	_("List supported output formats and exit");
    flags->list->suppress_required = YES;

    G_option_requires(flags->append, options->layer, NULL);
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
}
