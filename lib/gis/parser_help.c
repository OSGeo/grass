#include "parser_local_proto.h"

static void show_options(int, const char *);
static int show(const char *, int );

/*!
 * \brief Command line help/usage message.
 *
 * Calls to G_usage() allow the programmer to print the usage
 * message at any time. This will explain the allowed and required
 * command line input to the user. This description is given according
 * to the programmer's definitions for options and flags. This function
 * becomes useful when the user enters options and/or flags on the
 * command line that are syntactically valid to the parser, but
 * functionally invalid for the command (e.g. an invalid file name.)
 *
 * For example, the parser logic doesn't directly support grouping
 * options. If two options be specified together or not at all, the
 * parser must be told that these options are not required and the
 * programmer must check that if one is specified the other must be as
 * well. If this additional check fails, then G_parser() will succeed,
 * but the programmer can then call G_usage() to print the standard
 * usage message and print additional information about how the two
 * options work together.
 */
void G_usage(void)
{
    struct Option *opt;
    struct Flag *flag;
    char item[256];
    const char *key_desc;
    int maxlen;
    int len, n;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    if (!st->pgm_name)		/* v.dave && r.michael */
	st->pgm_name = G_program_name();
    if (!st->pgm_name)
	st->pgm_name = "??";

    if (st->module_info.label || st->module_info.description) {
	fprintf(stderr, _("\nDescription:\n"));
	if (st->module_info.label)
	    fprintf(stderr, " %s\n", st->module_info.label);
	if (st->module_info.description)
	    fprintf(stderr, " %s\n", st->module_info.description);
    }
    if (st->module_info.keywords) {
	fprintf(stderr, _("\nKeywords:\n "));
	G__print_keywords(stderr, NULL);
	fprintf(stderr, "\n");
    }

    fprintf(stderr, _("\nUsage:\n "));

    len = show(st->pgm_name, 1);

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
	len = show(item, len);
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

	    len = show(item, len);

	    opt = opt->next_opt;
	}
    }
    if (new_prompt) {
	strcpy(item, " [--overwrite]");
	len = show(item, len);
    }

    strcpy(item, " [--verbose]");
    len = show(item, len);

    strcpy(item, " [--quiet]");
    len = show(item, len);


    fprintf(stderr, "\n");

    /* Print help info for flags */

    fprintf(stderr, _("\nFlags:\n"));

    if (st->n_flags) {
	flag = &st->first_flag;
	while (flag != NULL) {
	    fprintf(stderr, "  -%c   ", flag->key);

	    if (flag->label) {
		fprintf(stderr, "%s\n", flag->label);
		if (flag->description)
		    fprintf(stderr, "        %s\n", flag->description);

	    }
	    else if (flag->description) {
		fprintf(stderr, "%s\n", flag->description);
	    }

	    flag = flag->next_flag;
	}
    }

    if (new_prompt)
	fprintf(stderr, " --o   %s\n",
		_("Allow output files to overwrite existing files"));

    fprintf(stderr, " --v   %s\n", _("Verbose module output"));
    fprintf(stderr, " --q   %s\n", _("Quiet module output"));

    /* Print help info for options */

    if (st->n_opts) {
	fprintf(stderr, _("\nParameters:\n"));
	opt = &st->first_option;
	while (opt != NULL) {
	    fprintf(stderr, "  %*s   ", maxlen, opt->key);

	    if (opt->label) {
		fprintf(stderr, "%s\n", opt->label);
		if (opt->description) {
		    fprintf(stderr, "  %*s    %s\n",
			    maxlen, " ", opt->description);
		}
	    }
	    else if (opt->description) {
		fprintf(stderr, "%s\n", opt->description);
	    }

	    if (opt->options)
		show_options(maxlen, opt->options);
	    /*
	       fprintf (stderr, "  %*s   options: %s\n", maxlen, " ",
	       _(opt->options)) ;
	     */
	    if (opt->def)
		fprintf(stderr, _("  %*s   default: %s\n"), maxlen, " ",
			opt->def);

	    if (opt->descs) {
		int i = 0;

		while (opt->opts[i]) {
		    if (opt->descs[i])
			fprintf(stderr, "  %*s    %s: %s\n",
				maxlen, " ", opt->opts[i], opt->descs[i]);

		    i++;
		}
	    }

	    opt = opt->next_opt;
	}
    }
}

static void show_options(int maxlen, const char *str)
{
    char *buff = G_store(str);
    char *p1, *p2;
    int totlen, len;

    fprintf(stderr, _("  %*s   options: "), maxlen, " ");
    totlen = maxlen + 13;
    p1 = buff;
    while ((p2 = G_index(p1, ','))) {
	*p2 = '\0';
	len = strlen(p1) + 1;
	if ((len + totlen) > 76) {
	    totlen = maxlen + 13;
	    fprintf(stderr, "\n %*s", maxlen + 13, " ");
	}
	fprintf(stderr, "%s,", p1);
	totlen += len;
	p1 = p2 + 1;
    }
    len = strlen(p1);
    if ((len + totlen) > 76)
	fprintf(stderr, "\n %*s", maxlen + 13, " ");
    fprintf(stderr, "%s\n", p1);

    G_free(buff);
}

static int show(const char *item, int len)
{
    int n;

    n = strlen(item) + (len > 0);
    if (n + len > 76) {
	if (len)
	    fprintf(stderr, "\n  ");
	len = 0;
    }
    fprintf(stderr, "%s", item);
    return n + len;
}
