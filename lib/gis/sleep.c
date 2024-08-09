#include <grass/config.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include <grass/gis.h>

/* Sleep */
void G_sleep(unsigned int seconds)
{
#ifdef _WIN32
    /* note: Sleep() cannot be interrupted */
    Sleep((seconds) * 1000);
#else
    sleep(seconds);
#endif
}
