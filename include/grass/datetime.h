#ifndef GRASS_DATETIME_H
#define GRASS_DATETIME_H

#define DATETIME_ABSOLUTE 1
#define DATETIME_RELATIVE 2

/* ranges - the values must start at 101 and increase 
 * to make sure they do not interfere with the spatial
 * units in gis.h */
#define DATETIME_YEAR   101
#define DATETIME_MONTH  102
#define DATETIME_DAY    103
#define DATETIME_HOUR   104
#define DATETIME_MINUTE 105
#define DATETIME_SECOND 106

typedef struct DateTime
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
#include <grass/defs/datetime.h>

#endif
