/*  
 ****************************************************************************
 *
 * MODULE:       g.message 
 * AUTHOR(S):    Jachym Cepicky - jachym AT les-ejk cz
 *               Hamish Bowman - hamish_b AT yahoo com
 *               Martin Landa - landa.martin AT gmail.com
 * PURPOSE:      Provides a means of reporting the contents of GRASS
 *               projection information files and creating
 *               new projection information files.
 * COPYRIGHT:    (C) 2007,2010,2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Flag *warning, *fatal, *percent, *debug_flag, *verbose, *important;
    struct Option *message, *debug_opt;
    struct GModule *module;
    int debug_level;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("support"));
    G_add_keyword(_("scripts"));
    module->label =
	_("Prints a message, warning, progress info, or fatal error in the GRASS way.");
    module->description =
	_("This module should be used in scripts for messages served to user.");

    warning = G_define_flag();
    warning->key = 'w';
    warning->guisection = _("Type");
    warning->description = _("Print message as warning");

    fatal = G_define_flag();
    fatal->key = 'e';
    fatal->guisection = _("Type");
    fatal->description = _("Print message as fatal error");

    debug_flag = G_define_flag();
    debug_flag->key = 'd';
    debug_flag->guisection = _("Type");
    debug_flag->description = _("Print message as debug message");

    percent = G_define_flag();
    percent->key = 'p';
    percent->guisection = _("Type");
    percent->description = _("Print message as progress info");

    important = G_define_flag();
    important->key = 'i';
    important->guisection = _("Level");
    important->label = _("Print message in all modes except of quiet mode");
    important->description = _("Message is printed on GRASS_VERBOSE>=1");
    
    verbose = G_define_flag();
    verbose->key = 'v';
    verbose->guisection = _("Level");
    verbose->label = _("Print message only in verbose mode");
    verbose->description = _("Message is printed only on GRASS_VERBOSE>=3");

    message = G_define_option();
    message->key = "message";
    message->type = TYPE_STRING;
    message->key_desc = "string";
    message->required = YES;
    message->label = _("Text of the message to be printed");
    message->description = _("Message is printed on GRASS_VERBOSE>=2");

    debug_opt = G_define_option();
    debug_opt->key = "debug";
    debug_opt->type = TYPE_INTEGER;
    debug_opt->required = NO;
    debug_opt->guisection = _("Level");
    debug_opt->answer = "1";
    debug_opt->options = "0-5";
    debug_opt->description = _("Level to use for debug messages");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (fatal->answer + warning->answer + debug_flag->answer +
	verbose->answer > 1)
	G_fatal_error(_("Select only one message level"));

    debug_level = atoi(debug_opt->answer);
    
    if (fatal->answer)
	G_fatal_error("%s", message->answer);
    else if (warning->answer)
	G_warning("%s", message->answer);
    else if (percent->answer) {
	int i, n, s;
	i = n = s = -1;
	sscanf(message->answer, "%d %d %d", &i, &n, &s);
	if (s == -1) 
	    G_fatal_error(_("Unable to parse input message"));
	G_percent(i, n, s);
    }
    else if (debug_flag->answer)
	G_debug(debug_level, "%s", message->answer);
    else if (important->answer)
	G_important_message("%s", message->answer);
    else if (verbose->answer)
	G_verbose_message("%s", message->answer);
    else
	G_message("%s", message->answer);

    exit(EXIT_SUCCESS);
}
