#include <stdio.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"
#include "string.h"
// static void print_escaped_for_html(FILE * f, const char *str);
// static void print_escaped_for_html_options(FILE * f, const char *str);

/*!
  \brief Print module usage description in HTML format.
*/
void G__usage_rest(void)
{
    struct Option *opt;
    struct Flag *flag;
    const char *type;
    int new_prompt = 0;
    int s;

    new_prompt = G__uses_new_gisprompt();

    if (!st->pgm_name)		/* v.dave && r.michael */
	st->pgm_name = G_program_name();
    if (!st->pgm_name)
	st->pgm_name = "??";

    fprintf(stdout, "=================");
    for (s = 0; s <= strlen(st->pgm_name); s++) {
        fprintf(stdout, "=");
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "GRASS GIS manual: %s\n", st->pgm_name);
    fprintf(stdout, "=================");
    for (s = 0; s <= strlen(st->pgm_name); s++) {
        fprintf(stdout, "=");
    }
    fprintf(stdout, "\n\n");
    
    fprintf(stdout,".. figure:: grass_logo.png\n");
    fprintf(stdout,"   :align: center\n");
    fprintf(stdout,"   :alt: GRASS logo\n\n");
    
    fprintf(stdout,"%s\n----\n", _("NAME"));
    fprintf(stdout, "**%s**", st->pgm_name); //TODO fix bold + emphase now only bold

    if (st->module_info.label || st->module_info.description)
	fprintf(stdout, " - ");

    if (st->module_info.label)
	fprintf(stdout, "%s\n", st->module_info.label);

    if (st->module_info.description)
	fprintf(stdout, "%s\n", st->module_info.description);


    fprintf(stdout, "%s\n--------\n", _("KEYWORDS"));
    if (st->module_info.keywords) {
	G__print_keywords(stdout, NULL);
	fprintf(stdout, "\n");
    }
    fprintf(stdout, "%s\n--------\n", _("SYNOPSIS"));
    fprintf(stdout, "**%s**\n\n", st->pgm_name);
    fprintf(stdout, "**%s** help\n\n", st->pgm_name);

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
	    if (!opt->required)
		fprintf(stdout, " [");
	    fprintf(stdout, "**%s**=*%s*", opt->key, type);
	    if (opt->multiple) {
		fprintf(stdout, "[,*%s*,...]", type);
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

    fprintf(stdout, "\n");


    /* now long version */
    fprintf(stdout, "\n");
    if (st->n_flags || new_prompt) {
	flag = &st->first_flag;
	fprintf(stdout, "%s:\n~~~~~~\n", _("Flags"));
	while (st->n_flags && flag != NULL) {
	    fprintf(stdout, "**-%c**\n", flag->key);

	    if (flag->label) {
		fprintf(stdout, "    %s", flag->label);
	    }

	    if (flag->description) {
		fprintf(stdout, "    %s", flag->description);
	    }

	    flag = flag->next_flag;
	    fprintf(stdout, "\n");
	}
	if (new_prompt) {
	    fprintf(stdout, "**--overwrite**\n");
	    fprintf(stdout, "    %s\n",
		    _("Allow output files to overwrite existing files"));
	}

	fprintf(stdout, "**--verbose**\n");
	fprintf(stdout, "    %s\n", _("Verbose module output"));

	fprintf(stdout, "**--quiet**\n");
	fprintf(stdout, "    %s\n", _("Quiet module output"));

	fprintf(stdout, "\n");
    }

    fprintf(stdout, "\n");
    if (st->n_opts) {
	opt = &st->first_option;
	fprintf(stdout, "%s:\n~~~~~~~~~~~\n", _("Parameters"));

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
	    fprintf(stdout, "**%s** = *%s", opt->key, type);
	    if (opt->multiple) {
		fprintf(stdout, "[,*%s*,...]", type);
	    }
	    fprintf(stdout, "*");
	    if (opt->required) {
		fprintf(stdout, " ;**[required]**");
	    }
	    fprintf(stdout, "\n");
	    if (opt->label) {
		fprintf(stdout, "    %s\n", opt->label);
	    }
	    if (opt->description) {
		fprintf(stdout, "    %s\n", opt->description);
	    }

	    if (opt->options) {
		fprintf(stdout, "    %s: *", _("Options"));
// 		print_escaped_for_html_options(stdout, opt->options);
		fprintf(stdout, "%s", opt->options);
		fprintf(stdout, "*\n");
	    }

	    if (opt->def) {
		fprintf(stdout, "    %s: *", _("Default"));
// 		print_escaped_for_html(stdout, opt->def);
		fprintf(stdout,"%s", opt->def);
		fprintf(stdout, "*\n");
	    }

	    if (opt->descs) {
		int i = 0;

		while (opt->opts[i]) {
		    if (opt->descs[i]) {
			fprintf(stdout, "    **");
// 			print_escaped_for_html(stdout, opt->opts[i]);
			fprintf(stdout,"%s", opt->opts[i]);
			fprintf(stdout, "**: ");
// 			print_escaped_for_html(stdout, opt->descs[i]);
			fprintf(stdout, "%s\n", opt->descs[i]);
		    }
		    i++;
		}
	    }

	    opt = opt->next_opt;
	    fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");
    }

}


/*!
 * \brief Format text for HTML output
 */
#define do_escape(c,escaped) case c: fputs(escaped,f);break
// void print_escaped_for_html(FILE * f, const char *str)
// {
//     const char *s;
// 
//     for (s = str; *s; s++) {
// 	switch (*s) {
// 	    do_escape('&', "&amp;");
// 	    do_escape('<', "&lt;");
// 	    do_escape('>', "&gt;");
// 	    do_escape('\n', "<br>");
// 	    do_escape('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
// 	default:
// 	    fputc(*s, f);
// 	}
//     }
// }
// 
// void print_escaped_for_html_options(FILE * f, const char *str)
// {
//     const char *s;
// 
//     for (s = str; *s; s++) {
// 	switch (*s) {
// 	    do_escape('&', "&amp;");
// 	    do_escape('<', "&lt;");
// 	    do_escape('>', "&gt;");
// 	    do_escape('\n', "<br>");
// 	    do_escape('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
// 	    do_escape(',',  ", ");
// 	default:
// 	    fputc(*s, f);
// 	}
//     }
// }
#undef do_escape
