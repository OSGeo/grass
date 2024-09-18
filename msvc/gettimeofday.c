/* Adopted from GPL-3 SDRangel
 * https://github.com/f4exb/sdrangel/blob/master/custom/windows/windows_time.h
 */

/*
 * missing gettimeofday implementation
 * for windows; based on postgresql
 */

#define WIN32_LEAN_AND_MEAN  /* stops windows.h including winsock.h; \
                              * timeval redefine */
#include <Windows.h>
#include <stdint.h> /* portable: uint64_t   MSVC: __int64 */

/* MSVC defines this in winsock2.h!? */
typedef struct timeval {
    long tv_sec;
    long tv_usec;
} timeval;

int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
    /* Note: some broken versions only have 8 trailing zero's, the correct epoch
     * has 9 trailing zero's This magic number is the number of 100 nanosecond
     * intervals since January 1, 1601 (UTC) until 00:00:00 January 1, 1970 */
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME system_time;
    FILETIME file_time;
    uint64_t time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}
