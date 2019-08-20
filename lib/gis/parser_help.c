/*!
  \file lib/gis/parser_help.c
 
  \brief GIS Library - Argument parsing functions (help)
  
  (C) 2001-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
  \author Soeren Gebbert added Dec. 2009 WPS process_description document
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

static void usage(FILE *fp, int markers);
static void show_options(FILE *fp, int maxlen, const char *str);
static int show(FILE *fp, const char *item, int len);

/*!
  \brief Command line help/usage message.
  
  Calls to G_usage() allow the programmer to print the usage
  message at any time. This will explain the allowed and required
  command line input to the user. This description is given according
  to the programmer's definitions for options and flags. This function
  becomes useful when the user enters options and/or flags on the
  command line that are syntactically valid to the parser, but
  functionally invalid for the command (e.g. an invalid file name.)
  
  For example, the parser logic doesn't directly support grouping
  options. If two options be specified together or not at all, the
  parser must be told that these options are not required and the
  programmer must check that if one is specified the other must be as
  well. If this additional check fails, then G_parser() will succeed,
  but the programmer can then call G_usage() to print the standard
  usage message and print additional information about how the two
  options work together.
*/
void G_usage(void)
{
    usage(stderr, 0);
}

void G__usage_text(void)
{
    usage(stdout, 1);
}

static void usage(FILE *fp, int markers)
{
    struct Option *opt;
    struct Flag *flag;
    char item[256];
    const char *key_desc;
    int maxlen;
    int len, n;
    int new_prompt = 0;
    int extensive = 0;  /* include also less important parts */
    int standard = 0;  /* include also standard flags */
    int detailed = 0;  /* details for each flag and option */

    new_prompt = G__uses_new_gisprompt();

    if (!st->pgm_name)		/* v.dave && r.michael */
	st->pgm_name = G_program_name();
    if (!st->pgm_name)
	st->pgm_name = "??";

    if (st->module_info.label || st->module_info.description) {
        if (extensive)
            fprintf(fp, "\n");
	if (markers)
	    fprintf(fp, "{{{DESCRIPTION}}}\n");
        if (extensive) {
            fprintf(fp, "%s\n", _("Description:"));
            if (st->module_info.label)
                fprintf(fp, " %s\n", st->module_info.label);
            if (st->module_info.description)
                fprintf(fp, " %s\n", st->module_info.description);
        }
        else {
            /* print label, if no label, try description */
            /* no leading space without heading */
            if (st->module_info.label)
                fprintf(fp, "%s\n", st->module_info.label);
            else if (st->module_info.description)
                fprintf(fp, "%s\n", st->module_info.description);
        }
    }
    if (extensive &&& st->module_info.keywords) {
	fprintf(fp, "\n");
	if (markers)
	    fprintf(fp, "{{{KEYWORDS}}}\n");
	fprintf(fp, "%s\n ", _("Keywords:"));
	G__print_keywords(fp, NULL);
	fprintf(fp, "\n");
    }

    fprintf(fp, "\n");
    if (markers)
	fprintf(fp, "{{{USAGE}}}\n");
    fprintf(fp, "%s\n ", _("Usage:"));

    len = show(fp, st->pgm_name, 1);

    /* Print flags */

    if (st->n_flags) {
	item[0] = ' ';
	item[1] = '[';
	item[2] = '-';
	flag = &st->first_flag;
	for (n = 3; flag != NULL; n++, flag = flag->next_flag)
	    item[n] = flag->key;
	item[n++] = ']';
	item[n] = 0;
	len = show(fp, item, len);
    }

    maxlen = 0;
    if (st->n_opts) {
	opt = &st->first_option;
	while (opt != NULL) {
	    if (opt->key_desc != NULL)
		key_desc = opt->key_desc;
	    else if (opt->type == TYPE_STRING)
		key_desc = "string";
	    else
		key_desc = "value";

	    if (!opt->key) {
		fprintf(stderr, "\n%s\n", _("ERROR: Option key not defined"));
		exit(EXIT_FAILURE);
	    }
	    n = strlen(opt->key);
	    if (n > maxlen)
		maxlen = n;

	    strcpy(item, " ");
	    if (!opt->required)
		strcat(item, "[");
	    strcat(item, opt->key);
	    strcat(item, "=");
	    strcat(item, key_desc);
	    if (opt->multiple) {
		strcat(item, "[,");
		strcat(item, key_desc);
		strcat(item, ",...]");
	    }
	    if (!opt->required)
		strcat(item, "]");

	    len = show(fp, item, len);

	    opt = opt->next_opt;
	}
    }
    if (new_prompt) {
	strcpy(item, " [--overwrite]");
	len = show(fp, item, len);
    }

    strcpy(item, " [--help]");
    len = show(fp, item, len);

    strcpy(item, " [--verbose]");
    len = show(fp, item, len);

    strcpy(item, " [--quiet]");
    len = show(fp, item, len);

    strcpy(item, " [--ui]");
    len = show(fp, item, len);

    fprintf(fp, "\n");

    /* Print help info for flags */

    /* Show section only when there are flags.
     * There are always the standard flags if we are printing those.
     * There is no use case for the markers, so no way to decide if
     * the marker for flags is mandatory and should be empty if it is
     * okay for it to be missing like in the current implementation.
     */
    if (st->n_flags || standard) {
	fprintf(fp, "\n");
	if (markers)
	    fprintf(fp, "{{{FLAGS}}}\n");
	fprintf(fp, "%s\n", _("Flags:"));
    }

    if (st->n_flags) {
	flag = &st->first_flag;
	while (flag != NULL) {
	    fprintf(fp, "  -%c   ", flag->key);

	    if (flag->label) {
		fprintf(fp, "%s\n", flag->label);
		if (detailed && flag->description)
		    fprintf(fp, "        %s\n", flag->description);

	    }
	    else if (flag->description) {
		fprintf(fp, "%s\n", flag->description);
	    }

	    flag = flag->next_flag;
	}
    }

    if (standard) {
        if (new_prompt)
            fprintf(fp, " --o   %s\n",
                    _("Allow output files to overwrite existing files"));

        fprintf(fp, " --h   %s\n", _("Print usage summary"));
        fprintf(fp, " --v   %s\n", _("Verbose module output"));
        fprintf(fp, " --q   %s\n", _("Quiet module output"));
        fprintf(fp, " --qq  %s\n", _("Super quiet module output"));
        fprintf(fp, " --ui  %s\n", _("Force launching GUI dialog"));
    }

    /* Print help info for options */

    if (st->n_opts) {
	fprintf(fp, "\n");
	if (markers)
	    fprintf(fp, "{{{PARAMETERS}}}\n");
	fprintf(fp, "%s\n", _("Parameters:"));
	opt = &st->first_option;
	while (opt != NULL) {
	    fprintf(fp, "  %*s   ", maxlen, opt->key);

	    if (opt->label) {
		fprintf(fp, "%s\n", opt->label);
                if (detailed && opt->description) {
		    fprintf(fp, "  %*s    %s\n",
			    maxlen, " ", opt->description);
		}
	    }
	    else if (opt->description) {
		fprintf(fp, "%s\n", opt->description);
	    }

	    if (opt->options)
		show_options(fp, maxlen, opt->options);
	    /*
	       fprintf (fp, "  %*s   options: %s\n", maxlen, " ",
	       _(opt->options)) ;
	     */
	    if (opt->def)
		fprintf(fp, _("  %*s   default: %s\n"), maxlen, " ",
			opt->def);

            if (detailed && opt->descs) {
		int i = 0;

		while (opt->opts[i]) {
		    if (opt->descs[i])
			fprintf(fp, "  %*s    %s: %s\n",
				maxlen, " ", opt->opts[i], opt->descs[i]);

		    i++;
		}
	    }

	    opt = opt->next_opt;
	}
    }
}

static void show_options(FILE *fp, int maxlen, const char *str)
{
    char *buff = G_store(str);
    char *p1, *p2;
    int totlen, len;

    fprintf(fp, _("  %*s   options: "), maxlen, " ");
    totlen = maxlen + 13;
    p1 = buff;
    while ((p2 = strchr(p1, ','))) {
	*p2 = '\0';
	len = strlen(p1) + 1;
	if ((len + totlen) > 76) {
	    totlen = maxlen + 13;
	    fprintf(fp, "\n %*s", maxlen + 13, " ");
	}
	fprintf(fp, "%s,", p1);
	totlen += len;
	p1 = p2 + 1;
    }
    len = strlen(p1);
    if ((len + totlen) > 76)
	fprintf(fp, "\n %*s", maxlen + 13, " ");
    fprintf(fp, "%s\n", p1);

    G_free(buff);
}

static int show(FILE *fp, const char *item, int len)
{
    int n;

    n = strlen(item) + (len > 0);
    if (n + len > 76) {
	if (len)
	    fprintf(fp, "\n  ");
	len = 0;
    }
    fprintf(fp, "%s", item);
    return n + len;
}
