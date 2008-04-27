/*  
 ****************************************************************************
 *
 * MODULE:       g.message 
 * AUTHOR(S):    Jachym Cepicky - jachym AT les-ejk cz
 *               Hamish Bowman - hamish_nospam AT yahoo com
 * PURPOSE:      Provides a means of reporting the contents of GRASS
 *               projection information files and creating
 *               new projection information files.
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
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
    struct Flag *warning, *fatal, *debug_flag, *verbose, *important;
    struct Option *message, *debug_opt;
    struct GModule *module;
    int debug_level;
   
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("general");
    module->label =
	_("Prints a message, warning, or fatal error the GRASS way.");
    module->description =
	_("This module should be used in scripts for messages served to user.");

    warning = G_define_flag();
    warning->key = 'w';
    warning->guisection = "Input";
    warning->description =
	_("Print message as GRASS warning");

    fatal = G_define_flag();
    fatal->key = 'e';
    fatal->guisection = "Input";
    fatal->description =
	_("Print message as GRASS fatal error");

    debug_flag = G_define_flag();
    debug_flag->key = 'd';
    debug_flag->guisection = "Input";
    debug_flag->description =
	_("Print message as GRASS debug message");

    important = G_define_flag();
    important->key = 'i';
    important->guisection = "Input";
    important->description =
	_("Print message in all but full quiet mode");

    verbose = G_define_flag();
    verbose->key = 'v';
    verbose->guisection = "Input";
    verbose->description =
	_("Print message only if in verbose mode");

    message = G_define_option();
    message->key = "message";
    message->type = TYPE_STRING;
    message->key_desc = "string";
    message->required = YES;
    message->guisection = "Input";
    message->description = _("Text of the message to be printed");

    debug_opt = G_define_option();
    debug_opt->key         = "debug";
    debug_opt->type        = TYPE_INTEGER;
    debug_opt->required    = NO;
    debug_opt->guisection  = "Input";
    debug_opt->answer      = "1";
    debug_opt->options     = "0-5";
    debug_opt->description = _("Level to use for debug messages");
 

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    if(fatal->answer + warning->answer + debug_flag->answer + verbose->answer > 1)
	G_fatal_error(_("Select only one message level."));

    debug_level = atoi(debug_opt->answer);


    if(fatal->answer)
	G_fatal_error(message->answer);

    else if(warning->answer)
	G_warning(message->answer);

    else if(debug_flag->answer)
	G_debug(debug_level, message->answer);

    else if(verbose->answer)
	G_verbose_message(message->answer);

    else if(important->answer)
	G_important_message(message->answer);

    else
        G_message(message->answer);
   

    exit(EXIT_SUCCESS);
}
