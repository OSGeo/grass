#ifndef GRASS_MSVC_UNISTD_H
#define GRASS_MSVC_UNISTD_H

#include <windows.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <time.h>
#include <synchapi.h>

#define access _access
#define close  _close
#define dup    _dup
#define dup2   _dup2
#define unlink _unlink
#define rmdir  _rmdir
#define getcwd _getcwd
#define chdir  _chdir
#define isatty _isatty
#define getpid _getpid

static inline int usleep(useconds_t usec)
{
    Sleep(usec / 1000); /* convert microseconds to milliseconds */
    return 0;
}

static inline unsigned int sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}

#endif
