
/***************************************************************************
 *            error.c
 *
 *  Mon Apr 18 15:00:13 2005
 *  Copyright  2005  Benjamin Ducke
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdarg.h>

#include "globals.h"

/*
   Displays an error message
 */
void print_error(int err_code, char *msg, ...)
{
    char buffer[MAXSTR];
    va_list ap;

    va_start(ap, msg);
    vsprintf(buffer, msg, ap);
    va_end(ap);

    fprintf(stderr, "\033[1;31m\nERROR:\033[0m %s", buffer);

    ERROR = err_code;

    exit(err_code);
}


/*
   Displays a warning message
 */
void print_warning(char *msg, ...)
{
    char buffer[MAXSTR];
    va_list ap;

    va_start(ap, msg);
    vsprintf(buffer, msg, ap);
    va_end(ap);

    fprintf(stderr, "\033[0;33m\nWARNING:\033[0m %s", buffer);

    WARNINGS = WARNINGS + 1;
}


/*
   Prints a fancy "DONE." on the screen
 */
void print_done(void)
{
    fprintf(stdout, "\033[0;32mDONE.\n\033[0m");
}
