#ifndef GRASS_MSVC_STDIO_H
#define GRASS_MSVC_STDIO_H

#include <../ucrt/stdio.h>

#define fdopen _fdopen

#include <BaseTsd.h>
typedef SSIZE_T ssize_t;

#endif
