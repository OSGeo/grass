#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "proto.h"

void parse_options(struct GParm *parm, struct GFlag *flag)
{
    char *desc;

    parm->input[0] = G_define_standard_option(G_OPT_V_INPUT);
    parm->input[0]->description = _("Input vector map from which to select features (A)");
    parm->input[0]->key = "ainput";

    parm->field[0] = G_define_standard_option(G_OPT_V_FIELD);
    parm->field[0]->label = _("Layer number (vector map A)");
    parm->field[0]->key = "alayer";
    parm->field[0]->guisection = _("Selection");

    parm->type[0] = G_define_standard_option(G_OPT_V_TYPE);
    parm->type[0]->label = _("Feature type (vector map A)");
    parm->type[0]->key = "atype";
    parm->type[0]->answer = "point,line,area";
    parm->type[0]->guisection = _("Selection");

    parm->input[1] = G_define_standard_option(G_OPT_V_INPUT);
    parm->input[1]->description = _("Query vector map (B)");
    parm->input[1]->key = "binput";

    parm->field[1] = G_define_standard_option(G_OPT_V_FIELD);
    parm->field[1]->label = _("Layer number (vector map B)");
    parm->field[1]->key = "blayer";
    parm->field[1]->guisection = _("Selection");
    
    parm->type[1] = G_define_standard_option(G_OPT_V_TYPE);
    parm->type[1]->label = _("Feature type (vector map B)");
    parm->type[1]->key = "btype";
    parm->type[1]->answer = "point,line,area";
    parm->type[1]->guisection = _("Selection");
    
    parm->output = G_define_standard_option(G_OPT_V_OUTPUT);

    parm->operator = G_define_option();
    parm->operator->key = "operator";
    parm->operator->type = TYPE_STRING;
    parm->operator->required = YES;
    parm->operator->multiple = NO;
    parm->operator->label =
	_("Operator defines required relation between features");
    parm->operator->description =
	_("A feature is written to output if the result of operation 'ainput operator binput' is true. "
	  "An input feature is considered to be true, if category of given layer is defined.");
    parm->operator->answer = "overlap";
    desc = NULL;
#ifndef HAVE_GEOS
    parm->operator->options = "overlap";
    G_asprintf(&desc,
               "overlap;%s",
               _("features partially or completely overlap"));
    parm->operator->descriptions = desc;

    parm->relate = NULL;
#else
    G_asprintf(&desc,
	       "overlap;%s;"
	       "equals;%s;"
	       "disjoint;%s;"
	       "intersects;%s;"
	       "touches;%s;"
	       "crosses;%s;"
	       "within;%s;"
	       "contains;%s;"
	       "overlaps;%s;"
	       "relate;%s;",
	       _("features partially or completely overlap"),
	       _("features are spatially equals (using GEOS)"),
	       _("features do not spatially intersect (using GEOS)"),
	       _("features spatially intersect (using GEOS)"),
	       _("features spatially touches (using GEOS)"),
	       _("features spatially crosses (using GEOS)"),
	       _("feature A is completely inside feature B (using GEOS)"),
	       _("feature B is completely inside feature A (using GEOS)"),
	       _("features spatially overlap (using GEOS)"),
	       _("feature A is spatially related to feature B (using GEOS, "
		 "requires 'relate' option)"));
    parm->operator->options = "overlap,equals,disjoint,intersects,touches,crosses,within,contains,overlaps,relate";
    parm->operator->descriptions = desc;
    
    parm->relate = G_define_option();
    parm->relate->key = "relate";
    parm->relate->type = TYPE_STRING;
    parm->relate->required = NO;
    parm->relate->multiple = NO;
    parm->relate->description = _("Intersection Matrix Pattern used for 'relate' operator");
#endif

    flag->table = G_define_standard_flag(G_FLG_V_TABLE);
    
    flag->cat = G_define_flag();
    flag->cat->key = 'c';
    flag->cat->description = _("Do not skip features without category");
    
    flag->reverse = G_define_flag();
    flag->reverse->key = 'r';
    flag->reverse->description = _("Reverse selection");
    flag->reverse->guisection = _("Selection");
}
