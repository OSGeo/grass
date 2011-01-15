#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char** argv,
		char** input, char** field,
		int* history, int* columns, int *shell)
{
    int i;
    const char *answer;
    
    struct Option *input_opt, *field_opt, *print_opt;
    struct Flag *hist_flag, *col_flag;
    
    input_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    print_opt = G_define_option();
    print_opt->key = "shell";
    print_opt->multiple = YES;
    print_opt->options = "basic,region,topo";
    print_opt->label = _("Print info in shell script style");
    print_opt->description = _("Ignored if -h or -c flags are given");
    print_opt->descriptions = _("basic;basic info only;"
				"region;map region;"
				"topo;topology information");
    print_opt->guisection = _("Print");
    
    hist_flag = G_define_flag();
    hist_flag->key = 'h';
    hist_flag->description = _("Print history instead of info and exit");
    hist_flag->guisection = _("Print");

    col_flag = G_define_flag();
    col_flag->key = 'c';
    col_flag->description =
	_("Print types/names of table columns for specified layer instead of info and exit");
    col_flag->guisection = _("Print");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    *input = G_store(input_opt->answer);
    *field = G_store(field_opt->answer);
    *history = hist_flag->answer ? TRUE : FALSE;
    *columns = col_flag->answer  ? TRUE : FALSE;
    i = 0;
    *shell = NO_INFO;
    if (print_opt->answer) {
	while(print_opt->answers[i]) {
	    answer = print_opt->answers[i++];
	    if (strcmp(answer, "basic") == 0)
		*shell |= BASIC_INFO;
	    else if (strcmp(answer, "region") == 0)
		*shell |= REGION_INFO;
	    else if (strcmp(answer, "topo") == 0)
		*shell |= TOPO_INFO;
	}
    }
}
