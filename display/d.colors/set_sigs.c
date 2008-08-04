#define SIG_MAIN
#include "externs.h"
#include <signal.h>

void sigint();

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
