#include <signal.h>
#include "externs.h"

void sigint(int n)
{
    signal(n, sigint);
    signalflag.interrupt = n;
}
