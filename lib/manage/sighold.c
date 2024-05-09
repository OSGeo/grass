/*!
   \file lib/manage/sighold.c

   \brief Manage Library - Hold signals

   (C) 2001-2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

#include <signal.h>
#include <grass/config.h>

/*!
   \brief Hold signals

   \param hold

   \return 0
 */
int M__hold_signals(int hold)
{
<<<<<<< HEAD
    void (*sig)(int) = hold ? SIG_IGN : SIG_DFL;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    void (*sig)(int) = hold ? SIG_IGN : SIG_DFL;
=======
    void (*sig)() = hold ? SIG_IGN : SIG_DFL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    void (*sig)() = hold ? SIG_IGN : SIG_DFL;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    void (*sig)(int) = hold ? SIG_IGN : SIG_DFL;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    void (*sig)() = hold ? SIG_IGN : SIG_DFL;
=======
    void (*sig)(int) = hold ? SIG_IGN : SIG_DFL;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main

    signal(SIGINT, sig);

#ifndef __MINGW32__
    signal(SIGQUIT, sig);
#endif

#ifdef SIGTSTP
    signal(SIGTSTP, sig);
#endif

    return 0;
}
