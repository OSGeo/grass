/* MSVC does not have strings.h */
#ifndef GRASS_MSVC_STRINGS_H
#define GRASS_MSVC_STRINGS_H

#include <../ucrt/string.h>

#define strncasecmp _strnicmp
#define strcasecmp  _stricmp

#endif
