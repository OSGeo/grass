#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/glocale.h>
#include "global.h"

int parse_units();
int parse_option();
int match();

int parse_command_line(int argc, char *argv[])
{
    int ncols;

    struct
    {
	struct Option *vect;
	struct Option *option;
	struct Option *type;
	struct Option *field;
	struct Option *qfield;
	struct Option *col;
	struct Option *units;
	struct Option *qcol;
        struct Option *fs;
    } parms;
    struct
    {
	struct Flag *p, *s, *t;
    } flags;
    char *desc;

    parms.vect = G_define_standard_option(G_OPT_V_MAP);

    parms.field = G_define_standard_option(G_OPT_V_FIELD);
    parms.field->label = _("Layer number or name (write to)");

    parms.type = G_define_standard_option(G_OPT_V_TYPE);
    parms.type->options = "point,line,boundary,centroid";
    parms.type->answer = "point,line,boundary,centroid";
    parms.type->label = _("Feature type");
    parms.type->description =
	_("For coor valid point/centroid, "
	  "for length valid line/boundary");
    parms.type->guisection = _("Selection");
    
    parms.option = G_define_option();
    parms.option->key = "option";
    parms.option->type = TYPE_STRING;
    parms.option->required = YES;
    parms.option->multiple = NO;
    parms.option->options =
	"cat,area,compact,fd,perimeter,length,count,coor,start,end,sides,query,slope,sinuous,azimuth";
    parms.option->description = _("Value to upload");
    desc = NULL;
    G_asprintf(&desc,
	       "cat;%s;"
	       "area;%s;"
	       "compact;%s;"
	       "fd;%s;"
	       "perimeter;%s;"
	       "length;%s;"
	       "count;%s;"
	       "coor;%s;"
	       "start;%s;"
	       "end;%s;"
	       "sides;%s;"
	       "query;%s;"
	       "slope;%s;"
	       "sinuous;%s;"
	       "azimuth;%s;",
	       _("insert new row for each category if doesn't exist yet"),
	       _("area size"),
	       _("compactness of an area, calculated as \n"
		 "              compactness = perimeter / (2 * sqrt(PI * area))"),
	       _("fractal dimension of boundary defining a polygon, calculated as \n"
		 "              fd = 2 * (log(perimeter) / log(area))"),
	       _("perimeter length of an area"),
	       _("line length"),
	       _("number of features for each category"),
	       _("point coordinates, X,Y or X,Y,Z"),
	       _("line/boundary starting point coordinates, X,Y or X,Y,Z"),
	       _("line/boundary end point coordinates, X,Y or X,Y,Z"),
	       _("categories of areas on the left and right side of the boundary, "
		 "'qlayer' is used for area category"),
	       _("result of a database query for all records of the geometry"
		 "(or geometries) from table specified by 'qlayer' option"),
	       _("slope steepness of vector line or boundary"),
	       _("line sinuousity, calculated as line length / distance between end points"),
	       _("line azimuth, calculated as angle between North direction and endnode direction at startnode"));
    parms.option->descriptions = desc;

    parms.col = G_define_standard_option(G_OPT_DB_COLUMNS);
    parms.col->label = _("Name of attribute column(s) to populate");
    parms.col->required = YES;

    parms.units = G_define_standard_option(G_OPT_M_UNITS);
    parms.units->options =
	"miles,feet,meters,kilometers,acres,hectares,radians,degrees";
    
    parms.qfield = G_define_standard_option(G_OPT_V_FIELD);
    parms.qfield->key = "qlayer";
    parms.qfield->label = _("Query layer number or name (read from)");
    parms.qfield->guisection = _("Query");
    parms.qfield->required = NO;
    
    parms.qcol = G_define_standard_option(G_OPT_DB_COLUMN);
    parms.qcol->key = "qcolumn";
    parms.qcol->label = _("Name of attribute column used for 'query' option");
    parms.qcol->description = _("E.g. 'cat', 'count(*)', 'sum(val)'");
    parms.qcol->required = NO;
    parms.qcol->guisection = _("Query");

    parms.fs = G_define_standard_option(G_OPT_F_SEP);
    parms.fs->label = _("Field separator for print mode");
    parms.fs->guisection = _("Print");
    
    flags.p = G_define_flag();
    flags.p->key = 'p';
    flags.p->description = _("Print only");
    flags.p->guisection = _("Print");
    flags.p->suppress_required = YES;
    
    flags.s = G_define_flag();
    flags.s->key = 's';
    flags.s->description = _("Only print SQL statements");
    flags.s->guisection = _("Print");

    flags.t = G_define_flag();
    flags.t->key = 'c';
    flags.t->description =
	_("Print also totals for option length, area, or count");
    flags.t->guisection = _("Print");
    flags.t->suppress_required = YES;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* check for required options */
    if (!parms.vect->answer)
        G_fatal_error(_("Required parameter <%s> not set:\n\t(%s)"),
                      parms.vect->key, parms.vect->description);
    if (!parms.option->answer)
        G_fatal_error(_("Required parameter <%s> not set:\n\t(%s)"),
                      parms.option->key, parms.option->description);

    options.print = flags.p->answer;
    options.sql = flags.s->answer;
    options.total = flags.t->answer;

    options.name = parms.vect->answer;

    options.type = Vect_option_to_types(parms.type);
    options.field = atoi(parms.field->answer);
    options.qfield = atoi(parms.qfield->answer);

    options.option = parse_option(parms.option->answer);
    options.units = parse_units(parms.units->answer);

    options.fs = G_option_to_separator(parms.fs);
    
    /* Check number of columns */
    ncols = 0;
    options.col[0] = NULL;
    options.col[1] = NULL;
    options.col[2] = NULL;
    while (parms.col->answers && parms.col->answers[ncols]) {
	options.col[ncols] = G_store(parms.col->answers[ncols]);
	ncols++;
    }

    if (!options.print && ! options.total) {
	if (options.option == O_AREA || options.option == O_LENGTH || options.option == O_COUNT ||
	    options.option == O_QUERY || options.option == O_COMPACT || options.option == O_FD ||
	    options.option == O_PERIMETER || options.option == O_SLOPE || options.option == O_SINUOUS ||
	    options.option == O_AZIMUTH) {	/* one column required */
	    if (ncols != 1) {
		G_fatal_error(_("This option requires one column"));
	    }
	}
	else if (options.option == O_SIDES) {
	    if (ncols != 2) {
		G_fatal_error(_("This option requires two columns"));
	    }
	}
	else if (options.option == O_COOR || options.option == O_START || options.option == O_END) {
	    if (ncols < 2) {
		G_fatal_error(_("This option requires at least two columns"));
	    }
	}
    }

    if (options.option == O_QUERY && !parms.qcol->answers)
	G_fatal_error(_("Parameter 'qcolumn' must be specified for 'option=query'"));

    options.qcol = parms.qcol->answer;

    if (options.option == O_SIDES && !(options.type | GV_BOUNDARY))
	G_fatal_error(_("The 'sides' option makes sense only for boundaries"));

    if (options.option == O_SINUOUS && !(options.type | GV_LINES))
	G_fatal_error(_("The 'sinuous' option makes sense only for lines"));
    
    if (options.option == O_AZIMUTH && !(options.type | GV_LINES))
	G_fatal_error(_("The 'azimuth' option makes sense only for lines"));


    return 0;
}

int parse_units(char *s)
{
    int x = 0;

    if (match(s, "miles", 2))
	x = U_MILES;
    else if (match(s, "feet", 1))
	x = U_FEET;
    else if (match(s, "meters", 2))
	x = U_METERS;
    else if (match(s, "kilometers", 1))
	x = U_KILOMETERS;
    else if (match(s, "acres", 1))
	x = U_ACRES;
    else if (match(s, "hectares", 1))
	x = U_HECTARES;
    else if (match(s, "radians", 1))
	x = U_RADIANS;
    else if (match(s, "degrees", 1))
	x = U_DEGREES;

    return x;
}

int parse_option(char *s)
{
    int x = 0;

    if (strcmp(s, "cat") == 0)
	x = O_CAT;
    else if (strcmp(s, "area") == 0)
	x = O_AREA;
    else if (strcmp(s, "length") == 0)
	x = O_LENGTH;
    else if (strcmp(s, "count") == 0)
	x = O_COUNT;
    else if (strcmp(s, "coor") == 0)
	x = O_COOR;
    else if (strcmp(s, "start") == 0)
	x = O_START;
    else if (strcmp(s, "end") == 0)
	x = O_END;
    else if (strcmp(s, "sides") == 0)
	x = O_SIDES;
    else if (strcmp(s, "query") == 0)
	x = O_QUERY;
    else if (strcmp(s, "compact") == 0)
	x = O_COMPACT;
    else if (strcmp(s, "fd") == 0)
	x = O_FD;
    else if (strcmp(s, "perimeter") == 0)
	x = O_PERIMETER;
    else if (strcmp(s, "slope") == 0)
	x = O_SLOPE;
    else if (strcmp(s, "sinuous") == 0)
	x = O_SINUOUS;
    else if (strcmp(s, "azimuth") == 0)
	x = O_AZIMUTH;

    return x;
}

int match(char *s, char *key, int min)
{
    size_t len;

    if (!s)
	return 0;
    len = strlen(s);
    if (len < (size_t) min)
	return 0;
    return strncmp(s, key, len) == 0;
}
