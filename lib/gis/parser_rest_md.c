/*!
   \file lib/gis/parser_rest.c

   \brief GIS Library - Argument parsing functions (reStructuredText output)

   (C) 2012-2023 by the GRASS Development Team

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

static void usage_rest_md(bool rest);
static void print_flag(const char *key, const char *label, const char *description, bool rest);
void print_option(const struct Option *opt, bool rest);
static void print_escaped_for_rest(FILE *f, const char *str);
static void print_escaped_for_rest_options(FILE *f, const char *str);

/*!
   \brief Print module usage description in reStructuredText format.
 */
void usage_rest_md(bool rest)
{
    struct Option *opt;
    struct Flag *flag;
    const char *type;
    int new_prompt = 0;
    unsigned int s;

    new_prompt = G__uses_new_gisprompt();

    if (!st->pgm_name)
        st->pgm_name = G_program_name();
    if (!st->pgm_name)
        st->pgm_name = "??";

    /* main header */
    if (rest) {
        fprintf(stdout, "=================");
        for (s = 0; s <= strlen(st->pgm_name); s++) {
            fprintf(stdout, "=");
        }
        fprintf(stdout, "\n");
    }
    if (!rest)
        fprintf(stdout, "# ");
    fprintf(stdout, "%s - GRASS GIS manual\n", st->pgm_name);
    if (rest) {
        fprintf(stdout, "=================");
        for (s = 0; s <= strlen(st->pgm_name); s++) {
            fprintf(stdout, "=");
        }
    }
    fprintf(stdout, "\n\n");
    
    /* GRASS GIS logo */
    if (rest) {
        fprintf(stdout, ".. figure:: grass_logo.png\n");
        fprintf(stdout, "   :align: center\n");
        fprintf(stdout, "   :alt: GRASS logo\n\n");
    }
    else {
        fprintf(stdout, "![GRASS GIS logo](./grass_logo.png)\n");
    }
    /* horizontal line */
    fprintf(stdout, "---");
    if (rest)
        fprintf(stdout, "-");
    fprintf(stdout, "\n");
    
    /* header - GRASS module */
    if (!rest)
        fprintf(stdout, "## ");
    fprintf(stdout, "%s\n", _("NAME"));
    if (rest)
        fprintf(stdout, "----");
    fprintf(stdout, "\n");
    fprintf(stdout, "**%s**", st->pgm_name);
    
    if (st->module_info.label || st->module_info.description)
        fprintf(stdout, " - ");
    
    if (st->module_info.label)
        fprintf(stdout, "%s\n\n", st->module_info.label);
    
    if (st->module_info.description)
        fprintf(stdout, "%s\n", st->module_info.description);
    fprintf(stdout, "\n");
    if (!rest)
        fprintf(stdout, "### ");
    fprintf(stdout, "%s\n", _("KEYWORDS"));
    if (rest)
        fprintf(stdout, "----------------------\n");
    if (st->module_info.keywords) {
        G__print_keywords(stdout, G__print_escaped_for_html_keywords);
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
    if (!rest)
        fprintf(stdout, "### ");
    fprintf(stdout, "%s\n", _("SYNOPSIS"));
    if (rest) {
        fprintf(stdout, "----------------------\n");
        fprintf(stdout, "| ");
    }
    fprintf(stdout, "**%s**", st->pgm_name);
    if (!rest)
        fprintf(stdout, "\\");
    fprintf(stdout, "\n");
    if (rest)
        fprintf(stdout, "| ");
    fprintf(stdout, "**%s** --**help**", st->pgm_name);
    if (!rest)
        fprintf(stdout, "\\");
    fprintf(stdout, "\n**%s**", st->pgm_name);
    
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
            if (!opt->required)
                fprintf(stdout, " [");
            fprintf(stdout, "**%s**=*%s*", opt->key, type);
            if (opt->multiple) {
                fprintf(stdout, " [, *%s* ,...]", type);
            }
            if (!opt->required)
                fprintf(stdout, "] ");
            
            opt = opt->next_opt;
            fprintf(stdout, " ");
        }
    }
    if (new_prompt)
        fprintf(stdout, " [--**overwrite**] ");
    
    fprintf(stdout, " [--**verbose**] ");
    fprintf(stdout, " [--**quiet**] ");
    fprintf(stdout, " [--**ui**] ");    
    fprintf(stdout, "\n");
    
    /* now long version */
    fprintf(stdout, "\n");
    if (st->n_flags || new_prompt) {
        flag = &st->first_flag;
        if (!rest)
            fprintf(stdout, "#### ");
        fprintf(stdout, "%s:\n", _("Flags"));
        if (rest)
            fprintf(stdout, "~~~~~~\n");
        while (st->n_flags && flag != NULL) {
            print_flag(&flag->key, flag->label,
                       flag->description, rest);
            if (!rest)
                fprintf(stdout, "\\");
            fprintf(stdout, "\n");
            flag = flag->next_flag;
        }
        if (new_prompt) {
            print_flag("overwrite", NULL,
                       _("Allow output files to overwrite existing files"),
                       rest);
            if (!rest)
                fprintf(stdout, "\\");
            fprintf(stdout, "\n");
        }

        print_flag("help", NULL,
                   _("Print usage summary"), rest);
        if (!rest)
            fprintf(stdout, "\\");
        fprintf(stdout, "\n");
        print_flag("verbose", NULL,
                   _("Verbose module output"), rest);
        if (!rest)
            fprintf(stdout, "\\");
        fprintf(stdout, "\n");
        print_flag("quiet", NULL,
                   _("Quiet module output"), rest);
        if (!rest)
            fprintf(stdout, "\\");
        fprintf(stdout, "\n");
        print_flag("ui", NULL,
                   _("Force launching GUI dialog"), rest);
        fprintf(stdout, "\n");
    }
    
    fprintf(stdout, "\n");
    if (st->n_opts) {
        opt = &st->first_option;
        if (!rest)
            fprintf(stdout, "#### ");
        fprintf(stdout, "%s:\n", _("Parameters"));
        if (rest)
            fprintf(stdout, "~~~~~~~~~~~\n");
        
        while (opt != NULL) {
            print_option(opt, rest);
            opt = opt->next_opt;
            if (opt != NULL) {
                if (!rest)
                    fprintf(stdout, "\\");
            }
            fprintf(stdout, "\n");
        }
    }
}

void print_flag(const char *key, const char *label, const char *description, bool rest)
{
    if (rest)
        fprintf(stdout, "| ");
    if (strlen(key) > 1)
        fprintf(stdout, "-");
    fprintf(stdout, "-**%s**", key);
    if (!rest)
        fprintf(stdout, "\\");
    fprintf(stdout, "\n");
    if (label != NULL) {
        if (rest)
            fprintf(stdout, "| ");
        print_escaped_for_rest(stdout, label);
        if (!rest)
            fprintf(stdout, "\\");
        fprintf(stdout, "\n");
    }
    if (rest)
        fprintf(stdout, "| ");
    print_escaped_for_rest(stdout, "\t");
    print_escaped_for_rest(stdout, description);
}

void print_option(const struct Option *opt, bool rest)
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
    fprintf(stdout, "**%s**=*%s*", opt->key, type);
    if (opt->multiple) {
        fprintf(stdout, " [, *%s* ,...]", type);
    }
    /* fprintf(stdout, "*"); */
    if (opt->required) {
        fprintf(stdout, " **[required]**");
    }
    if (!rest)
        fprintf(stdout, "\\");
    fprintf(stdout, "\n");
    if (opt->label) {
        print_escaped_for_rest(stdout, "\t");
        print_escaped_for_rest(stdout, opt->label);
        if (!rest)
            fprintf(stdout, "\\");
        fprintf(stdout, "\n");
    }
    if (opt->description) {
        if (rest)
            fprintf(stdout, "| ");
        print_escaped_for_rest(stdout, "\t");
        print_escaped_for_rest(stdout, opt->description);
    }
    
    if (opt->options) {
        if (!rest)
            fprintf(stdout, "\\");
        fprintf(stdout, "\n");
        print_escaped_for_rest(stdout, "\t");
        fprintf(stdout, "%s: *", _("Options"));
        print_escaped_for_rest_options(stdout, opt->options);
    }
    
    if (opt->def) {
        if (!rest)
            fprintf(stdout, "\\");
        fprintf(stdout, "\n");
        print_escaped_for_rest(stdout, "\t");
        fprintf(stdout, "%s:", _("Default"));
        /* TODO check if value is empty
           if (!opt->def.empty()){ */
        fprintf(stdout, " *");
        print_escaped_for_rest(stdout, opt->def);
        if (!rest)
            fprintf(stdout, "\\");
        fprintf(stdout, "\n");
    }
    
    if (opt->descs) {
        int i = 0;
        
        while (opt->opts[i]) {
            if (opt->descs[i]) {
                if (!rest)
                    fprintf(stdout, "\\");
                fprintf(stdout, "\n");
                print_escaped_for_rest(stdout, "\t");
                print_escaped_for_rest(stdout, "\t");
                print_escaped_for_rest(stdout, opt->opts[i]);
                fprintf(stdout, "** : ");
                print_escaped_for_rest(stdout, opt->descs[i]);
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
    fputs(escaped, f);        \
    break
void print_escaped_for_rest(FILE *f, const char *str)
{
    const char *s;
    
    for (s = str; *s; s++) {
        switch (*s) {
            do_escape('\n', "\n\n");
            do_escape('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
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
        default:
            fputc(*s, f);
        }
    }
}

#undef do_escape

/*!
  \brief Print module usage description in reStructuredText format.
*/
void G__usage_rest(void)
{
    usage_rest_md(true);
}

/*!
  \brief Print module usage description in Markdown format.
*/
void G__usage_markdown(void)
{
    usage_rest_md(false);
}
