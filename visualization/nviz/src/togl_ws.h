#ifndef TOGL_WS_H
#define TOGL_WS_H

#include <grass/config.h>

/* define windowing system togl is compiled with */
#if defined(OPENGL_X11)
# define TOGL_X11
#elif defined(OPENGL_AQUA)
# define TOGL_AGL
#elif defined(OPENGL_WINDOWS)
# define TOGL_WGL
#else
# error None of OPENGL_X11, OPENGL_AQUA or OPENGL_WINDOWS defined
#endif

#endif
