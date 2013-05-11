#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(struct opts *opt)
{
    opt->reverse = G_define_flag();
    opt->reverse->key = 'r';
    opt->reverse->description =
	_("Reverse transformation; 3D vector features to 2D");

    opt->table = G_define_standard_flag(G_FLG_V_TABLE);
    opt->table->description = _("Do not copy table");

    opt->input = G_define_standard_option(G_OPT_V_INPUT);

    opt->field = G_define_standard_option(G_OPT_V_FIELD_ALL);
    opt->field->guisection = _("Selection");

    opt->type = G_define_standard_option(G_OPT_V_TYPE);
    opt->type->options = "point,line,boundary,centroid";
    opt->type->answer = "point,line,boundary,centroid";
    opt->type->guisection = _("Selection");

    opt->output = G_define_standard_option(G_OPT_V_OUTPUT);

    opt->column = G_define_standard_option(G_OPT_DB_COLUMN);
    opt->column->label = _("Name of attribute column used for height");
    opt->column->description =
	_("Can be used for reverse transformation, to store height of points");
    opt->column->guisection = _("Height");

    opt->height = G_define_option();
    opt->height->key = "height";
    opt->height->type = TYPE_DOUBLE;
    opt->height->required = NO;
    opt->height->multiple = NO;
    opt->height->description = _("Fixed height for 3D vector features");
    opt->height->guisection = _("Height");
    
    return;
}
