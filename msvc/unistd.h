#ifndef GRASS_MSVC_UNISTD_H
#define GRASS_MSVC_UNISTD_H

#include <io.h>
#define access _access
#define close  _close
#define dup    _dup
#define dup2   _dup2
#define unlink _unlink
#define isatty _isatty

#include <direct.h>
#define rmdir  _rmdir
#define getcwd _getcwd
#define chdir  _chdir

#include <process.h>
#define getpid _getpid

#endif
