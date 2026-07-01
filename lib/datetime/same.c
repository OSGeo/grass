/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <string.h>
#include <grass/datetime.h>

/*!
 * \brief
 *
 * Returns:
 * 1 if 'src' is exactly the same as 'dst'
 * 0 if they differ
 *
 *  \param src
 *  \param dst
 *  \return int
 */
int datetime_is_same(const DateTime *src, const DateTime *dst)
{
    /* Compare field-by-field (DateTime may contain padding) */
    return src->year == dst->year &&
        src->month == dst->month &&
        src->day == dst->day &&
        src->hour == dst->hour &&
        src->minute == dst->minute &&
        src->second == dst->second &&
        src->usec == dst->usec &&
        src->timezone == dst->timezone;

}
