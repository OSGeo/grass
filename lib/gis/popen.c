#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <grass/gis.h>

#ifdef __MINGW32__
#define popen(cmd,mode) _popen(cmd,mode)
#define pclose(fp) _pclose(fp)
#endif

FILE *G_popen(const char *cmd, const char *mode)
{
    return popen(cmd, mode);
}

int G_pclose(FILE *ptr)
{
    return pclose(ptr);
}
