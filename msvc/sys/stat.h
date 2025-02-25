#ifndef GRASS_MSVC_SYS_STAT_H
#define GRASS_MSVC_SYS_STAT_H

#include <../ucrt/sys/stat.h>

#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)

#endif
