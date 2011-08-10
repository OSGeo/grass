#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <grass/gis.h>

#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  This function ignores the error.
 *
 *  \param 
 *  \return void
 */

void G3d_skipError(const char *msg)
{
}


/*!
 * \brief 
 *
 *  This function prints the
 * error message <em>msg</em> to <em>stderr</em> and returns.
 *
 *  \param 
 *  \return void
 */

void G3d_printError(const char *msg)
{
    fprintf(stderr, "ERROR: ");
    fprintf(stderr, msg);
    fprintf(stderr, "\n");
}


/*!
 * \brief 
 *
 *  This function prints the
 * error message <em>msg</em>, and terminates the program with an error status.
 *
 *  \param 
 *  \return void
 */

void G3d_fatalError(const char *msg, ...)
{
    char buffer[2000];		/* No novels to the error logs, OK? */
    va_list ap;

    va_start(ap, msg);
    vsprintf(buffer, msg, ap);
    va_end(ap);

    G_fatal_error("%s", buffer);
}

void G3d_fatalError_noargs(const char *msg)
{
    G_fatal_error("%s", msg);
}

void G3d_error(const char *msg, ...)
{
    char buffer[2000];		/* No novels to the error logs, OK? */
    va_list ap;

    va_start(ap, msg);
    vsprintf(buffer, msg, ap);
    va_end(ap);

    (*g3d_error_fun) (buffer);
}
