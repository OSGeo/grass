#ifndef GRASS_SPAWN_H
#define GRASS_SPAWN_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define SF_MODE_IN	((const char *) (O_RDONLY))
#define SF_MODE_OUT	((const char *) (O_WRONLY|O_CREAT|O_TRUNC))
#define SF_MODE_APPEND	((const char *) (O_WRONLY|O_CREAT|O_APPEND))

#define SF_STDIN	((const char *) STDIN_FILENO)
#define SF_STDOUT	((const char *) STDOUT_FILENO)
#define SF_STDERR	((const char *) STDERR_FILENO)

#define SF_REDIRECT_FILE		((const char *) 1)
#define SF_REDIRECT_DESCRIPTOR		((const char *) 2)
#define SF_CLOSE_DESCRIPTOR		((const char *) 3)
#define SF_SIGNAL			((const char *) 4)
#define SF_VARIABLE			((const char *) 5)
#define SF_BINDING			((const char *) 6)
#define SF_BACKGROUND			((const char *) 7)
#define SF_DIRECTORY			((const char *) 8)
#define SF_ARGVEC			((const char *) 9)

enum signal_action
{
    SSA_NONE,
    SSA_IGNORE,
    SSA_DEFAULT,
    SSA_BLOCK,
    SSA_UNBLOCK,
};

enum signal_type
{
    SST_PRE,
    SST_POST,
    SST_CHILD,
};

#include <grass/defs/spawn.h>

#endif
