#include <signal.h>
#include <grass/config.h>

int hold_signals(int hold)
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
