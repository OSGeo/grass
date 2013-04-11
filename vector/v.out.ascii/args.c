#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char **argv,
		char **input, char**output, int *format, int *dp, char **delim,
		char **field, char ***columns, char **where, int *region,
		int *old_format, int *header, char **cats, int *type)
{
    struct Option *input_opt, *output_opt, *format_opt, *dp_opt, *delim_opt,
	*field_opt, *column_opt, *where_opt, *cats_opt, *type_opt;
    struct Flag *old_flag, *header_flag, *region_flag;
    
    char *desc;
    
    input_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    field_opt->guisection = _("Selection");
    field_opt->answer = "1";
    
    type_opt = G_define_standard_option(G_OPT_V3_TYPE);
    type_opt->guisection = _("Selection");
    
    output_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    output_opt->label = _("Name for output ASCII file "
			  "or ASCII vector name if '-o' is defined");
    output_opt->description = _("If not given or '-' then standard output");
    output_opt->required = NO;

    column_opt = G_define_standard_option(G_OPT_DB_COLUMNS);
    column_opt->label = _("Name of attribute column(s) to be exported (point mode)");
    column_opt->description = _("\"*\" for all columns");
    column_opt->guisection = _("Points");
    
    cats_opt = G_define_standard_option(G_OPT_V_CATS);
    cats_opt->guisection = _("Selection");
    
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

    format_opt = G_define_option();
    format_opt->key = "format";
    format_opt->type = TYPE_STRING;
    format_opt->required = YES;
    format_opt->multiple = NO;
    format_opt->options = "point,standard,wkt";
    format_opt->answer = "point";
    format_opt->description = _("Output format");
    desc = NULL;
    G_asprintf(&desc,
	       "point;%s;standard;%s;wkt;%s",
	       _("Simple point format (point per row)"),
	       _("GRASS ASCII vector format"),
	       _("OGC well-known text"));
    format_opt->descriptions = desc;

    delim_opt = G_define_standard_option(G_OPT_F_SEP);
    delim_opt->description = _("Field separator (points mode)");
    delim_opt->guisection = _("Points");
    
    dp_opt = G_define_option();
    dp_opt->key = "dp";
    dp_opt->type = TYPE_INTEGER;
    dp_opt->required = NO;
    dp_opt->options = "0-32";
    dp_opt->answer = "8";	/* This value is taken from the lib settings in G_format_easting() */
    dp_opt->description =
	_("Number of significant digits (floating point only)");
    dp_opt->guisection = _("Points");
    
    old_flag = G_define_flag();
    old_flag->key = 'o';
    old_flag->description = _("Create old (version 4) ASCII file");

    header_flag = G_define_flag();
    header_flag->key = 'c';
    header_flag->description = _("Include column names in output (points mode)");
    header_flag->guisection = _("Points");

    region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->description =
	_("Only export points falling within current 3D region (points mode)");
    region_flag->guisection = _("Points");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    *input = G_store(input_opt->answer);
    if (output_opt->answer)
        *output = G_store(output_opt->answer);
    else
        *output = G_store("-"); /* standard output */
    if (format_opt->answer[0] == 'p')
	*format = GV_ASCII_FORMAT_POINT;
    else if (format_opt->answer[0] == 's')
	*format = GV_ASCII_FORMAT_STD;
    else
	*format = GV_ASCII_FORMAT_WKT;
    if (sscanf(dp_opt->answer, "%d", dp) != 1)
	G_fatal_error(_("Failed to interpret 'dp' parameter as an integer"));
    /* the field separator */
    if (strcmp(delim_opt->answer, "\\t") == 0)
	*delim = G_store("\t");
    else if (strcmp(delim_opt->answer, "tab") == 0)
	*delim = G_store("\t");
    else if (strcmp(delim_opt->answer, "space") == 0)
	*delim = G_store(" ");
    else if (strcmp(delim_opt->answer, "comma") == 0)
	*delim = G_store(",");
    else
	*delim = G_store(delim_opt->answer);
    
    *field = G_store(field_opt->answer);
    *type = Vect_option_to_types(type_opt);
    if (*type & GV_AREA) {
	*type |= GV_BOUNDARY;
	*type |= GV_CENTROID;
    }
    *columns = NULL;
    if (column_opt->answer) {
	int i, nopt;
	nopt = 0;
	while(column_opt->answers[nopt++])
	    ;
	*columns = (char **) G_malloc(nopt * sizeof(char *));
	for (i = 0; i < nopt - 1; i++)
	    (*columns)[i] = G_store(column_opt->answers[i]);
	(*columns)[nopt - 1] = NULL;
    }

    *where = NULL;
    if (where_opt->answer) {
	*where = G_store(where_opt->answer);
    }

    *cats = NULL;
    if (cats_opt->answer) {
        *cats = G_store(cats_opt->answer);
    }

    *region = region_flag->answer ? TRUE : FALSE;
    *old_format = old_flag->answer ? TRUE : FALSE;
    *header = header_flag->answer ? TRUE : FALSE;
}
