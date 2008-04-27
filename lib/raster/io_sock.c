
#include <grass/config.h>

#ifdef HAVE_SOCKET

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef __MINGW32__
#include <winsock2.h>
#define ECONNREFUSED WSAECONNREFUSED
#define EADDRINUSE   WSAEADDRINUSE  
#define ENOTSOCK     WSAENOTSOCK    
#define ETIMEDOUT    WSAETIMEDOUT   
#endif

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>

#include "open.h"

extern int _rfd, _wfd;
extern int _quiet;

extern int sync_driver(char *);

static char *sockpath;

/*!
 * \brief initialize graphics
 *
 * Initializes connection to
 * current graphics driver. Refer to GRASS User's Manual entries on the
 * <i>d.mon</i> command. If connection cannot be made, the application module
 * sends a message to the user stating that a driver has not been selected or
 * could not be opened. Note that only one application module can be connected to
 * a graphics driver at once.
 * After all graphics have been completed, the driver should be closed.
 *
 *  \param void
 *  \return int
 */

int REM_open_driver(void)
{
    int verbose;
    char *name;

    verbose = !_quiet;
    _quiet = 0;

    name = getenv("MONITOR_OVERRIDE");
    if (!name)
	name = G__getenv("MONITOR");

    if (!name)
    {
        if (verbose)
        {
            G_warning(_("No graphics monitor has been selected for output."));
            G_warning(_("Please run \"d.mon\" to select a graphics monitor."));
        }
        return(NO_MON);
    }

    /* Get the full path to the unix socket */
    if ((sockpath = G_sock_get_fname(name)) == NULL)
    {
        if (verbose)
            G_warning(_("Failed to get socket name for monitor <%s>."), name);
        return (NO_MON);
    }

    /* See if the socket exists, if it doesn't no point in trying to
     * connect to it.
     */
    if (!G_sock_exists(sockpath))
    {
        if (verbose)
            G_warning(_("No socket to connect to for monitor <%s>."), name);
        return (NO_MON);
    }

    /** We try to make a connection now **/

    _wfd = G_sock_connect(sockpath);
    if (_wfd > 0) /* success */
    {
	_rfd = dup(_wfd);
	sync_driver(name);
	return (OK);
    }

    switch (errno)
    {
    case ECONNREFUSED:
    case EADDRINUSE:
	if (verbose)
	{
	    G_warning(_("Socket is already in use or not accepting connections."));
	    G_warning(_("Use d.mon to select a monitor"));
	}
	return (NO_RUN);
    case EBADF:
    case ENOTSOCK:
	if (verbose)
	{
	    G_warning(_("Trying to connect to something not a socket."));
	    G_warning(_("Probably program error."));
	}
	return (NO_RUN);
    case ETIMEDOUT:
	if (verbose)
	{
	    G_warning(_("Connect attempt timed out."));
	    G_warning(_("Probably an error with the server."));
	}
	return (NO_RUN);
    default:
	break;
    }

    if (verbose)
	G_warning(_("Connection failed."));
            
    /* We couldn't connect... */
    return (NO_RUN);
}

#endif /* HAVE_SOCKET */
