#include <unistd.h>
#include <signal.h>

#include <grass/calc.h>

/****************************************************************************/

volatile int floating_point_exception;
volatile int floating_point_exception_occurred;

int columns;

/****************************************************************************/

static void handle_fpe(int n UNUSED)
{
    floating_point_exception = 1;
    floating_point_exception_occurred = 1;
}

void pre_exec(void)
{
#ifndef _WIN32
#ifdef SIGFPE
    struct sigaction act;

    act.sa_handler = &handle_fpe;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(SIGFPE, &act, NULL);
#endif
#endif

    floating_point_exception_occurred = 0;
}

void post_exec(void)
{
#ifndef _WIN32
#ifdef SIGFPE
    struct sigaction act;

    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(SIGFPE, &act, NULL);
#endif
#endif
}

/****************************************************************************/

void calc_init(int cols)
{
    columns = cols;
}

/****************************************************************************/
