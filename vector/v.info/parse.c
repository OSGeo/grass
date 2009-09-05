#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char** argv,
		char** input, char** field,
		int* history, int* columns, int* region, int* topo, int* title, int* level1)
{
    struct Option *input_opt, *field_opt;
    struct Flag *hist_flag, *col_flag, *region_flag, *topo_flag, *title_flag, *level1_flag;

    input_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    hist_flag = G_define_flag();
    hist_flag->key = 'h';
    hist_flag->description = _("Print vector history instead of info");
    hist_flag->guisection = _("Print");

    col_flag = G_define_flag();
    col_flag->key = 'c';
    col_flag->description =
	_("Print types/names of table columns for specified layer instead of info");
    col_flag->guisection = _("Print");

    region_flag = G_define_flag();
    region_flag->key = 'g';
    region_flag->description = _("Print map region only");
    region_flag->guisection = _("Print");

    title_flag = G_define_flag();
    title_flag->key = 'm';
    title_flag->description = _("Print map title only");
    title_flag->guisection = _("Print");

    topo_flag = G_define_flag();
    topo_flag->key = 't';
    topo_flag->description = _("Print topology information only");
    topo_flag->guisection = _("Print");

    level1_flag = G_define_flag();
    level1_flag->key = 'l';
    level1_flag->description = _("Open Vector without topology (level 1)");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    *input = G_store(input_opt->answer);
    *field = G_store(field_opt->answer);

    *history = hist_flag-> answer ? 1 : 0;
    *columns = col_flag-> answer ? 1 : 0;
    *region  = region_flag-> answer ? 1 : 0;
    *title   = title_flag-> answer ? 1 : 0;
    *topo    = topo_flag-> answer ? 1 : 0;
    *level1  = level1_flag-> answer ? 1 : 0;
}
