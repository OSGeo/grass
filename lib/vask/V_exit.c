
/**
 * \file V_exit.c
 *
 * \brief Interactive exit functions.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <grass/config.h>
#include <stdio.h>
#include <grass/vask.h>


/**
 * \fn int V_exit ()
 *
 * \brief Erases the current screen and flushes the current curses setup.
 *
 * \return always returns 0
 */

void V_exit(void)
{
#ifdef HAVE_KEYPAD
    keypad(stdscr, 0);
#endif
    clear();
    refresh();

    /* added for Mips' braindead implementation of curses 
     * and the ordering is important
     */
    echo();
    nl();
    noraw();

    endwin();
    fflush(stdout);
    fflush(stderr);
    fflush(stdin);

    /* Added 17 Sep 1990  dpg.  is a hack we have been using on Sys V 
     * machines it is not the correct way, but it seems to do the job.
     * Fixes the problem with prompts not being displayed after 
     * exitting curses. */
#ifdef SYSV
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
#endif
}
