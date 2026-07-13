/*!
   \file lib/gis/parser_md_python.c

   \brief GIS Library - Argument parsing functions (Markdown output - Python)

   (C) 2025 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Vaclav Petras
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

static void print_python_short_flag(FILE *file, const char *key,
                                    const char *label, const char *description,
                                    const char *indent);
static void print_python_long_flag(FILE *file, const char *key,
                                   const char *label, const char *description,
                                   const char *indent);
static void print_python_option(FILE *file, const struct Option *opt,
                                const char *indent, bool tools_api);
static void print_python_example(FILE *file, const char *python_function,
                                 const char *output_format_default,
                                 const char *indent, bool tools_api);
static void print_python_tuple(FILE *file, const char *type, int num_items);

void print_python_short_flag(FILE *file, const char *key, const char *label,
                             const char *description, const char *indent)
{
    fprintf(file, "%s", indent);
    G__md_print_escaped(file, "\t", indent);
    fprintf(file, "**%s**", key);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    if (label != NULL) {
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t\t", indent);
        G__md_print_escaped(file, label, indent);
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
    }
    if (description != NULL) {
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t\t", indent);
        G__md_print_escaped(file, description, indent);
    }
}

void print_python_long_flag(FILE *file, const char *key, const char *label,
                            const char *description, const char *indent)
{
    fprintf(file, "%s**%s** : bool, *optional*", indent, key);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    if (label != NULL) {
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t", indent);
        G__md_print_escaped(file, label, indent);
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
    }
    if (description != NULL) {
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t", indent);
        G__md_print_escaped(file, description, indent);
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
    }
    fprintf(file, "%s", indent);
    G__md_print_escaped(file, "\t", indent);
    const char *flag_default = "*None*";
    fprintf(file, "Default: %s", flag_default);
}

void print_python_tuple(FILE *file, const char *type, int num_items)
{
    fprintf(file, "tuple[%s", type);
    for (int i = 1; i < num_items; i++) {
        fprintf(file, ", %s", type);
    }
    fprintf(file, "]");
}

void print_python_option(FILE *file, const struct Option *opt,
                         const char *indent, bool tools_api)
{
    const char *type;

    switch (opt->type) {
    case TYPE_INTEGER:
        type = "int";
        break;
    case TYPE_DOUBLE:
        type = "float";
        break;
    case TYPE_STRING:
        type = "str";
        break;
    default:
        type = "str";
        break;
    }

    char age[KEYLENGTH];
    char element[KEYLENGTH];
    char prompt_description[KEYLENGTH];
    if (opt->gisprompt) {
        G__split_gisprompt(opt->gisprompt, age, element, prompt_description);
        if (tools_api && !opt->multiple && opt->type == TYPE_STRING) {
            if (G_strncasecmp("old", age, 3) == 0 &&
                G_strncasecmp("file", element, 4) == 0) {
                type = "str | io.StringIO";
            }
            if (G_strncasecmp("old", age, 3) == 0 &&
                G_strncasecmp("cell", element, 4) == 0) {
                type = "str | np.ndarray";
            }
            if (G_strncasecmp("new", age, 3) == 0 &&
                G_strncasecmp("cell", element, 4) == 0) {
                type = "str | type(np.ndarray) | type(np.array) | "
                       "type(gs.array.array)";
            }
        }
    }

    fprintf(file, "%s**%s** : ", indent, opt->key);
    int tuple_items = G__option_num_tuple_items(opt);
    if (opt->multiple) {
        if (tuple_items) {
            fprintf(file, "list[");
            print_python_tuple(file, type, tuple_items);
            fprintf(file, "] | ");
            print_python_tuple(file, type, tuple_items);
            fprintf(file, " | list[%s] | str", type);
        }
        else {
            if (strcmp(type, "str")) {
                // If it is not a string, we also show it can be a string
                // because that may be more relevant to show that for
                // lists due to examples (it is possible for single value as
                // well).
                fprintf(file, "%s | list[%s] | str", type, type);
            }
            else {
                fprintf(file, "%s | list[%s]", type, type);
            }
        }
    }
    else if (tuple_items) {
        print_python_tuple(file, type, tuple_items);
        fprintf(file, " | list[%s] | str", type);
    }
    else {
        fprintf(file, "%s", type);
    }
    if (opt->required) {
        fprintf(file, ", *required*");
    }
    else {
        fprintf(file, ", *optional*");
    }

    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    if (opt->label) {
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t", indent);
        G__md_print_escaped(file, opt->label, indent);
    }
    if (opt->description) {
        if (opt->label) {
            fprintf(file, MD_NEWLINE);
            fprintf(file, "\n");
        }
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t", indent);
        G__md_print_escaped(file, opt->description, indent);
    }
    if (opt->gisprompt || opt->key_desc) {
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t", indent);
        fprintf(file, "%s: ", _("Used as"));
    }
    if (opt->gisprompt) {
        if (strcmp(age, "new") == 0)
            fprintf(file, "output, ");
        else if (strcmp(age, "old") == 0)
            fprintf(file, "input, ");
        // While element more strictly expresses how the value will be
        // used given that the parser may read that information, desc
        // is meant as a user-facing representation of the same
        // information.
        fprintf(file, "%s", prompt_description);
    }
    if (opt->gisprompt && opt->key_desc) {
        fprintf(file, ", ");
    }
    if (opt->key_desc) {
        fprintf(file, "*%s*", opt->key_desc);
    }

    if (opt->options) {
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t", indent);
        fprintf(file, "%s: *", _("Allowed values"));
        G__md_print_escaped_for_options(file, opt->options);
        fprintf(file, "*");
    }

    if (opt->descs) {
        int i = 0;

        while (opt->opts[i]) {
            if (opt->descs[i]) {
                fprintf(file, MD_NEWLINE);
                fprintf(file, "\n");
                fprintf(file, "%s", indent);
                char *thumbnails = NULL;
                if (opt->gisprompt) {
                    if (strcmp(opt->gisprompt, "old,colortable,colortable") ==
                        0)
                        thumbnails = "colortables";
                    else if (strcmp(opt->gisprompt, "old,barscale,barscale") ==
                             0)
                        thumbnails = "barscales";
                    else if (strcmp(opt->gisprompt,
                                    "old,northarrow,northarrow") == 0)
                        thumbnails = "northarrows";

                    if (thumbnails) {
                        G__md_print_escaped(file, "\t\t", indent);
                        fprintf(file, "![%s](%s/%s.png) ", opt->opts[i],
                                thumbnails, opt->opts[i]);
                    }
                    else {
                        G__md_print_escaped(file, "\t\t", indent);
                    }
                }
                G__md_print_escaped(file, "\t", indent);
                fprintf(file, "**");
                G__md_print_escaped(file, opt->opts[i], indent);
                fprintf(file, "**: ");
                G__md_print_escaped(file, opt->descs[i], indent);
            }
            i++;
        }
    }

    if (opt->def && opt->def[0] != '\0') {
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t", indent);
        fprintf(file, "%s:", _("Default"));
        fprintf(file, " *");
        G__md_print_escaped(file, opt->def, indent);
        fprintf(file, "*");
    }
}

void print_python_example(FILE *file, const char *python_function,
                          const char *output_format_default, const char *indent,
                          bool tools_api)
{
    fprintf(file, "\n%sExample:\n", indent);

    fprintf(file, "\n%s```python\n", indent);
    bool first_parameter_printed = false;
    if (tools_api) {
        char *tool_name = G_store(st->pgm_name);
        G_strchg(tool_name, '.', '_');
        fprintf(file, "%stools = Tools()\n", indent);
        fprintf(file, "%stools.%s(", indent, tool_name);
        G_free(tool_name);
    }
    else {
        fprintf(file, "%sgs.%s(\"%s\"", indent, python_function, st->pgm_name);
        first_parameter_printed = true;
    }

    const struct Option *first_required_rule_option =
        G__first_required_option_from_rules();
    const struct Option *opt = NULL;
    const char *type;

    if (st->n_opts) {
        opt = &st->first_option;

        while (opt != NULL) {
            if (opt->key_desc != NULL)
                type = opt->key_desc;
            else
                switch (opt->type) {
                case TYPE_INTEGER:
                    type = "integer";
                    break;
                case TYPE_DOUBLE:
                    type = "float";
                    break;
                case TYPE_STRING:
                    type = "string";
                    break;
                default:
                    type = "string";
                    break;
                }
            if (opt->required || first_required_rule_option == opt ||
                (strcmp(opt->key, "format") == 0 && output_format_default)) {
                if (first_parameter_printed) {
                    fprintf(file, ", ");
                }
                fprintf(file, "%s=", opt->key);

                char *value = NULL;
                if (opt->answer) {
                    value = G_store(opt->answer);
                }
                else if (opt->options && opt->type == TYPE_STRING) {
                    // Get example value from allowed values, but only for
                    // strings because numbers may have ranges and we don't
                    // want to print a range.
                    // Get allowed values as tokens.
                    char **tokens;
                    char delm[2];
                    delm[0] = ',';
                    delm[1] = '\0';
                    tokens = G_tokenize(opt->options, delm);
                    // We are interested in the first allowed value.
                    if (tokens[0]) {
                        G_chop(tokens[0]);
                        value = G_store(tokens[0]);
                    }
                    G_free_tokens(tokens);
                }

                if (output_format_default && strcmp(opt->key, "format") == 0) {
                    fprintf(file, "\"%s\"", output_format_default);
                }
                else if (value) {
                    if (opt->type == TYPE_INTEGER || opt->type == TYPE_DOUBLE) {
                        fprintf(file, "%s", value);
                    }
                    else {
                        fprintf(file, "\"%s\"", value);
                    }
                }
                else {
                    if (opt->type == TYPE_INTEGER) {
                        fprintf(file, "0");
                    }
                    else if (opt->type == TYPE_DOUBLE) {
                        fprintf(file, "0.0");
                    }
                    else {
                        fprintf(file, "\"%s\"", type);
                    }
                }
                first_parameter_printed = true;
                G_free(value);
            }
            opt = opt->next_opt;
        }
    }
    fprintf(file, ")\n%s```\n", indent);
}

void G__md_print_python_short_version(FILE *file, const char *indent,
                                      bool tools_api)
{
    struct Option *opt;
    struct Flag *flag;
    int new_prompt = 0;
    bool output_format_option = false;
    const char *output_format_default = NULL;
    bool shell_eval_flag = false;
    const char *python_function = NULL;

    new_prompt = G__uses_new_gisprompt();

    if (st->n_opts) {
        opt = &st->first_option;
        while (opt != NULL) {
            if (strcmp(opt->key, "format") == 0) {
                if (opt->options) {
                    int i = 0;
                    while (opt->opts[i]) {
                        if (strcmp(opt->opts[i], "csv") == 0)
                            output_format_default = "csv";
                        if (strcmp(opt->opts[i], "json") == 0) {
                            output_format_default = "json";
                            break;
                        }
                        i++;
                    }
                }
                if (output_format_default) {
                    output_format_option = true;
                }
                break;
            }
            opt = opt->next_opt;
        }
    }
    if (st->n_flags) {
        flag = &st->first_flag;
        while (st->n_flags && flag != NULL) {
            if (flag->key == 'g') {
                shell_eval_flag = true;
                break;
            }
            flag = flag->next_flag;
        }
    }
    bool first_parameter_printed = false;
    if (tools_api) {
        char *tool_name = G_store(st->pgm_name);
        G_strchg(tool_name, '.', '_');
        fprintf(file, "%s*grass.tools.Tools.%s*(", indent, tool_name);
        G_free(tool_name);
    }
    else {
        if (output_format_option || (!new_prompt && shell_eval_flag)) {
            python_function = "parse_command";
            // We know this can be parsed, but we don't detect just plain
            // text output to use read_command because we can't distinguish
            // between plain text outputs and modifications of data.
        }
        else {
            python_function = "run_command";
        }
        fprintf(file, "%s*grass.script.%s*(\"***%s***\",", indent,
                python_function, st->pgm_name);
        fprintf(file, "\n");
        first_parameter_printed = true;
    }

    if (st->n_opts) {
        opt = &st->first_option;

        while (opt != NULL) {
            if (first_parameter_printed) {
                fprintf(file, "%s    ", indent);
            }
            if (!opt->required && !opt->answer) {
                fprintf(file, "**%s**=*None*", opt->key);
            }
            else {
                fprintf(file, "**%s**", opt->key);
            }
            if (opt->answer) {
                fprintf(file, "=");
                int tuple_items = G__option_num_tuple_items(opt);
                if (!tuple_items &&
                    (opt->type == TYPE_INTEGER || opt->type == TYPE_DOUBLE)) {
                    fprintf(file, "*");
                    G__md_print_escaped(file, opt->answer, indent);
                    fprintf(file, "*");
                }
                else {
                    fprintf(file, "*\"");
                    G__md_print_escaped(file, opt->answer, indent);
                    fprintf(file, "\"*");
                }
            }
            fprintf(file, ",\n");
            first_parameter_printed = true;
            opt = opt->next_opt;
        }
    }

    if (st->n_flags) {
        flag = &st->first_flag;
        fprintf(file, "%s    **flags**=*None*,\n", indent);
    }

    const char *flag_default = "*None*";
    if (new_prompt)
        fprintf(file, "%s    **overwrite**=%s,\n", indent, flag_default);

    fprintf(file, "%s    **verbose**=%s,\n", indent, flag_default);
    fprintf(file, "%s    **quiet**=%s,\n", indent, flag_default);
    fprintf(file, "%s    **superquiet**=%s)\n", indent, flag_default);

    print_python_example(file, python_function, output_format_default, indent,
                         tools_api);
    if (tools_api) {
        fprintf(file,
                "\n%sThis grass.tools API is experimental in version 8.5 "
                "and expected to be stable in version 8.6.\n",
                indent);
    }
}

void G__md_print_python_long_version(FILE *file, const char *indent,
                                     bool tools_api)
{
    struct Option *opt;
    struct Flag *flag;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    // Options (key-value parameters)
    if (st->n_opts) {
        opt = &st->first_option;
        while (opt != NULL) {
            print_python_option(file, opt, indent, tools_api);
            opt = opt->next_opt;
            fprintf(file, MD_NEWLINE);
            fprintf(file, "\n");
        }
    }

    // Short (one-letter) flags and tool-specific long flags
    if (st->n_flags) {
        fprintf(file, "%s**flags** : str, *optional*", indent);
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t", indent);
        fprintf(file, "Allowed values: ");
        flag = &st->first_flag;
        while (st->n_flags && flag != NULL) {
            fprintf(file, "*%s*", &flag->key);
            flag = flag->next_flag;
            if (flag != NULL)
                fprintf(file, ", ");
        }
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
        flag = &st->first_flag;
        while (st->n_flags && flag != NULL) {
            print_python_short_flag(file, &flag->key, flag->label,
                                    flag->description, indent);
            fprintf(file, MD_NEWLINE);
            fprintf(file, "\n");
            flag = flag->next_flag;
        }
    }
    if (new_prompt) {
        print_python_long_flag(
            file, "overwrite", NULL,
            _("Allow output files to overwrite existing files"), indent);
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
    }
    // Pre-defined long flags
    print_python_long_flag(file, "verbose", NULL, _("Verbose module output"),
                           indent);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    print_python_long_flag(file, "quiet", NULL, _("Quiet module output"),
                           indent);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    print_python_long_flag(file, "superquiet", NULL,
                           _("Very quiet module output"), indent);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");

    if (!tools_api)
        return;

    fprintf(file, "\n%sReturns:\n\n", indent);

    bool outputs_arrays = false;
    char age[KEYLENGTH];
    char element[KEYLENGTH];
    char prompt_description[KEYLENGTH];
    if (st->n_opts) {
        opt = &st->first_option;
        while (opt != NULL) {
            if (opt->gisprompt) {
                G__split_gisprompt(opt->gisprompt, age, element,
                                   prompt_description);
                if (tools_api && !opt->multiple && opt->type == TYPE_STRING) {
                    if (G_strncasecmp("new", age, 3) == 0 &&
                        G_strncasecmp("cell", element, 4) == 0) {
                        outputs_arrays = true;
                    }
                }
            }
            opt = opt->next_opt;
        }
    }

    fprintf(file, "%s**result** : ", indent);
    fprintf(file, "grass.tools.support.ToolResult");
    if (outputs_arrays) {
        fprintf(file, " | np.ndarray | tuple[np.ndarray]");
    }
    fprintf(file, " | None");
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n%s", indent);
    fprintf(file, "If the tool produces text as standard output, a "
                  "*ToolResult* object will be returned. "
                  "Otherwise, `None` will be returned.");
    if (outputs_arrays) {
        fprintf(file, " If an array type (e.g., *np.ndarray*) is used for one "
                      "of the raster outputs, "
                      "the result will be an array and will have the shape "
                      "corresponding to the computational region. "
                      "If an array type is used for more than one raster "
                      "output, the result will be a tuple of arrays.");
    }
    fprintf(file, "\n");

    fprintf(file, "\n%sRaises:\n\n", indent);
    fprintf(file,
            "%s*grass.tools.ToolError*: When the tool ended with an error.\n",
            indent);
}
