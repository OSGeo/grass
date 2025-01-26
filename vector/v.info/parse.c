#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void parse_args(int argc, char **argv, char **input, char **field, int *history,
                int *columns, int *shell, enum OutputFormat *format_ptr)
{
    struct Option *input_opt, *field_opt, *format_opt;
    struct Flag *hist_flag, *col_flag, *shell_flag, *region_flag, *topo_flag;

    input_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    hist_flag = G_define_flag();
    hist_flag->key = 'h';
    hist_flag->description = _("Print history instead of info and exit");
    hist_flag->guisection = _("Print");

    col_flag = G_define_flag();
    col_flag->key = 'c';
    col_flag->description = _("Print types/names of table columns for "
                              "specified layer instead of info and exit");
    col_flag->guisection = _("Print");

    region_flag = G_define_flag();
    region_flag->key = 'g';
    region_flag->description = _("Print region info in shell script style");
    region_flag->guisection = _("Print");

    shell_flag = G_define_flag();
    shell_flag->key = 'e';
    shell_flag->description =
        _("Print extended metadata info in shell script style");
    shell_flag->guisection = _("Print");

    topo_flag = G_define_flag();
    topo_flag->key = 't';
    topo_flag->description = _("Print topology info in shell script style");
    topo_flag->guisection = _("Print");

    format_opt = G_define_standard_option(G_OPT_F_FORMAT);
    format_opt->options = "plain,shell,json";
    format_opt->descriptions = _("plain;Human readable text output;"
                                 "shell;shell script style text output;"
                                 "json;JSON (JavaScript Object Notation);");
    format_opt->guisection = _("Print");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    *input = G_store(input_opt->answer);
    *field = G_store(field_opt->answer);
    *history = hist_flag->answer ? TRUE : FALSE;
    *columns = col_flag->answer ? TRUE : FALSE;
    *shell = SHELL_NO;
    if (shell_flag->answer)
        *shell |= SHELL_BASIC;
    if (region_flag->answer)
        *shell |= SHELL_REGION;
    if (topo_flag->answer)
        *shell |= SHELL_TOPO;

    if (strcmp(format_opt->answer, "plain") == 0) {
        // if shell flags are specified and format=PLAIN (default),
        // print in shell script format
        if (*shell != 0) {
            *format_ptr = SHELL;
        }
        else {
            *format_ptr = PLAIN;
        }
    }
    else if (strcmp(format_opt->answer, "json") == 0)
        *format_ptr = JSON;
    else {
        *format_ptr = SHELL;
        // if shell flags are specified with format=shell, obey them
        // if only format=shell is specified, print all info
        if (*shell == 0) {
            *shell |= SHELL_BASIC;
            *shell |= SHELL_REGION;
            *shell |= SHELL_TOPO;
        }
    }
}

void parse_history_json(char *buf, char *command, char *gisdbase,
                        char *location, char *mapset, char *user, char *date,
                        char *mapset_path, JSON_Array *record_array)
{
    JSON_Value *info_value = NULL;
    JSON_Object *info_object = NULL;

    if (strncmp(buf, "COMMAND:", 8) == 0) {
        sscanf(buf, "COMMAND: %[^\n]", command);
    }
    else if (strncmp(buf, "GISDBASE:", 9) == 0) {
        sscanf(buf, "GISDBASE: %[^\n]", gisdbase);
    }
    else if (strncmp(buf, "LOCATION:", 9) == 0) {
        sscanf(buf, "LOCATION: %s MAPSET: %s USER: %s DATE: %[^\n]", location,
               mapset, user, date);

        info_value = json_value_init_object();
        if (info_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        info_object = json_object(info_value);

        json_object_set_string(info_object, "command", command);
        snprintf(mapset_path, MAX_STR_LEN, "%s/%s/%s", gisdbase, location,
                 mapset);
        json_object_set_string(info_object, "mapset_path", mapset_path);
        json_object_set_string(info_object, "user", user);
        json_object_set_string(info_object, "date", date);

        json_array_append_value(record_array, info_value);
    }
}
