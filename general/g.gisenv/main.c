
/****************************************************************************
 *
 * MODULE:       g.gisenv
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Markus Neteler <neteler itc.it>
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2003-2006, 2011-2013 by the GRASS Development Team
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

static char *parse_variable(const char *, char **);

int main(int argc, char *argv[])
{
    int n, store, nopts;
    const char *name, *u_name;
    char *value;
    struct Option *get_opt, *set_opt, *unset_opt, *store_opt;
    struct Flag *flag_s, *flag_n;
    struct GModule *module;

    G_set_program_name(argv[0]);
    G_no_gisinit();

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("settings"));
    G_add_keyword(_("variables"));
    module->label =
	_("Outputs and modifies the user's current GRASS variable settings.");
    module->description = _("Prints all defined GRASS variables if no option is given.");

    get_opt = G_define_option();
    get_opt->key = "get";
    get_opt->type = TYPE_STRING;
    get_opt->description = _("GRASS variable to get");
    get_opt->key_desc = "VARIABLE";
    get_opt->required = NO;
    get_opt->guisection = _("Get");

    set_opt = G_define_option();
    set_opt->key = "set";
    set_opt->type = TYPE_STRING;
    set_opt->description = _("GRASS variable to set");
    set_opt->key_desc = "\"VARIABLE=value\"";
    set_opt->required = NO;
    set_opt->guisection = _("Set");

    unset_opt = G_define_option();
    unset_opt->key = "unset";
    unset_opt->type = TYPE_STRING;
    unset_opt->description = _("GRASS variable to unset");
    unset_opt->key_desc = "VARIABLE";
    unset_opt->required = NO;
    unset_opt->guisection = _("Set");

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

    nopts = 0;
    if (get_opt->answer)
	nopts++;
    if (set_opt->answer)
	nopts++;
    if (unset_opt->answer)
	nopts++;

    if (nopts == 0) {
	/* Print or optionally set environment variables */
	int quote;
	
	if (flag_s->answer)
	    quote = TRUE;
	else if (flag_n->answer)
	    quote = FALSE;
	else
	    quote = !isatty(fileno(stdout));
	
	for (n = 0; (name = G__env_name(n)); n++) {
	    value = (char *)G__getenv(name);
	    if (value) {
		if (!quote)
		    fprintf(stdout, "%s=%s\n", name, value);
		else
		    fprintf(stdout, "%s='%s';\n", name, value);
	    }
	}
	exit(EXIT_SUCCESS);
    }
    
    if (nopts != 1)
	G_fatal_error(_("Options <%s>, <%s>, and <%s> are mutually exclusive"),
		      get_opt->key, set_opt->key, unset_opt->key);
    
    store = G_VAR_GISRC;
    if (store_opt->answer[0] == 'm')
	store = G_VAR_MAPSET;

    if (get_opt->answer) {
        u_name = parse_variable(get_opt->answer, NULL);
	value = (char *)G_getenv2(u_name, store);
	fprintf(stdout, "%s\n", value);
	exit(EXIT_SUCCESS);
    }

    u_name = NULL;
    if (set_opt->answer) {
        u_name = parse_variable(set_opt->answer, &value);
        if (value) {
	    G_setenv2(u_name, value, store);
	}
	else {
            /* unset */
	    G_getenv2(u_name, store); /* G_fatal_error() if not defined */
	    G_unsetenv2(u_name, store);
	}
    }
    
    if (unset_opt->answer) {
        u_name = parse_variable(unset_opt->answer, &value);
        if (value)
            G_warning(_("Value '%s' ignored when unsetting the GRASS variable"),
                      value);
        
	G_getenv2(u_name, store); /* G_fatal_error() if not defined */
	G_unsetenv2(u_name, store);
    }

    if (u_name)
        exit(EXIT_SUCCESS);

    /* Something's wrong if we got this far */
    G_usage();

    exit(EXIT_FAILURE);
}

char *parse_variable(const char *v_name, char **value)
{
    char *u_name; /* uppercase variable name */
    char *name, *ptr;

    name  = G_store(v_name);
    if (value)
        *value = NULL;

    ptr = strchr(name, '=');
    if (ptr != NULL) {
        *ptr = '\0';
        if (value)
            *value = ptr + 1;
    }
    /* Allow unset without '=' sign */
    if (value) {
        if (*value != NULL && **value == '\0')
            *value = NULL;
    }
    if (strlen(name) < 1)
        G_fatal_error(_("GRASS variable not defined"));

    /* Check variable uppercase */
    u_name = G_store(name);
    G_str_to_upper(u_name);
    if (strcmp(name, u_name) != 0) {
        G_verbose_message(_("GRASS variable must be uppercase. Using '%s'."),
                          u_name);
    }

    return u_name;
}
