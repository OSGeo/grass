/*!
   \file lib/gis/parser_md_cli.c

   \brief GIS Library - Argument parsing functions (Markdown output - CLI)

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

static void print_cli_flag(FILE *file, const char *key, const char *label,
                           const char *description, const char *indent);
static void print_cli_option(FILE *file, const struct Option *opt,
                             const char *indent);
static void print_cli_example(FILE *file, const char *indent);

void print_cli_flag(FILE *file, const char *key, const char *label,
                    const char *description, const char *indent)
{
    fprintf(file, "%s**", indent);
    if (strlen(key) > 1)
        fprintf(file, "-");
    fprintf(file, "-%s**", key);
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
    }
}

void print_cli_option(FILE *file, const struct Option *opt, const char *indent)
{
    const char *type;

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
    fprintf(file, "%s**%s**=", indent, opt->key);
    fprintf(file, "*%s*", type);
    if (opt->multiple) {
        fprintf(file, " [,");
        fprintf(file, "*%s*,...]", type);
    }
    /* fprintf(file, "*"); */
    if (opt->required) {
        fprintf(file, " **[required]**");
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

    if (opt->options) {
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t");
        fprintf(file, "%s: *", _("Allowed values"));
        G__md_print_escaped_for_options(file, opt->options);
        fprintf(file, "*");
    }

    if (opt->def) {
        fprintf(file, MD_NEWLINE);
        fprintf(file, "\n");
        fprintf(file, "%s", indent);
        G__md_print_escaped(file, "\t");
        fprintf(file, "%s:", _("Default"));
        /* TODO check if value is empty
           if (!opt->def.empty()){ */
        fprintf(file, " *");
        G__md_print_escaped(file, opt->def);
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
}

void print_cli_example(FILE *file, const char *indent)
{
    fprintf(file, "\n%sExample:\n", indent);

    fprintf(file, "\n%s```sh\n", indent);
    fprintf(file, "%s%s", indent, st->pgm_name);

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
                fprintf(file, " ");
                fprintf(file, "%s=", opt->key);
                if (opt->answer) {
                    fprintf(file, "%s", opt->answer);
                }
                else {
                    fprintf(file, "%s", type);
                }
            }
            opt = opt->next_opt;
        }
    }
    fprintf(file, "\n%s```\n", indent);
}

void G__md_print_cli_short_version(FILE *file, const char *indent)
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
        fprintf(file, "\n");
    }

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
    fprintf(file, "%s[**--qq**]\n", indent);
    fprintf(file, "%s[**--ui**]\n", indent);

    print_cli_example(file, indent);
}

void G__md_print_cli_long_version(FILE *file, const char *indent)
{
    struct Option *opt;
    struct Flag *flag;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    // Options (key-value parameters)
    if (st->n_opts) {
        opt = &st->first_option;
        while (opt != NULL) {
            print_cli_option(file, opt, indent);
            opt = opt->next_opt;
            fprintf(file, MD_NEWLINE);
            fprintf(file, "\n");
        }
    }

    // Short (one-letter) flags and tool-specific long flags
    if (st->n_flags || new_prompt) {
        flag = &st->first_flag;
        while (st->n_flags && flag != NULL) {
            print_cli_flag(file, &flag->key, flag->label, flag->description,
                           indent);
            fprintf(file, MD_NEWLINE);
            fprintf(file, "\n");
            flag = flag->next_flag;
        }
        if (new_prompt) {
            print_cli_flag(file, "overwrite", NULL,
                           _("Allow output files to overwrite existing files"),
                           indent);
            fprintf(file, MD_NEWLINE);
            fprintf(file, "\n");
        }
    }
    // Pre-defined long flags
    print_cli_flag(file, "help", NULL, _("Print usage summary"), indent);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    print_cli_flag(file, "verbose", NULL, _("Verbose module output"), indent);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    print_cli_flag(file, "quiet", NULL, _("Quiet module output"), indent);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    print_cli_flag(file, "qq", NULL, _("Very quiet module output"), indent);
    fprintf(file, MD_NEWLINE);
    fprintf(file, "\n");
    print_cli_flag(file, "ui", NULL, _("Force launching GUI dialog"), indent);
    fprintf(file, "\n");
}
