#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "proto.h"

void parse_options(struct GParm *parm, struct GFlag *flag)
{
    parm->input[0] = G_define_standard_option(G_OPT_V_INPUT);
    parm->input[0]->description = _("Name of input vector map (A)");
    parm->input[0]->key = "ainput";

    parm->field[0] = G_define_standard_option(G_OPT_V_FIELD);
    parm->field[0]->label = _("Layer number (vector map A)");
    parm->field[0]->key = "alayer";
    parm->field[0]->guisection = _("Selection");

    parm->type[0] = G_define_standard_option(G_OPT_V_TYPE);
    parm->type[0]->label = _("Feature type (vector map A)");
    parm->type[0]->key = "atype";
    parm->type[0]->guisection = _("Selection");

    parm->input[1] = G_define_standard_option(G_OPT_V_INPUT);
    parm->input[1]->description = _("Name of input vector map (B)");
    parm->input[1]->key = "binput";

    parm->field[1] = G_define_standard_option(G_OPT_V_FIELD);
    parm->field[1]->label = _("Layer number (vector map B)");
    parm->field[1]->key = "blayer";
    parm->field[1]->guisection = _("Selection");
    
    parm->type[1] = G_define_standard_option(G_OPT_V_TYPE);
    parm->type[1]->label = _("Feature type (vector map B)");
    parm->type[1]->key = "btype";
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
#ifndef HAVE_GEOS
    parm->operator->options = "overlaps";
    parm->operator->answer = "overlaps";
    parm->operator->descriptions = _("overlaps;features partially or completely overlap");

    parm->relate = NULL;
#else
    parm->operator->options = "equals,disjoint,intersects,touches,crosses,within,contains,overlaps,relate";
    parm->operator->descriptions = _("equals;features are spatially equals (requires flag 'g');"
				    "disjoint;features do not spatially intersect (requires flag 'g');"
				    "intersects;features spatially intersect (requires flag 'g');"
				    "touches;features spatially touches (requires flag 'g');"
				    "crosses;features spatially crosses (requires flag 'g');"
				    "within;feature A is completely inside feature B (requires flag 'g');"
				    "contains;feature B is completely inside feature A (requires flag 'g');"
				    "overlaps;features spatilly overlap;"
				    "relate;feature A is spatially related to feature B "
				    "(requires 'relate' option and flag 'g');");
    
    parm->relate = G_define_option();
    parm->relate->key = "relate";
    parm->relate->type = TYPE_STRING;
    parm->relate->required = NO;
    parm->relate->multiple = NO;
    parm->relate->description = _("Intersection Matrix Pattern used for 'relate' operator");
#endif

    flag->table = G_define_flag();
    flag->table->key = 't';
    flag->table->description = _("Do not create attribute table");
    
    flag->cat = G_define_flag();
    flag->cat->key = 'c';
    flag->cat->description = _("Do not skip features without category");
    
    flag->reverse = G_define_flag();
    flag->reverse->key = 'r';
    flag->reverse->description = _("Reverse selection");
    flag->reverse->guisection = _("Selection");
#ifdef HAVE_GEOS
    flag->geos = G_define_flag();
    flag->geos->key = 'g';
    flag->geos->description = _("Use GEOS operators");
#else
    flag->geos = NULL;
#endif
}
