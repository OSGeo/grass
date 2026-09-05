#ifndef GRASS_MSVC_SYS_TIME_H
#define GRASS_MSVC_SYS_TIME_H

#include <winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif

int gettimeofday(struct timeval *, struct timezone *);

#ifdef __cplusplus
}
#endif

#endif
