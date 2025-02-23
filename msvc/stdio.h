#ifndef GRASS_MSVC_STDIO_H
#define GRASS_MSVC_STDIO_H

#include <../ucrt/stdio.h>

/* if not defined by another GRASS header */
#ifndef ssize_t_defined
#define ssize_t_defined
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#endif
