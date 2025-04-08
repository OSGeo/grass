/*!
   \file lib/gis/parser_md_python.c

   \brief GIS Library - Argument parsing functions (Markdown output - Python)

   (C) 2025 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Vaclav Petras
 */
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
                                const char *indent);
static void print_python_example(FILE *file, const char *python_function,
                                 const char *output_format_default,
                                 const char *indent);
static void print_python_tuple(FILE *file, const char *type, int num_items);

void print_python_short_flag(FILE *file, const char *key, const char *label,
                             const char *description, const char *indent)
{
    fprintf(file, "%s", indent);
    G__md_print_escaped(file, "\t");
    fprintf(file, "**%s**", key);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    if (label != NULL) {
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t\t");
        G__md_print_escaped(file, label);
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
    }
    if (description != NULL) {
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t\t");
        G__md_print_escaped(file, description);
    }
}

void print_python_long_flag(FILE *file, const char *key, const char *label,
                            const char *description, const char *indent)
{
    fprintf(file, "%s**%s**: bool, *optional*", indent, key);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    if (label != NULL) {
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t");
        G__md_print_escaped(file, label);
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
    }
    if (description != NULL) {
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t");
        G__md_print_escaped(file, description);
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
    }
    fprintf(file, "%s", indent);
    G__md_print_escaped(file, "\t");
    fprintf(file, "Default: *False*");
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
                         const char *indent)
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
        G__md_print_escaped(file, "\t");
        G__md_print_escaped(file, opt->label);
    }
    if (opt->description) {
        if (opt->label) {
            fprintf(file, MD_NEWLINE);
            fprintf(file, "\n");
        }
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t");
        G__md_print_escaped(file, opt->description);
    }
    if (opt->gisprompt || opt->key_desc) {
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t");
        fprintf(file, "%s: ", _("Used as"));
    }
    if (opt->gisprompt) {
        char age[KEYLENGTH];
        char element[KEYLENGTH];
        char desc[KEYLENGTH];
        G__split_gisprompt(opt->gisprompt, age, element, desc);
        if (strcmp(age, "new") == 0)
            fprintf(file, "output, ");
        else if (strcmp(age, "old") == 0)
            fprintf(file, "input, ");
        // While element more strictly expresses how the value will be
        // used given that the parser may read that information, desc
        // is meant as a user-facing representation of the same
        // information.
        fprintf(file, "%s", desc);
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
        G__md_print_escaped(file, "\t");
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
                        G__md_print_escaped(file, "\t\t");
                        fprintf(file, "![%s](%s/%s.png) ", opt->opts[i],
                                thumbnails, opt->opts[i]);
                    }
                    else {
                        G__md_print_escaped(file, "\t\t");
                    }
                }
                G__md_print_escaped(file, "\t");
                fprintf(file, "**");
                G__md_print_escaped(file, opt->opts[i]);
                fprintf(file, "**: ");
                G__md_print_escaped(file, opt->descs[i]);
            }
            i++;
        }
    }

    if (opt->def) {
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t");
        fprintf(file, "%s:", _("Default"));
        fprintf(file, " *");
        G__md_print_escaped(file, opt->def);
        fprintf(file, "*");
    }
}

void print_python_example(FILE *file, const char *python_function,
                          const char *output_format_default, const char *indent)
{
    fprintf(file, "\n%sExample:\n", indent);

    fprintf(file, "\n%s```python\n", indent);
    fprintf(file, "%sgs.%s(\"%s\"", indent, python_function, st->pgm_name);

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
            if (opt->required || first_required_rule_option == opt) {
                fprintf(file, ", %s=", opt->key);
                if (output_format_default && strcmp(opt->key, "format") == 0) {
                    fprintf(file, "\"%s\"", output_format_default);
                }
                else if (opt->answer) {
                    if (opt->type == TYPE_INTEGER || opt->type == TYPE_DOUBLE) {
                        fprintf(file, "%s", opt->answer);
                    }
                    else {
                        fprintf(file, "\"%s\"", opt->answer);
                    }
                }
                else {
                    if (opt->type == TYPE_INTEGER || opt->type == TYPE_DOUBLE) {
                        fprintf(file, "%s", type);
                    }
                    else {
                        fprintf(file, "\"%s\"", type);
                    }
                }
            }
            opt = opt->next_opt;
        }
    }
    fprintf(file, ")\n%s```\n", indent);
}

void G__md_print_python_short_version(FILE *file, const char *indent)
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
    if (output_format_option || (!new_prompt && shell_eval_flag)) {
        python_function = "parse_command";
        // We know this is can be parsed, but we can't detect just plain file
        // because we can't distinguish between plain text outputs and
        // modifications of data.
    }
    else {
        python_function = "run_command";
    }
    fprintf(file, "%s*grass.script.%s*(\"***%s***\",", indent, python_function,
            st->pgm_name);
    fprintf(file, "\n");

    if (st->n_opts) {
        opt = &st->first_option;

        while (opt != NULL) {
            fprintf(file, "%s    ", indent);
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
                    G__md_print_escaped(file, opt->answer);
                    fprintf(file, "*");
                }
                else {
                    fprintf(file, "*\"");
                    G__md_print_escaped(file, opt->answer);
                    fprintf(file, "\"*");
                }
            }
            fprintf(file, ",\n");

            opt = opt->next_opt;
        }
    }

    if (st->n_flags) {
        flag = &st->first_flag;
        fprintf(file, "%s    **flags**=*None*,\n", indent);
    }

    if (new_prompt)
        fprintf(file, "%s    **overwrite**=*False*,\n", indent);

    fprintf(file, "%s    **verbose**=*False*,\n", indent);
    fprintf(file, "%s    **quiet**=*False*,\n", indent);
    fprintf(file, "%s    **superquiet**=*False*)\n", indent);

    print_python_example(file, python_function, output_format_default, indent);
}

void G__md_print_python_long_version(FILE *file, const char *indent)
{
    struct Option *opt;
    struct Flag *flag;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    // Options (key-value parameters)
    if (st->n_opts) {
        opt = &st->first_option;
        while (opt != NULL) {
            print_python_option(file, opt, indent);
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
        G__md_print_escaped(file, "\t");
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
}
