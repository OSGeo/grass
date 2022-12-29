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
 * Copies the DateTime [into/from ???]  src
 *
 *  \param dst
 *  \param src
 *  \return void
 */

void datetime_copy(DateTime * dst, const DateTime * src)
{
    memcpy(dst, src, sizeof(DateTime));
}
