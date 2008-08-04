#ifndef GRASS_DATETIME_H
#define GRASS_DATETIME_H

#define DATETIME_ABSOLUTE 1
#define DATETIME_RELATIVE 2

/* ranges - the values must start at 1 and increase */
#define DATETIME_YEAR   1
#define DATETIME_MONTH  2
#define DATETIME_DAY    3
#define DATETIME_HOUR   4
#define DATETIME_MINUTE 5
#define DATETIME_SECOND 6

typedef struct
{
    int mode;			/* absolute or relative */
    int from, to;
    int fracsec;		/* #decimal place in printed seconds */
    int year, month, day;
    int hour, minute;
    double second;
    int positive;
    int tz;			/* timezone - minutes from UTC */
} DateTime;

/* prototype of functions */
#include <grass/P_datetime.h>

#endif
