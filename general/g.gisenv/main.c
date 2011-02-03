
/****************************************************************************
 *
 * MODULE:       g.gisenv
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2003-2006, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    int n, store;
    const char *name, *value;
    char *ptr;
    struct Option *get, *set, *store_opt;
    struct Flag *flag_s, *flag_n;
    struct GModule *module;

    G_set_program_name(argv[0]);
    G_no_gisinit();

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("settings"));
    module->description =
	_("Outputs and modifies the user's current GRASS variable settings.");

    get = G_define_option();
    get->key = "get";
    get->type = TYPE_STRING;
    get->description = _("GRASS variable to get");
    get->key_desc = "VARIABLE";
    get->required = NO;
    get->guisection = _("Get");

    set = G_define_option();
    set->key = "set";
    set->type = TYPE_STRING;
    set->description = _("GRASS variable to set");
    set->key_desc = "VARIABLE=value";
    set->required = NO;
    set->guisection = _("Set");

    store_opt = G_define_option();
    store_opt->key = "store";
    store_opt->type = TYPE_STRING;
    store_opt->options = "gisrc,mapset";
    store_opt->answer = "gisrc";
    store_opt->description = _("Where GRASS variable is stored");
    store_opt->required = NO;
    store_opt->guisection = _("Set");

    flag_s = G_define_flag();
    flag_s->key = 's';
    flag_s->description = _("Use shell syntax (for \"eval\")");
    flag_s->guisection = _("Format");

    flag_n = G_define_flag();
    flag_n->key = 'n';
    flag_n->description = _("Don't use shell syntax");
    flag_n->guisection = _("Format");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (flag_s->answer && flag_n->answer)
	G_fatal_error(_("Flags -%c and -%c are mutually exclusive"), flag_s->key, flag_n->key);

    /* Print or optionally set environment variables */
    if (!get->answer && !set->answer) {
	for (n = 0; (name = G__env_name(n)); n++) {
	    int quote;

	    if (flag_s->answer)
		quote = 1;
	    else if (flag_n->answer)
		quote = 0;
	    else
		quote = !isatty(fileno(stdout));

	    if ((value = G__getenv(name))) {
		if (!quote)
		    fprintf(stdout, "%s=%s\n", name, value);
		else
		    fprintf(stdout, "%s='%s';\n", name, value);
	    }
	}
	return 0;
    }

    store = G_VAR_GISRC;
    if (store_opt->answer[0] == 'm')
	store = G_VAR_MAPSET;

    if (get->answer != NULL) {
	value = G__getenv2(get->answer, store);
	if (value != NULL)
	    fprintf(stdout, "%s\n", value);
	return 0;
    }

    if (set->answer != NULL) {
	value = NULL;
	name = set->answer;
	ptr = strchr(name, '=');
	if (ptr != NULL) {
	    *ptr = '\0';
	    value = ptr + 1;
	}
	/* Allow unset without '=' sign */
	if (value != NULL && *value == '\0')
	    value = NULL;

	G_setenv2(name, value, store);

	return 0;
    }

    /* Something's wrong if we got this far */
    G_usage();
    exit(EXIT_FAILURE);
}
