#include <grass/config.h>
#ifndef __MINGW32__
#include <unistd.h>
#endif
#ifdef __MINGW32__
#include <windows.h>
#endif
#include <grass/gis.h>

/* Sleep */
void G_sleep(unsigned int seconds)
{
#ifdef __MINGW32__
    /* note: Sleep() cannot be interrupted */
    Sleep((seconds) * 1000);
#else
    sleep(seconds);
#endif
}
