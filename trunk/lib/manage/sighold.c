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
    RETSIGTYPE (*sig)() = hold ? SIG_IGN : SIG_DFL;

    signal(SIGINT, sig);

#ifndef __MINGW32__
    signal(SIGQUIT, sig);
#endif

#ifdef SIGTSTP
    signal(SIGTSTP, sig);
#endif

    return 0;
}
