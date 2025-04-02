/*!
   \file lib/gis/parser_md.c

   \brief GIS Library - Argument parsing functions (Markdown output)

   (C) 2012-2025 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa
   \author Vaclav Petras
 */
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

#define MD_NEWLINE "  "

static void print_cli_flag(const char *key, const char *label,
                           const char *description, const char *indent);
static void print_python_short_flag(const char *key, const char *label,
                                    const char *description,
                                    const char *indent);
static void print_python_long_flag(const char *key, const char *label,
                                   const char *description, const char *indent);
static void print_cli_option(const struct Option *opt, const char *indent);
static void print_python_option(const struct Option *opt, const char *indent);
static void print_escaped(FILE *f, const char *str);
static void print_escaped_for_md(FILE *f, const char *str);
static void print_escaped_for_md_options(FILE *f, const char *str);
static void print_cli_short_version(FILE *file, const char *indent);
static void print_python_short_version(FILE *file, const char *indent);
static void print_cli_long_version(const char *indent);
static void print_python_long_version(const char *indent);

/*!
   \brief Print module usage description in Markdown format.
 */
void G__usage_markdown(void)
{
    if (!st->pgm_name)
        st->pgm_name = G_program_name();
    if (!st->pgm_name)
        st->pgm_name = "??";

    /* print metadata used by man/build*.py */
    fprintf(stdout, "---\n");
    fprintf(stdout, "name: %s\n", st->pgm_name);
    fprintf(stdout, "description: %s\n", st->module_info.description);
    fprintf(stdout, "keywords: [ ");
    G__print_keywords(stdout, NULL, FALSE);
    fprintf(stdout, " ]");
    fprintf(stdout, "\n---\n\n");

    /* main header */
    fprintf(stdout, "# %s\n\n", st->pgm_name);

    /* header */
    if (st->module_info.label)
        fprintf(stdout, "%s\n", st->module_info.label);

    if (st->module_info.description) {
        if (st->module_info.label)
            fprintf(stdout, "\n");
        fprintf(stdout, "%s\n", st->module_info.description);
    }

    const char *tab_indent = "    ";

    /* short version */
    fprintf(stdout, "\n=== \"Command line (Bash)\"\n\n");
    print_cli_short_version(stdout, tab_indent);
    fprintf(stdout, "\n=== \"Python (grass.script)\"\n\n");
    print_python_short_version(stdout, tab_indent);

    fprintf(stdout, "\n## %s\n", _("Parameters"));

    /* long version */
    fprintf(stdout, "\n=== \"Command line (Bash)\"\n\n");
    print_cli_long_version(tab_indent);
    fprintf(stdout, "\n=== \"Python (grass.script)\"\n\n");
    print_python_long_version(tab_indent);
}

void print_cli_flag(const char *key, const char *label, const char *description,
                    const char *indent)
{
    fprintf(stdout, "%s**", indent);
    if (strlen(key) > 1)
        fprintf(stdout, "-");
    fprintf(stdout, "-%s**", key);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (label != NULL) {
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        print_escaped(stdout, label);
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
    }
    if (description != NULL) {
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        print_escaped(stdout, description);
    }
}

void print_python_short_flag(const char *key, const char *label,
                             const char *description, const char *indent)
{
    fprintf(stdout, "%s", indent);
    print_escaped(stdout, "\t");
    fprintf(stdout, "**%s**", key);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (label != NULL) {
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t\t");
        print_escaped(stdout, label);
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
    }
    if (description != NULL) {
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t\t");
        print_escaped(stdout, description);
    }
}

void print_python_long_flag(const char *key, const char *label,
                            const char *description, const char *indent)
{
    fprintf(stdout, "%s**%s**: bool, default False", indent, key);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (label != NULL) {
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        print_escaped(stdout, label);
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
    }
    if (description != NULL) {
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        print_escaped(stdout, description);
    }
}

void print_cli_option(const struct Option *opt, const char *indent)
{
    const char *type;

    /* TODO: make this a enumeration type? */
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
    fprintf(stdout, "%s**%s**=", indent, opt->key);
    fprintf(stdout, "*%s*", type);
    if (opt->multiple) {
        fprintf(stdout, " [,");
        fprintf(stdout, "*%s*,...]", type);
    }
    /* fprintf(stdout, "*"); */
    if (opt->required) {
        fprintf(stdout, " **[required]**");
    }
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (opt->label) {
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        print_escaped(stdout, opt->label);
    }
    if (opt->description) {
        if (opt->label) {
            fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
        }
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        print_escaped(stdout, opt->description);
    }

    if (opt->options) {
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        fprintf(stdout, "%s: *", _("Options"));
        print_escaped_for_md_options(stdout, opt->options);
        fprintf(stdout, "*");
    }

    if (opt->def) {
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        fprintf(stdout, "%s:", _("Default"));
        /* TODO check if value is empty
           if (!opt->def.empty()){ */
        fprintf(stdout, " *");
        print_escaped(stdout, opt->def);
        fprintf(stdout, "*");
    }

    if (opt->descs) {
        int i = 0;

        while (opt->opts[i]) {
            if (opt->descs[i]) {
                fprintf(stdout, MD_NEWLINE);
                fprintf(stdout, "\n");
                fprintf(stdout, "%s", indent);
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
                        print_escaped(stdout, "\t\t");
                        fprintf(stdout, "![%s](%s/%s.png) ", opt->opts[i],
                                thumbnails, opt->opts[i]);
                    }
                    else {
                        print_escaped(stdout, "\t\t");
                    }
                }
                print_escaped(stdout, "\t");
                fprintf(stdout, "**");
                print_escaped(stdout, opt->opts[i]);
                fprintf(stdout, "**: ");
                print_escaped(stdout, opt->descs[i]);
            }
            i++;
        }
    }
}

void print_python_option(const struct Option *opt, const char *indent)
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
    fprintf(stdout, "%s**%s** : ", indent, opt->key);
    if (opt->multiple) {
        fprintf(stdout, "Iterable, list[%s]", type);
    }
    else {
        fprintf(stdout, "%s", type);
    }
    if (opt->key_desc) {
        fprintf(stdout, ", `%s`", opt->key_desc);
    }
    /* fprintf(stdout, "*"); */
    if (opt->required) {
        fprintf(stdout, ", required");
    }
    else {
        fprintf(stdout, ", optional");
    }

    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (opt->label) {
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        print_escaped(stdout, opt->label);
    }
    if (opt->description) {
        if (opt->label) {
            fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
        }
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        print_escaped(stdout, opt->description);
    }

    if (opt->options) {
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        fprintf(stdout, "%s: *", _("Allowed values"));
        print_escaped_for_md_options(stdout, opt->options);
        fprintf(stdout, "*");
    }

    if (opt->def) {
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        fprintf(stdout, "%s:", _("Default"));
        /* TODO check if value is empty
           if (!opt->def.empty()){ */
        fprintf(stdout, " *");
        print_escaped(stdout, opt->def);
        fprintf(stdout, "*");
    }

    if (opt->descs) {
        int i = 0;

        while (opt->opts[i]) {
            if (opt->descs[i]) {
                fprintf(stdout, MD_NEWLINE);
                fprintf(stdout, "\n");
                fprintf(stdout, "%s", indent);
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
                        print_escaped(stdout, "\t\t");
                        fprintf(stdout, "![%s](%s/%s.png) ", opt->opts[i],
                                thumbnails, opt->opts[i]);
                    }
                    else {
                        print_escaped(stdout, "\t\t");
                    }
                }
                print_escaped(stdout, "\t");
                fprintf(stdout, "**");
                print_escaped(stdout, opt->opts[i]);
                fprintf(stdout, "**: ");
                print_escaped(stdout, opt->descs[i]);
            }
            i++;
        }
    }
}

/*!
 * \brief Format text for Markdown output
 */
#define do_escape(c, escaped) \
    case c:                   \
        fputs(escaped, f);    \
        break

void print_escaped(FILE *f, const char *str)
{
    print_escaped_for_md(f, str);
}

void print_escaped_for_md(FILE *f, const char *str)
{
    const char *s;

    for (s = str; *s; s++) {
        switch (*s) {
            do_escape('\n', "\\\n");
            do_escape('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
            do_escape('<', "&lt;");
            do_escape('>', "&gt;");
            do_escape('*', "\\*");
        default:
            fputc(*s, f);
        }
    }
}

void print_escaped_for_md_options(FILE *f, const char *str)
{
    const char *s;

    for (s = str; *s; s++) {
        switch (*s) {
            do_escape('\n', "\n\n");
            do_escape(',', ", ");
        default:
            fputc(*s, f);
        }
    }
}

void print_cli_short_version(FILE *file, const char *indent)
{
    struct Option *opt;
    struct Flag *flag;
    const char *type;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    fprintf(file, "%s**%s**", indent, st->pgm_name);
    fprintf(file, "\n");

    /* print short version first */
    if (st->n_flags) {
        flag = &st->first_flag;
        fprintf(file, "%s[**-", indent);
        while (flag != NULL) {
            fprintf(file, "%c", flag->key);
            flag = flag->next_flag;
        }
        fprintf(file, "**]");
    }
    fprintf(file, "\n");

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
            fprintf(file, "%s", indent);
            if (!opt->required)
                fprintf(file, "[");
            fprintf(file, "**%s**=", opt->key);
            fprintf(file, "*%s*", type);
            if (opt->multiple) {
                fprintf(file, " [,");
                fprintf(file, "*%s*,...]", type);
            }
            if (!opt->required)
                fprintf(file, "]");
            fprintf(file, "\n");

            opt = opt->next_opt;
        }
    }
    if (new_prompt)
        fprintf(file, "%s[**--overwrite**]\n", indent);

    fprintf(file, "%s[**--verbose**]\n", indent);
    fprintf(file, "%s[**--quiet**]\n", indent);
    fprintf(file, "%s[**--ui**]\n", indent);
}

void print_python_short_version(FILE *file, const char *indent)
{
    struct Option *opt;
    struct Flag *flag;
    const char *type;
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
        // We know this is can be parsed, but we can't detect just plain stdout
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
                if (opt->type == TYPE_INTEGER || opt->type == TYPE_DOUBLE) {
                    fprintf(file, "*");
                    print_escaped(file, opt->answer);
                    fprintf(file, "*");
                }
                else {
                    fprintf(file, "*\"");
                    print_escaped(file, opt->answer);
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

    fprintf(file, "\n%sExample:\n", indent);

    fprintf(file, "\n%s```python\n", indent);
    fprintf(file, "%sgs.%s(\"%s\"", indent, python_function, st->pgm_name);

    const struct Option *first_required_rule_option =
        G__first_required_option_from_rules();

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

void print_cli_long_version(const char *indent)
{
    struct Option *opt;
    struct Flag *flag;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    // Options (key-value parameters)
    if (st->n_opts) {
        opt = &st->first_option;
        while (opt != NULL) {
            print_cli_option(opt, indent);
            opt = opt->next_opt;
            if (opt != NULL) {
                fprintf(stdout, MD_NEWLINE);
            }
            fprintf(stdout, "\n");
        }
    }

    // Short (one-letter) flags and tool-specific long flags
    if (st->n_flags || new_prompt) {
        flag = &st->first_flag;
        while (st->n_flags && flag != NULL) {
            print_cli_flag(&flag->key, flag->label, flag->description, indent);
            fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
            flag = flag->next_flag;
        }
        if (new_prompt) {
            print_cli_flag("overwrite", NULL,
                           _("Allow output files to overwrite existing files"),
                           indent);
            fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
        }
    }
    // Pre-defined long flags
    print_cli_flag("help", NULL, _("Print usage summary"), indent);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_cli_flag("verbose", NULL, _("Verbose module output"), indent);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_cli_flag("quiet", NULL, _("Quiet module output"), indent);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_cli_flag("ui", NULL, _("Force launching GUI dialog"), indent);
    fprintf(stdout, "\n");
}

void print_python_long_version(const char *indent)
{
    struct Option *opt;
    struct Flag *flag;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    // Options (key-value parameters)
    if (st->n_opts) {
        opt = &st->first_option;
        while (opt != NULL) {
            print_python_option(opt, indent);
            opt = opt->next_opt;
            fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
        }
    }

    // Short (one-letter) flags and tool-specific long flags
    if (st->n_flags) {
        fprintf(stdout, "%s**flags** : str", indent);
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
        fprintf(stdout, "%s", indent);
        print_escaped(stdout, "\t");
        fprintf(stdout, "Allowed values: ");
        flag = &st->first_flag;
        while (st->n_flags && flag != NULL) {
            fprintf(stdout, "*%s*, ", &flag->key);
            flag = flag->next_flag;
        }
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
        flag = &st->first_flag;
        while (st->n_flags && flag != NULL) {
            print_python_short_flag(&flag->key, flag->label, flag->description,
                                    indent);
            fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
            flag = flag->next_flag;
        }
    }
    if (new_prompt) {
        print_python_long_flag(
            "overwrite", NULL,
            _("Allow output files to overwrite existing files"), indent);
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
    }
    // Pre-defined long flags
    print_python_long_flag("verbose", NULL, _("Verbose module output"), indent);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_python_long_flag("quiet", NULL, _("Quiet module output"), indent);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_python_long_flag("superquiet", NULL, _("Very quiet module output"),
                           indent);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
}

#undef do_escape
