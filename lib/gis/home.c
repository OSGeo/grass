/*
 ****************************************************************
 * char *
 * G_home ()
 *
 *   returns char pointer to home directory for user
 *   dies if can't determine
 *
 * char *
 * G__home()
 *
 *   returns char pointer to home directory for user
 *   NULL if can't determine
 *
 ***************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/*!
 * \brief user's home directory
 *
 * Returns a pointer to a string
 * which is the full path name of the user's home directory.
 *
 *  \param ~
 *  \return char * 
 */

char *
G_home (void)
{
    char *home = G__home();

    if (home)
	return home;
    
    G_fatal_error(_("unable to determine user's home directory"));
    return NULL;
}

char *
G__home (void)
{
    static char *home = 0;

    if (home)
        return home;

#ifdef __MINGW32__
    { 
	char buf[GPATH_MAX];

	/* TODO: we should probably check if the dir exists */
	home = getenv ( "USERPROFILE" ) ;

        if ( !home ) 
        {
	    sprintf ( buf, "%s%s", getenv ( "HOMEDRIVE" ), 
				   getenv ( "HOMEPATH" ) );

	    if ( strlen(buf) >= 0 )
		home = G_store ( buf );
        }

	if ( !home )
	    home = getenv ( "HOME" ) ;
    }
#else
    home = getenv("HOME");
#endif
    G_debug (2, "G__home home = %s", home );
    return home;
}
