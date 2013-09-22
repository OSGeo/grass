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

static void print_escaped_for_html(FILE * f, const char *str);
static void print_escaped_for_html_options(FILE * f, const char *str);

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
    fprintf(stdout, "<title>GRASS GIS manual: %s</title>\n", st->pgm_name);
    fprintf(stdout,
	    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n");
    fprintf(stdout,
	    "<link rel=\"stylesheet\" href=\"grassdocs.css\" type=\"text/css\">\n");
    fprintf(stdout, "</head>\n");
    fprintf(stdout, "<body bgcolor=\"white\">\n\n");
    fprintf(stdout,
	    "<img src=\"grass_logo.png\" alt=\"GRASS logo\"><hr align=center size=6 noshade>\n\n");
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
	G__print_keywords(stdout, NULL);
	fprintf(stdout, "\n");
    }
    fprintf(stdout, "<h2>%s</h2>\n", _("SYNOPSIS"));
    fprintf(stdout, "<div id=\"name\"><b>%s</b><br></div>\n", st->pgm_name);
    fprintf(stdout, "<b>%s help</b><br>\n", st->pgm_name);

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

    fprintf(stdout, " [--<b>verbose</b>] ");
    fprintf(stdout, " [--<b>quiet</b>] ");

    fprintf(stdout, "\n</div>\n");


    /* now long version */
    fprintf(stdout, "\n");
    fprintf(stdout, "<div id=\"flags\">\n");
    if (st->n_flags || new_prompt) {
	flag = &st->first_flag;
	fprintf(stdout, "<h3>%s:</h3>\n", _("Flags"));
	fprintf(stdout, "<dl>\n");
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
	if (new_prompt) {
	    fprintf(stdout, "<dt><b>--overwrite</b></dt>\n");
	    fprintf(stdout, "<dd>%s</dd>\n",
		    _("Allow output files to overwrite existing files"));
	}

	fprintf(stdout, "<dt><b>--verbose</b></dt>\n");
	fprintf(stdout, "<dd>%s</dd>\n", _("Verbose module output"));

	fprintf(stdout, "<dt><b>--quiet</b></dt>\n");
	fprintf(stdout, "<dd>%s</dd>\n", _("Quiet module output"));

	fprintf(stdout, "</dl>\n");
    }
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
                            
                            if (thumbnails)
                                fprintf(stdout, "<img width=\"80\" height=\"12\" "
                                        "src=\"%s/%s.png\" alt=\"%s\">",
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
#undef do_escape
