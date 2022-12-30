#include <unistd.h>
#include <signal.h>

#include <grass/calc.h>

/****************************************************************************/

volatile int floating_point_exception;
volatile int floating_point_exception_occurred;

int columns;

/****************************************************************************/

<<<<<<< HEAD
<<<<<<< HEAD
static void handle_fpe(int n UNUSED)
=======
static void handle_fpe(int n)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
static void handle_fpe(int n)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
{
    floating_point_exception = 1;
    floating_point_exception_occurred = 1;
}

void pre_exec(void)
{
#ifndef __MINGW32__
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
#ifndef __MINGW32__
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
