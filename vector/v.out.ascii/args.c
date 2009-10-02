#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char **argv,
		char **input, char**output, int *format, int *dp, char **delim,
		int *field, char ***columns, char **where, int *region, int *old_format)
{
    struct Option *input_opt, *output_opt, *format_opt, *dp_opt, *delim_opt,
	*field_opt, *column_opt, *where_opt;
    struct Flag *old_flag, *region_flag;
    
    input_opt = G_define_standard_option(G_OPT_V_INPUT);

    output_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    output_opt->description =
	_("Path to resulting ASCII file ('-' for standard output) "
	  "or ASCII vector name if '-o' is defined");
    output_opt->answer = "-";
    
    format_opt = G_define_option();
    format_opt->key = "format";
    format_opt->type = TYPE_STRING;
    format_opt->required = YES;
    format_opt->multiple = NO;
    format_opt->options = "point,standard";
    format_opt->answer = "point";
    format_opt->description = _("Output format");
    
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
    
    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    field_opt->guisection = _("Selection");

    column_opt = G_define_standard_option(G_OPT_DB_COLUMNS);
    column_opt->description = _("Name of attribute column(s) to be exported (point mode)");
    column_opt->guisection = _("Points");
    
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

    old_flag = G_define_flag();
    old_flag->key = 'o';
    old_flag->description = _("Create old (version 4) ASCII file");

    region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->description =
	_("Only export points falling within current 3D region (points mode)");
    region_flag->guisection = _("Points");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    *input = G_store(input_opt->answer);
    *output = G_store(output_opt->answer);
    if (strcmp(format_opt->answer, "point") == 0)
	*format = GV_ASCII_FORMAT_POINT;
    else
	*format = GV_ASCII_FORMAT_ALL;
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
    
    *field = atoi(field_opt->answer);
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
    *region = region_flag->answer ? 1 : 0;
    *old_format = old_flag->answer ? 1 : 0;
}
