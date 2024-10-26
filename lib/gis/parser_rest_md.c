/*!
   \file lib/gis/parser_rest_md.c

   \brief GIS Library - Argument parsing functions (reStructuredText and
   Markdown output)

   (C) 2012-2024 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Luca Delucchi
   \author Martin Landa (Markdown added)
 */
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

#define MD_NEWLINE "  "

static void usage_rest_md(bool rest);
static void print_flag(const char *key, const char *label,
                       const char *description, bool rest);
void print_option(const struct Option *opt, bool rest, char *);
static void print_escaped(FILE *f, const char *str, bool rest);
static void print_escaped_for_rest(FILE *f, const char *str);
static void print_escaped_for_md(FILE *f, const char *str);
static void print_escaped_for_rest_options(FILE *f, const char *str);
static void print_escaped_for_md_keywords(FILE *f, const char *str);

/*!
   \brief Print module usage description in reStructuredText or in Markdown
   format.

   \param bool rest TRUE for reStructuredText otherwise Markdown
 */
void usage_rest_md(bool rest)
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
    fprintf(stdout, "\n---\n\n");

    /* main header */
    if (!rest)
        fprintf(stdout, "# %s\n\n", st->pgm_name);

    /* header - GRASS module */
    if (!rest)
        fprintf(stdout, "## ");
    fprintf(stdout, "%s\n", _("NAME"));
    if (rest)
        fprintf(stdout, "----");
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
    if (!rest)
        fprintf(stdout, "### ");
    fprintf(stdout, "%s\n", _("KEYWORDS"));
    if (rest)
        fprintf(stdout, "--------\n");
    fprintf(stdout, "\n");
    if (st->module_info.keywords) {
        if (rest) {
            G__print_keywords(stdout, NULL, FALSE);
            fprintf(stdout, "\n");
        }
        else {
            G__print_keywords(stdout, print_escaped_for_md_keywords, TRUE);
        }
    }
    fprintf(stdout, "\n");
    if (!rest)
        fprintf(stdout, "### ");
    fprintf(stdout, "%s\n", _("SYNOPSIS"));
    if (rest) {
        fprintf(stdout, "--------\n\n");
        fprintf(stdout, "| ");
    }
    else {
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "**%s**", st->pgm_name);
    if (!rest)
        fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (rest)
        fprintf(stdout, "| ");
    fprintf(stdout, "**%s --help**", st->pgm_name);
    if (!rest)
        fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (rest)
        fprintf(stdout, "| ");
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
            if (rest)
                fprintf(stdout, "\\ ");
            fprintf(stdout, "*%s*", type);
            if (opt->multiple) {
                fprintf(stdout, " [,");
                if (rest)
                    fprintf(stdout, "\\ ");
                fprintf(stdout, "*%s*,...]", type);
            }
            if (!opt->required)
                fprintf(stdout, "]");
            if (rest)
                fprintf(stdout, " ");
            else
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
        if (!rest)
            fprintf(stdout, "#### ");
        fprintf(stdout, "%s\n", _("Flags"));
        if (rest)
            fprintf(stdout, "~~~~~~\n");
        fprintf(stdout, "\n");
        while (st->n_flags && flag != NULL) {
            print_flag(&flag->key, flag->label, flag->description, rest);
            if (!rest)
                fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
            flag = flag->next_flag;
        }
        if (new_prompt) {
            print_flag("overwrite", NULL,
                       _("Allow output files to overwrite existing files"),
                       rest);
            if (!rest)
                fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
        }
    }
    print_flag("help", NULL, _("Print usage summary"), rest);
    if (!rest)
        fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_flag("verbose", NULL, _("Verbose module output"), rest);
    if (!rest)
        fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_flag("quiet", NULL, _("Quiet module output"), rest);
    if (!rest)
        fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    print_flag("ui", NULL, _("Force launching GUI dialog"), rest);
    fprintf(stdout, "\n");

    if (st->n_opts) {
        fprintf(stdout, "\n");
        opt = &st->first_option;
        if (!rest)
            fprintf(stdout, "#### ");
        fprintf(stdout, "%s\n", _("Parameters"));
        if (rest)
            fprintf(stdout, "~~~~~~~~~~~\n");
        fprintf(stdout, "\n");
        char image_spec_rest[GPATH_MAX];
        image_spec_rest[0] = '\0';
        while (opt != NULL) {
            print_option(opt, rest, image_spec_rest);
            opt = opt->next_opt;
            if (opt != NULL) {
                if (!rest)
                    fprintf(stdout, MD_NEWLINE);
            }
            fprintf(stdout, "\n");
        }
        if (strlen(image_spec_rest) > 0) {
            fprintf(stdout, "\n");
            fprintf(stdout, "%s", image_spec_rest);
        }
    }
}

void print_flag(const char *key, const char *label, const char *description,
                bool rest)
{
    if (rest)
        fprintf(stdout, "| ");
    fprintf(stdout, "**");
    if (strlen(key) > 1)
        fprintf(stdout, "-");
    fprintf(stdout, "-%s**", key);
    if (!rest)
        fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (label != NULL) {
        if (rest)
            fprintf(stdout, "| ");
        print_escaped(stdout, "\t", rest);
        print_escaped(stdout, label, rest);
        if (!rest)
            fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
    }
    if (description != NULL) {
        if (rest)
            fprintf(stdout, "| ");
        print_escaped(stdout, "\t", rest);
        print_escaped(stdout, description, rest);
    }
}

void print_option(const struct Option *opt, bool rest, char *image_spec_rest)
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

    if (rest)
        fprintf(stdout, "| ");
    fprintf(stdout, "**%s**=", opt->key);
    if (rest)
        fprintf(stdout, "\\ ");
    fprintf(stdout, "*%s*", type);
    if (opt->multiple) {
        fprintf(stdout, " [,");
        if (rest)
            fprintf(stdout, "\\ ");
        fprintf(stdout, "*%s*,...]", type);
    }
    /* fprintf(stdout, "*"); */
    if (opt->required) {
        fprintf(stdout, " **[required]**");
    }
    if (!rest)
        fprintf(stdout, MD_NEWLINE);
    fprintf(stdout, "\n");
    if (opt->label) {
        if (rest)
            fprintf(stdout, "| ");
        print_escaped(stdout, "\t", rest);
        print_escaped(stdout, opt->label, rest);
    }
    if (opt->description) {
        if (opt->label) {
            if (!rest)
                fprintf(stdout, MD_NEWLINE);
            fprintf(stdout, "\n");
        }
        if (rest)
            fprintf(stdout, "| ");
        print_escaped(stdout, "\t", rest);
        print_escaped(stdout, opt->description, rest);
    }

    if (opt->options) {
        if (!rest)
            fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
        if (rest)
            fprintf(stdout, "| ");
        print_escaped(stdout, "\t", rest);
        fprintf(stdout, "%s: *", _("Options"));
        print_escaped_for_rest_options(stdout, opt->options);
        fprintf(stdout, "*");
    }

    if (opt->def) {
        if (!rest)
            fprintf(stdout, MD_NEWLINE);
        fprintf(stdout, "\n");
        if (rest)
            fprintf(stdout, "| ");
        print_escaped(stdout, "\t", rest);
        fprintf(stdout, "%s:", _("Default"));
        /* TODO check if value is empty
           if (!opt->def.empty()){ */
        fprintf(stdout, " *");
        print_escaped(stdout, opt->def, rest);
        fprintf(stdout, "*");
    }

    if (opt->descs) {
        int i = 0;

        while (opt->opts[i]) {
            if (opt->descs[i]) {
                if (!rest)
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
                        if (rest) {
                            char *image_spec;
                            G_asprintf(&image_spec,
                                       ".. |%s| image:: %s/%s.png\n",
                                       opt->opts[i], thumbnails, opt->opts[i]);
                            strcat(image_spec_rest, image_spec);
                        }
                        else {
                            print_escaped(stdout, "\t\t", rest);
                            fprintf(stdout, "![%s](%s/%s.png) ", opt->opts[i],
                                    thumbnails, opt->opts[i]);
                        }
                    }
                    else {
                        if (rest)
                            fprintf(stdout, "| ");
                        print_escaped(stdout, "\t\t", rest);
                        if (rest)
                            fprintf(stdout, "\\ ");
                    }
                }

                if (rest && thumbnails) {
                    fprintf(stdout, "| ");
                    print_escaped(stdout, "\t\t", rest);
                    fprintf(stdout, "|%s| ", opt->opts[i]);
                }
                if (!rest)
                    print_escaped(stdout, "\t", rest);
                fprintf(stdout, "**");
                print_escaped(stdout, opt->opts[i], rest);
                fprintf(stdout, "**: ");
                print_escaped(stdout, opt->descs[i], rest);
            }
            i++;
        }
    }
}

/*!
 * \brief Format text for reStructuredText output
 */
#define do_escape(c, escaped) \
    case c:                   \
        fputs(escaped, f);    \
        break

void print_escaped(FILE *f, const char *str, bool rest)
{
    if (rest)
        print_escaped_for_rest(f, str);
    else
        print_escaped_for_md(f, str);
}

void print_escaped_for_rest(FILE *f, const char *str)
{
    const char *s;

    for (s = str; *s; s++) {
        switch (*s) {
            do_escape('\n', "\n\n");
            do_escape('\t', "    ");
        default:
            fputc(*s, f);
        }
    }
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

void print_escaped_for_rest_options(FILE *f, const char *str)
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

    /* HTML link only for second keyword */
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
        fprintf(f, ".html)");
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
            fprintf(f, ".html)");
        }
        else {
            /* keyword index */
            char *str_link;
            str_link = G_str_replace(str_s, " ", "%20");
            fprintf(f, "[%s](keywords.html#%s)", str_s, str_link);
            G_free(str_link);
        }
    }

    G_free(str_s);
}

#undef do_escape

/*!
  \brief Print module usage description in reStructuredText format.
*/
void G__usage_rest(void)
{
    usage_rest_md(TRUE);
}

/*!
  \brief Print module usage description in Markdown format.
*/
void G__usage_markdown(void)
{
    usage_rest_md(FALSE);
}
