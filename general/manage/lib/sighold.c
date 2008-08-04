#include "signal.h"

void (*sig) ();

int hold_signals(int hold)
{

    sig = hold ? SIG_IGN : SIG_DFL;

    signal(SIGINT, sig);

#ifndef __MINGW32__
    signal(SIGQUIT, sig);
#endif

#ifdef SIGTSTP
    signal(SIGTSTP, sig);
#endif

    return 0;
}
