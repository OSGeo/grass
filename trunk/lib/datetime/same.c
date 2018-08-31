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

int datetime_is_same(const DateTime * src, const DateTime * dst)
{
    /* WARNING: doesn't allow for padding */
    return memcmp(src, dst, sizeof(DateTime)) == 0;
}
