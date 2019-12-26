/*!
  \file lib/gis/parser_html.c
  
  \brief GIS Library - Argument parsing functions (HTML output)
  
  (C) 2001-2009, 2011-2013 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

static void print_escaped_for_html(FILE *, const char *);
static void print_escaped_for_html_options(FILE *, const char *);
static void print_escaped_for_html_keywords(FILE * , const char *);

/*!
  \brief Print module usage description in HTML format.
*/
void G__usage_html(void)
{
    struct Option *opt;
    struct Flag *flag;
    const char *type;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    if (!st->pgm_name)		/* v.dave && r.michael */
	st->pgm_name = G_program_name();
    if (!st->pgm_name)
	st->pgm_name = "??";

    fprintf(stdout,
	    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
    fprintf(stdout, "<html>\n<head>\n");
    fprintf(stdout, "<title>%s - GRASS GIS manual</title>\n", st->pgm_name);
    fprintf(stdout,
	    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n");
    fprintf(stdout,
	    "<link rel=\"stylesheet\" href=\"grassdocs.css\" type=\"text/css\">\n");
    fprintf(stdout, "</head>\n");
    fprintf(stdout, "<body bgcolor=\"white\">\n");
    fprintf(stdout, "<div id=\"container\">\n\n");
    fprintf(stdout,
	    "<a href=\"index.html\"><img src=\"grass_logo.png\" alt=\"GRASS logo\"></a>\n");
    fprintf(stdout, "<hr class=\"header\">\n\n");
    fprintf(stdout, "<h2>%s</h2>\n", _("NAME"));
    fprintf(stdout, "<em><b>%s</b></em> ", st->pgm_name);

    if (st->module_info.label || st->module_info.description)
	fprintf(stdout, " - ");

    if (st->module_info.label)
	fprintf(stdout, "%s<BR>\n", st->module_info.label);

    if (st->module_info.description)
	fprintf(stdout, "%s\n", st->module_info.description);


    fprintf(stdout, "<h2>%s</h2>\n", _("KEYWORDS"));
    if (st->module_info.keywords) {
	G__print_keywords(stdout, print_escaped_for_html_keywords);
	fprintf(stdout, "\n");
    }
    fprintf(stdout, "<h2>%s</h2>\n", _("SYNOPSIS"));
    fprintf(stdout, "<div id=\"name\"><b>%s</b><br></div>\n", st->pgm_name);
    fprintf(stdout, "<b>%s --help</b><br>\n", st->pgm_name);

    fprintf(stdout, "<div id=\"synopsis\"><b>%s</b>", st->pgm_name);

    /* print short version first */
    if (st->n_flags) {
	flag = &st->first_flag;
	fprintf(stdout, " [-<b>");
	while (flag != NULL) {
	    fprintf(stdout, "%c", flag->key);
	    flag = flag->next_flag;
	}
	fprintf(stdout, "</b>] ");
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
	    fprintf(stdout, "<b>%s</b>=<em>%s</em>", opt->key, type);
	    if (opt->multiple) {
		fprintf(stdout, "[,<i>%s</i>,...]", type);
	    }
	    if (!opt->required)
		fprintf(stdout, "] ");

	    opt = opt->next_opt;
	    fprintf(stdout, " ");
	}
    }
    if (new_prompt)
	fprintf(stdout, " [--<b>overwrite</b>] ");

    fprintf(stdout, " [--<b>help</b>] ");
    fprintf(stdout, " [--<b>verbose</b>] ");
    fprintf(stdout, " [--<b>quiet</b>] ");
    fprintf(stdout, " [--<b>ui</b>] ");
    
    fprintf(stdout, "\n</div>\n");

    /* now long version */
    fprintf(stdout, "\n");
    fprintf(stdout, "<div id=\"flags\">\n");
    fprintf(stdout, "<h3>%s:</h3>\n", _("Flags"));
    fprintf(stdout, "<dl>\n");
    if (st->n_flags) {
	flag = &st->first_flag;
	while (st->n_flags && flag != NULL) {
	    fprintf(stdout, "<dt><b>-%c</b></dt>\n", flag->key);

	    if (flag->label) {
		fprintf(stdout, "<dd>");
		fprintf(stdout, "%s", flag->label);
		fprintf(stdout, "</dd>\n");
	    }

	    if (flag->description) {
		fprintf(stdout, "<dd>");
		fprintf(stdout, "%s", flag->description);
		fprintf(stdout, "</dd>\n");
	    }

	    flag = flag->next_flag;
	    fprintf(stdout, "\n");
	}
    }
    if (new_prompt) {
	fprintf(stdout, "<dt><b>--overwrite</b></dt>\n");
	fprintf(stdout, "<dd>%s</dd>\n",
		_("Allow output files to overwrite existing files"));
    }
    /* these flags are always available */
    fprintf(stdout, "<dt><b>--help</b></dt>\n");
    fprintf(stdout, "<dd>%s</dd>\n", _("Print usage summary"));

    fprintf(stdout, "<dt><b>--verbose</b></dt>\n");
    fprintf(stdout, "<dd>%s</dd>\n", _("Verbose module output"));

    fprintf(stdout, "<dt><b>--quiet</b></dt>\n");
    fprintf(stdout, "<dd>%s</dd>\n", _("Quiet module output"));

    fprintf(stdout, "<dt><b>--ui</b></dt>\n");
    fprintf(stdout, "<dd>%s</dd>\n", _("Force launching GUI dialog"));

    fprintf(stdout, "</dl>\n");
    fprintf(stdout, "</div>\n");

    fprintf(stdout, "\n");
    fprintf(stdout, "<div id=\"parameters\">\n");
    if (st->n_opts) {
	opt = &st->first_option;
	fprintf(stdout, "<h3>%s:</h3>\n", _("Parameters"));
	fprintf(stdout, "<dl>\n");

	while (opt != NULL) {
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
	    fprintf(stdout, "<dt><b>%s</b>=<em>%s", opt->key, type);
	    if (opt->multiple) {
		fprintf(stdout, "[,<i>%s</i>,...]", type);
	    }
	    fprintf(stdout, "</em>");
	    if (opt->required) {
		fprintf(stdout, "&nbsp;<b>[required]</b>");
	    }
	    fprintf(stdout, "</dt>\n");

	    if (opt->label) {
		fprintf(stdout, "<dd>");
		print_escaped_for_html(stdout, opt->label);
		fprintf(stdout, "</dd>\n");
	    }
	    if (opt->description) {
		fprintf(stdout, "<dd>");
		print_escaped_for_html(stdout, opt->description);
		fprintf(stdout, "</dd>\n");
	    }

	    if (opt->options) {
		fprintf(stdout, "<dd>%s: <em>", _("Options"));
		print_escaped_for_html_options(stdout, opt->options);
		fprintf(stdout, "</em></dd>\n");
	    }

	    if (opt->def) {
		fprintf(stdout, "<dd>%s: <em>", _("Default"));
		print_escaped_for_html(stdout, opt->def);
		fprintf(stdout, "</em></dd>\n");
	    }

	    if (opt->descs) {
		int i = 0;
                
		while (opt->opts[i]) {
		    if (opt->descs[i]) {
			fprintf(stdout, "<dd><b>");
                        if (opt->gisprompt) {
                            char *thumbnails = NULL;
                            
                            if (strcmp(opt->gisprompt,
                                       "old,colortable,colortable") == 0)
                                thumbnails = "colortables";
                            else if (strcmp(opt->gisprompt,
                                            "old,barscale,barscale") == 0)
                                thumbnails = "barscales";
                            else if (strcmp(opt->gisprompt,
                                            "old,northarrow,northarrow") == 0)
                                thumbnails = "northarrows";

                            if (thumbnails)
                                fprintf(stdout, "<img height=\"12\" "
                                        "style=\"max-width: 80;\" "
                                        "src=\"%s/%s.png\" alt=\"%s\"> ",
                                        thumbnails, opt->opts[i], opt->opts[i]);
                        }
			print_escaped_for_html(stdout, opt->opts[i]);
			fprintf(stdout, "</b>: ");
			print_escaped_for_html(stdout, opt->descs[i]);
			fprintf(stdout, "</dd>\n");
		    }
		    i++;
		}
	    }

	    opt = opt->next_opt;
	    fprintf(stdout, "\n");
	}
	fprintf(stdout, "</dl>\n");
    }
    fprintf(stdout, "</div>\n");

    fprintf(stdout, "</body>\n</html>\n");
}


/*!
 * \brief Format text for HTML output
 */
#define do_escape(c,escaped) case c: fputs(escaped,f);break
void print_escaped_for_html(FILE * f, const char *str)
{
    const char *s;

    for (s = str; *s; s++) {
	switch (*s) {
	    do_escape('&', "&amp;");
	    do_escape('<', "&lt;");
	    do_escape('>', "&gt;");
	    do_escape('\n', "<br>");
	    do_escape('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
	default:
	    fputc(*s, f);
	}
    }
}

void print_escaped_for_html_options(FILE * f, const char *str)
{
    const char *s;

    for (s = str; *s; s++) {
	switch (*s) {
	    do_escape('&', "&amp;");
	    do_escape('<', "&lt;");
	    do_escape('>', "&gt;");
	    do_escape('\n', "<br>");
	    do_escape('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
	    do_escape(',',  ", ");
	default:
	    fputc(*s, f);
	}
    }
}

void print_escaped_for_html_keywords(FILE * f, const char * str)
{
    /* generate HTML links */

    /* HTML link only for second keyword */
    if (st->n_keys > 1 &&
        strcmp(st->module_info.keywords[1], str) == 0) {
    
        const char *s;
        
        /* TODO: fprintf(f, _("topic: ")); */
        fprintf(f, "<a href=\"topic_");
        for (s = str; *s; s++) {
            switch (*s) {
                do_escape(' ', "_");
            default:
                fputc(*s, f);
            }
        }
        fprintf(f, ".html\">%s</a>", str);
    }
    else { /* first and other than second keyword */
         if (st->n_keys > 0 &&
             strcmp(st->module_info.keywords[0], str) == 0) {
             /* command family */
             const char *s;

             fprintf(f, "<a href=\"");
             for (s = str; *s; s++) {
                 switch (*s) {
                     do_escape(' ', "_");
                 default:
                     fputc(*s, f);
                 }
             }
             fprintf(f, ".html\">%s</a>", str);
         } else {
             /* keyword index */
             if (st->n_keys > 0 &&
                strcmp(st->module_info.keywords[2], str) == 0) {

                /* TODO: fprintf(f, _("keywords: ")); */
                fprintf(f, "<a href=\"keywords.html#%s\">%s</a>", str, str);
             } else {
                fprintf(f, "<a href=\"keywords.html#%s\">%s</a>", str, str);
             }
         }
    }
}
#undef do_escape
