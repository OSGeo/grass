#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char** argv,
		char** input, char** field,
		int* history, int* columns, int *shell)
{
    int i;
    const char *answer;
    
    struct Option *input_opt, *field_opt;
    struct Flag *hist_flag, *col_flag, *shell_flag, *region_flag, *topo_flag;
    
    input_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    
    hist_flag = G_define_flag();
    hist_flag->key = 'h';
    hist_flag->description = _("Print history instead of info and exit");
    hist_flag->guisection = _("Print");

    col_flag = G_define_flag();
    col_flag->key = 'c';
    col_flag->description =
	_("Print types/names of table columns for specified layer instead of info and exit");
    col_flag->guisection = _("Print");

    region_flag = G_define_flag();
    region_flag->key = 'g';
    region_flag->description = _("Print region info in shell script style");
    region_flag->guisection = _("Print");

    shell_flag = G_define_flag();
    shell_flag->key = 'e';
    shell_flag->description = _("Print extended metadata info in shell script style");
    shell_flag->guisection = _("Print");

    topo_flag = G_define_flag();
    topo_flag->key = 't';
    topo_flag->description = _("Print topology info in shell script style");
    topo_flag->guisection = _("Print");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    *input = G_store(input_opt->answer);
    *field = G_store(field_opt->answer);
    *history = hist_flag->answer ? TRUE : FALSE;
    *columns = col_flag->answer  ? TRUE : FALSE;
    i = 0;
    *shell = SHELL_NO;
    if (shell_flag->answer)
	*shell |= SHELL_BASIC;
    if (region_flag->answer)
	*shell |= SHELL_REGION;
    if (topo_flag->answer)
	*shell |= SHELL_TOPO;
}
