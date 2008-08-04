#include <signal.h>
#include <grass/config.h>
#include "globals.h"
#include "local_proto.h"

static RETSIGTYPE do_quit(int sig)
{
    quit();
}

static RETSIGTYPE sigint(int sig)
{
    signal(sig, sigint);
    signalflag.interrupt = sig;
}

int set_signals(void)
{
    /* set the ctrlz catch 
       signal (SIGTSTP, ctrlz);
     */
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);	/* ignore ctrl-Z */
#endif

    /* set other signal catches */
    signalflag.interrupt = 0;
    signal(SIGINT, sigint);

    signal(SIGTERM, do_quit);

    return 0;
}
