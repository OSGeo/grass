#ifndef GRASS_MSVC_SYS_STAT_H
#define GRASS_MSVC_SYS_STAT_H

#include <../ucrt/sys/stat.h>

#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)

#define S_IRUSR       _S_IREAD
#define S_IWUSR       _S_IWRITE

#endif
