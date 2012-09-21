#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <grass/gis.h>

#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/


/*!
 * \brief This function ignores the error.
 *
 *  \param msg
 *  \return void
 */

void Rast3d_skip_error(const char *msg)
{
}


/*!
 * \brief Prints error message
 *
 *  This function prints the
 *  error message <em>msg</em> to <em>stderr</em> and returns.
 *
 *  \param msg
 *  \return void
 */

void Rast3d_print_error(const char *msg)
{
    fprintf(stderr, "ERROR: ");
    fprintf(stderr, "%s", msg);
    fprintf(stderr, "\n");
}


/*!
 * \brief Prints fatal error message
 *
 *  This function prints the fatal
 *  error message <em>msg</em>, and terminates the program with an error status.
 *
 *  \param msg
 *  \return void
 */

void Rast3d_fatal_error(const char *msg, ...)
{
    char buffer[2000];		/* No novels to the error logs, OK? */
    va_list ap;

    va_start(ap, msg);
    vsprintf(buffer, msg, ap);
    va_end(ap);

    G_fatal_error("%s", buffer);
}

void Rast3d_fatal_error_noargs(const char *msg)
{
    G_fatal_error("%s", msg);
}

void Rast3d_error(const char *msg, ...)
{
    char buffer[2000];		/* No novels to the error logs, OK? */
    va_list ap;

    va_start(ap, msg);
    vsprintf(buffer, msg, ap);
    va_end(ap);

    (*g3d_error_fun) (buffer);
}
