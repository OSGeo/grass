#include <signal.h>
#include "globals.h"

void sigalarm(int n)
{
    signal(n, sigalarm);
    signalflag.alarm = n;
}
