
/****************************************************************************
 *
 * MODULE:       db.login
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      Store db login settings
 * COPYRIGHT:    (C) 2004-2009, 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
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

int main(int argc, char *argv[])
{
    struct Option *driver, *database, *user, *password;
    struct GModule *module;
    
    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("connection settings"));
    module->description = _("Sets user/password for DB driver/database.");

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    driver->required = YES;
    driver->answer = (char *) db_get_default_driver_name();

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    database->required = YES;
    database->answer = (char *) db_get_default_database_name();

    user = G_define_option();
    user->key = "user";
    user->type = TYPE_STRING;
    user->required = NO;
    user->multiple = NO;
    user->description = _("Username");
    user->guisection = _("Settings");
    
    password = G_define_option();
    password->key = "password";
    password->type = TYPE_STRING;
    password->required = NO;
    password->multiple = NO;
    password->description = _("Password");
    password->guisection = _("Settings");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (db_set_login(driver->answer, database->answer, user->answer,
                     password->answer) == DB_FAILED) {
        G_fatal_error(_("Unable to set user/password"));
    }
    
    if (password->answer)
        G_important_message(_("The password was stored in file (%s/dblogin)"), CONFIG_DIR);
    
    exit(EXIT_SUCCESS);
}
