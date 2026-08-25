#ifndef GRASS_MSVC_UNISTD_H
#define GRASS_MSVC_UNISTD_H

#include <io.h>
#define read   _read
#define write  _write
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
#define getpid        _getpid

#define F_OK          0 /* Test for existence. */
#define X_OK          1 /* Test for execute permission. */
#define W_OK          2 /* Test for write permission. */
#define R_OK          4 /* Test for read permission. */

#define STDIN_FILENO  0 /* Standard input. */
#define STDOUT_FILENO 1 /* Standard output. */
#define STDERR_FILENO 2 /* Standard error output. */

#endif
