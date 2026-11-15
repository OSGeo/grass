#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char **argv, char **input, char **field, int *history,
                int *columns, int *print_content, enum OutputFormat *format_ptr)
{
    struct Option *input_opt, *field_opt, *format_opt;
    struct Flag *hist_flag, *col_flag, *shell_flag, *region_flag, *topo_flag;

    input_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    col_flag = G_define_flag();
    col_flag->key = 'c';
    col_flag->label = _("Print attribute table columns instead of info");
    col_flag->description =
        _("Column names and types are printed for the "
          "specified layer if it has an attribute table database connection");
    col_flag->guisection = _("Print");

    hist_flag = G_define_flag();
    hist_flag->key = 'h';
    hist_flag->description = _("Print data history instead of info");
    hist_flag->guisection = _("Print");

    shell_flag = G_define_flag();
    shell_flag->key = 'e';
    shell_flag->description = _("Print only metadata info");
    shell_flag->guisection = _("Print");

    region_flag = G_define_flag();
    region_flag->key = 'g';
    region_flag->description = _("Print only region info");
    region_flag->guisection = _("Print");

    topo_flag = G_define_flag();
    topo_flag->key = 't';
    topo_flag->description = _("Print only topology info");
    topo_flag->guisection = _("Print");

    format_opt = G_define_standard_option(G_OPT_F_FORMAT);
    format_opt->options = "plain,shell,json,csv";
    format_opt->descriptions = _("plain;Human readable text output;"
                                 "shell;shell script style text output;"
                                 "json;JSON (JavaScript Object Notation);"
                                 "csv;CSV (Comma Separated Values);");
    format_opt->answer = NULL;
    format_opt->required = NO;
    format_opt->guisection = _("Print");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    *input = G_store(input_opt->answer);
    *field = G_store(field_opt->answer);
    *history = hist_flag->answer ? TRUE : FALSE;
    *columns = col_flag->answer ? TRUE : FALSE;
    *print_content = PRINT_CONTENT_UNSET;
    if (shell_flag->answer)
        *print_content |= PRINT_CONTENT_TEXT;
    if (region_flag->answer)
        *print_content |= PRINT_CONTENT_REGION;
    if (topo_flag->answer)
        *print_content |= PRINT_CONTENT_TOPO;

    if (format_opt->answer && strcmp(format_opt->answer, "plain") == 0) {
        *format_ptr = PLAIN;
    }
    else if (format_opt->answer && strcmp(format_opt->answer, "json") == 0) {
        *format_ptr = JSON;
        if (*print_content == PRINT_CONTENT_UNSET) {
            *print_content |= PRINT_CONTENT_TEXT;
            *print_content |= PRINT_CONTENT_REGION;
            *print_content |= PRINT_CONTENT_TOPO;
        }
    }
    else if (format_opt->answer && strcmp(format_opt->answer, "csv") == 0) {
        if (!*columns) {
            G_fatal_error(_("format=csv is only valid with -c flag."));
        }
        *format_ptr = CSV;
    }
    else if (!format_opt->answer && *print_content == PRINT_CONTENT_UNSET &&
             !*columns && !*history) {
        // No flags and no format specified, default to plain.
        *format_ptr = PLAIN;
    }
    else if (!format_opt->answer && *columns) { // -c without `format=`
        // Backward compatibilty: sep should be pipe + header skipped
        // sep and header is handled during printing
        *format_ptr = NONE;
    }
    else {
        *format_ptr = SHELL;
        // If flags are specified with format=shell, obey them just as for JSON.
        // If only format=shell is specified, print all info.
        if (*print_content == PRINT_CONTENT_UNSET) {
            *print_content |= PRINT_CONTENT_TEXT;
            *print_content |= PRINT_CONTENT_REGION;
            *print_content |= PRINT_CONTENT_TOPO;
        }
    }
}
