/*!
  \file lib/gis/parser_rest.c
  
  \brief GIS Library - Argument parsing functions (reStructuredText output)
  
  (C) 2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Luca Delucchi
*/
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"


static void print_escaped_for_rest(FILE * f, const char *str);
static void print_escaped_for_rest_options(FILE * f, const char *str);


/*!
  \brief Print module usage description in reStructuredText format.
*/
void G__usage_rest(void)
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

    fprintf(stdout, "=================");
    for (s = 0; s <= strlen(st->pgm_name); s++) {
        fprintf(stdout, "=");
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "%s - GRASS GIS manual\n", st->pgm_name);
    fprintf(stdout, "=================");
    for (s = 0; s <= strlen(st->pgm_name); s++) {
        fprintf(stdout, "=");
    }
    fprintf(stdout, "\n\n");
    
    fprintf(stdout,".. figure:: grass_logo.png\n");
    fprintf(stdout,"   :align: center\n");
    fprintf(stdout,"   :alt: GRASS logo\n\n");
    
    fprintf(stdout,"%s\n----\n", _("NAME"));
    fprintf(stdout, "**%s**", st->pgm_name);

    if (st->module_info.label || st->module_info.description)
	fprintf(stdout, " - ");

    if (st->module_info.label)
	fprintf(stdout, "%s\n\n", st->module_info.label);

    if (st->module_info.description)
	fprintf(stdout, "%s\n", st->module_info.description);


    fprintf(stdout, "\n%s\n----------------------\n", _("KEYWORDS"));
    if (st->module_info.keywords) {
	G__print_keywords(stdout, NULL);
	fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n%s\n----------------------\n", _("SYNOPSIS"));
    fprintf(stdout, "**%s**\n\n", st->pgm_name);
    fprintf(stdout, "**%s** --help\n\n", st->pgm_name);

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
	    fprintf(stdout, "**%s** = *%s*", opt->key, type);
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
	fprintf(stdout, " [-- **overwrite**] ");

    fprintf(stdout, " [-- **verbose**] ");
    fprintf(stdout, " [-- **quiet**] ");

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
	    fprintf(stdout, "-- **overwrite**\n");
	    fprintf(stdout, "    %s\n",
		    _("Allow output files to overwrite existing files"));
	}

	fprintf(stdout, "-- **verbose**\n");
	fprintf(stdout, "    %s\n", _("Verbose module output"));

	fprintf(stdout, "-- **quiet**\n");
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
	    fprintf(stdout, "**%s** = *%s*", opt->key, type);
	    if (opt->multiple) {
		fprintf(stdout, " [, *%s* ,...]", type);
	    }
	    /* fprintf(stdout, "*"); */
	    if (opt->required) {
		fprintf(stdout, " **[required]**");
	    }
	    fprintf(stdout, "\n\n");
	    if (opt->label) {
	        fprintf(stdout, "\t");
		print_escaped_for_rest(stdout, opt->label);
		/* fprintf(stdout, "    %s\n", opt->label); */
		fprintf(stdout, "\n\n");
	    }
	    if (opt->description) {
	        fprintf(stdout, "\t");
	        print_escaped_for_rest(stdout, opt->description);
		/* fprintf(stdout, "    %s\n", opt->description); */
		fprintf(stdout, "\n\n");
	    }

	    if (opt->options) {
		fprintf(stdout, "\t%s: *", _("Options"));
		print_escaped_for_rest_options(stdout, opt->options);
		/* fprintf(stdout, "%s", opt->options);*/
		fprintf(stdout, "*\n\n");
	    }

	    if (opt->def) {
		fprintf(stdout, "\t%s:", _("Default"));
		/* TODO check if value is empty
		if (!opt->def.empty()){ */
		    fprintf(stdout, " *");
		    print_escaped_for_rest(stdout, opt->def); 
		    /* fprintf(stdout,"%s", opt->def); */
		    fprintf(stdout, "*\n\n");
		/* } */
		fprintf(stdout, "\n\n");
	    }

	    if (opt->descs) {
		int i = 0;

		while (opt->opts[i]) {
		    if (opt->descs[i]) {
			fprintf(stdout, "\t\t**");
			print_escaped_for_rest(stdout, opt->opts[i]);
			/*fprintf(stdout,"%s", opt->opts[i]); */
			fprintf(stdout, "** : ");
			print_escaped_for_rest(stdout, opt->descs[i]);
			/* fprintf(stdout, "%s\n", opt->descs[i]); */
			fprintf(stdout, "\n\n");
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
 * \brief Format text for reStructuredText output
 */
#define do_escape(c,escaped) case c: fputs(escaped,f);break
 void print_escaped_for_rest(FILE * f, const char *str)
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
 
 void print_escaped_for_rest_options(FILE * f, const char *str)
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
