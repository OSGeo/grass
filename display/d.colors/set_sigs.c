
#include "externs.h"
#include <signal.h>
#include <grass/config.h>

struct signalflag signalflag;

extern RETSIGTYPE sigint(int);

int set_signals(void)
{

    /* ignore ctrlz */

#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
#endif

    /* set other signal catches */

    signalflag.interrupt = 0;

    signal(SIGINT, sigint);

    return 0;
}
