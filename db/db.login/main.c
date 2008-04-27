/****************************************************************************
 *
 * MODULE:       db.login
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2004-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <grass/config.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

int
main(int argc, char *argv[])
{
    struct Option *driver, *database, *user, *password;
    struct GModule *module;
#ifdef HAVE_TERMIOS_H
    struct termios tios, tios2;
#endif
    char answer[200];
    
    /* Initialize the GIS calls */
    G_gisinit(argv[0]) ;

    module              = G_define_module();
    module->keywords    = _("database, SQL");
    module->description = _("Sets user/password for driver/database.");
    
    driver             = G_define_standard_option(G_OPT_DRIVER) ;
    driver->options    = db_list_drivers();
    driver->required   = YES;
    driver->answer     = db_get_default_driver_name();

    database             = G_define_standard_option(G_OPT_DATABASE) ;
    database->required   = YES ;
    database->answer     = db_get_default_database_name();

    user             = G_define_option() ;
    user->key        = "user" ;
    user->type       = TYPE_STRING ;
    user->required   = NO  ;
    user->multiple   = NO ;
    user->description= _("Username") ;

    password             = G_define_option() ;
    password->key        = "password" ;
    password->type       = TYPE_STRING ;
    password->required   = NO ;
    password->multiple   = NO ;
    password->description= _("Password") ;

    if(G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* set connection */
    if (!password->answer && isatty(fileno(stdin)) ){
        for(;;) {
#ifdef HAVE_TERMIOS_H
          tcgetattr(STDIN_FILENO, &tios);
          tios2 = tios;
          tios2.c_lflag &= ~ECHO;
          tcsetattr(STDIN_FILENO, TCSAFLUSH, &tios2);
#endif
          do {
              fprintf (stderr,_("\nEnter database password for connection\n<%s:%s:user=%s>\n"), driver->answer, database->answer, user->answer);
              fprintf (stderr, _("Hit RETURN to cancel request\n"));
              fprintf (stderr,">");
          } while(!G_gets(answer));
#ifdef HAVE_TERMIOS_H
          tcsetattr(STDIN_FILENO, TCSANOW, &tios);
#endif
          G_strip(answer);
          if(strlen(answer)==0) {
	     G_message(_("Exiting. Not changing current settings"));
	     return -1;
	  } else {
	     G_message(_("New password set"));
	     password->answer = G_store(answer);
	     break;
	  }
	}
    }
    if (  db_set_login ( driver->answer, database->answer, user->answer, password->answer ) == DB_FAILED ) {
	G_fatal_error ( _("Unable to set user/password") );
    }

    if ( password->answer )
        G_warning ( _("The password was stored in file") );
	
    exit(EXIT_SUCCESS);
}
