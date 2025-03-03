/*!
   \file lib/gis/parser_md.c

   \brief GIS Library - Argument parsing functions (Markdown output)

   (C) 2012-2025 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa
 */
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

#define MD_NEWLINE "  "

static void print_flag(const char *key, const char *label,
                       const char *description);
void print_option(const struct Option *opt);
static void print_escaped(FILE *f, const char *str);
static void print_escaped_for_md(FILE *f, const char *str);
static void print_escaped_for_md_keywords(FILE *f, const char *str);
static void print_escaped_for_md_options(FILE *f, const char *str);

/*!
   \brief Print module usage description in Markdown format.
 */
void G__usage_markdown(void)
{
    struct Option *opt;
    struct Flag *flag;
    const char *type;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    if (!st->pgm_name)
        st->pgm_name = G_program_name();
    if (!st->pgm_name)
        st->pgm_name = "??";

    /* print metadata used by man/build*.py */
    fprintf(stdout, "---\n");
    fprintf(stdout, "name: %s\n", st->pgm_name);
    fprintf(stdout, "description: %s\n", st->module_info.description);
    fprintf(stdout, "keywords: ");
    G__print_keywords(stdout, NULL, FALSE);
    fprintf(stdout, "\n");
    fprintf(stdout, "tags: [ ");
    G__print_keywords(stdout, NULL, FALSE);
    fprintf(stdout, " ]");
    fprintf(stdout, "\n---\n\n");

    /* main header */
    fprintf(stdout, "# %s\n\n", st->pgm_name);

    /* header - GRASS module */
    fprintf(stdout, "## ");
    fprintf(stdout, "%s\n", _("NAME"));
    fprintf(stdout, "\n");
    fprintf(stdout, "***%s***", st->pgm_name);

    if (st->module_info.label || st->module_info.description)
        fprintf(stdout, " - ");

    if (st->module_info.label)
        fprintf(stdout, "%s\n", st->module_info.label);

    if (st->module_info.description) {
        if (st->module_info.label)
            fprintf(stdout, "\n");
        fprintf(stdout, "%s\n", st->module_info.description);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "### ");
    fprintf(stdout, "%s\n", _("KEYWORDS"));
    fprintf(stdout, "\n");
    if (st->module_info.keywords) {
        G__print_keywords(stdout, print_escaped_for_md_keywords, TRUE);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "### ");
    fprintf(stdout, "%s\n", _("SYNOPSIS"));
    fprintf(stdout, "\n");
    fprintf(stdout, "**%s**", st->pgm_name);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    fprintf(stdout, "**%s --help**", st->pgm_name);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    fprintf(stdout, "**%s**", st->pgm_name);

    /* print short version first */
    if (st->n_flags) {
        flag = &st->first_flag;
        fprintf(stdout, " [**-");
        while (flag != NULL) {
            fprintf(stdout, "%c", flag->key);
            flag = flag->next_flag;
        }
        fprintf(stdout, "**] ");
    }
    else
        fprintf(stdout, " ");

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
            fprintf(stdout, " ");
            if (!opt->required)
                fprintf(stdout, "[");
            fprintf(stdout, "**%s**=", opt->key);
            fprintf(stdout, "*%s*", type);
            if (opt->multiple) {
                fprintf(stdout, " [,");
                fprintf(stdout, "*%s*,...]", type);
            }
            if (!opt->required)
                fprintf(stdout, "]");
            fprintf(stdout, "\n");

            opt = opt->next_opt;
        }
    }
    if (new_prompt)
        fprintf(stdout, " [**--overwrite**] ");

    fprintf(stdout, " [**--verbose**] ");
    fprintf(stdout, " [**--quiet**] ");
    fprintf(stdout, " [**--ui**]\n");

    /* now long version */
    fprintf(stdout, "\n");
    if (st->n_flags || new_prompt) {
        flag = &st->first_flag;
        fprintf(stdout, "#### ");
        fprintf(stdout, "%s\n", _("Flags"));
        fprintf(stdout, "\n");
        while (st->n_flags && flag != NULL) {
            print_flag(&flag->key, flag->label, flag->description);
            fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
            flag = flag->next_flag;
        }
        if (new_prompt) {
            print_flag("overwrite", NULL,
                       _("Allow output files to overwrite existing files"));
            fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
        }
    }
    print_flag("help", NULL, _("Print usage summary"));
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_flag("verbose", NULL, _("Verbose module output"));
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_flag("quiet", NULL, _("Quiet module output"));
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_flag("ui", NULL, _("Force launching GUI dialog"));
    fprintf(stdout, "\n");

    if (st->n_opts) {
        fprintf(stdout, "\n");
        opt = &st->first_option;
        fprintf(stdout, "#### ");
        fprintf(stdout, "%s\n", _("Parameters"));
        fprintf(stdout, "\n");
        while (opt != NULL) {
            print_option(opt);
            opt = opt->next_opt;
            if (opt != NULL) {
                fprintf(stdout, MD_NEWLINE);
            }
            fprintf(stdout, "\n");
        }
    }
}

void print_flag(const char *key, const char *label, const char *description)
{
    fprintf(stdout, "**");
    if (strlen(key) > 1)
        fprintf(stdout, "-");
    fprintf(stdout, "-%s**", key);
    fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (label != NULL) {
        print_escaped(stdout, "\t");
        print_escaped(stdout, label);
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
    }
    if (description != NULL) {
        print_escaped(stdout, "\t");
        print_escaped(stdout, description);
    }
}

void print_option(const struct Option *opt)
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
    fprintf(stdout, "**%s**=", opt->key);
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
        print_escaped(stdout, "\t");
        print_escaped(stdout, opt->label);
    }
    if (opt->description) {
        if (opt->label) {
            fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
        }
        print_escaped(stdout, "\t");
        print_escaped(stdout, opt->description);
    }

    if (opt->options) {
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
        print_escaped(stdout, "\t");
        fprintf(stdout, "%s: *", _("Options"));
        print_escaped_for_md_options(stdout, opt->options);
        fprintf(stdout, "*");
    }

    if (opt->def) {
        fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
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

/* generate HTML links */
void print_escaped_for_md_keywords(FILE *f, const char *str)
{
    /* removes all leading and trailing white space from keyword
       string, as spotted in Japanese and other locales. */
    char *str_s;
    str_s = G_store(str);
    G_strip(str_s);

    /* HTML link only for second keyword = topic */
    if (st->n_keys > 1 && strcmp(st->module_info.keywords[1], str) == 0) {

        const char *s;

        /* TODO: fprintf(f, _("topic: ")); */
        fprintf(f, "[%s](topic_", str_s);
        for (s = str_s; *s; s++) {
            switch (*s) {
                do_escape(' ', "_");
            default:
                fputc(*s, f);
            }
        }
        fprintf(f, ".md)");
    }
    else { /* first and other than second keyword */
        if (st->n_keys > 0 && strcmp(st->module_info.keywords[0], str) == 0) {
            /* command family */
            const char *s;

            fprintf(f, "[%s](", str_s);
            for (s = str_s; *s; s++) {
                switch (*s) {
                    do_escape(' ', "_");
                default:
                    fputc(*s, f);
                }
            }
            fprintf(f, ".md)");
        }
        else {
            /* keyword index, mkdocs expects dash */
            char *str_link;
            str_link = G_str_replace(str_s, " ", "-");
            G_str_to_lower(str_link);
            fprintf(f, "[%s](keywords.md#%s)", str_s, str_link);
            G_free(str_link);
        }
    }

    G_free(str_s);
}

#undef do_escape
