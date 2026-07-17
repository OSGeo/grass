/*!
   \file lib/manage/sighold.c

   \brief Manage Library - Hold signals

   SPDX-FileCopyrightText: 2001-2011 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

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
    void (*sig)(int) = hold ? SIG_IGN : SIG_DFL;

    signal(SIGINT, sig);

#ifndef _WIN32
    signal(SIGQUIT, sig);
#endif

#ifdef SIGTSTP
    signal(SIGTSTP, sig);
#endif

    return 0;
}
