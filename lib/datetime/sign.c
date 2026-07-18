/*
 * SPDX-FileCopyrightText: 1995 Bill Brown <brown@gis.uiuc.edu>
 * SPDX-FileCopyrightText: 1995 Michael Shapiro
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <grass/datetime.h>

/*!
 * \brief
 *
 * Returns:
 * 1 if the Datetime is positive
 * 0 otherwise
 *
 *  \param dt
 *  \return int
 */
int datetime_is_positive(const DateTime *dt)
{
    return dt->positive != 0;
}

/*!
 * \brief
 *
 * Returns:
 * 1 if the DateTime is negative
 * 0 otherwise
 *
 *  \param dt
 *  \return int
 */
int datetime_is_negative(const DateTime *dt)
{
    return dt->positive == 0;
}

/*!
 * \brief
 *
 * Makes the DateTime positive. (A.D. for ABSOLUTE DateTimes)
 *
 *  \param dt
 *  \return void
 */
void datetime_set_positive(DateTime *dt)
{
    dt->positive = 1;
}

/*!
 * \brief
 *
 * Makes the DateTime negative. (B.C. for ABSOLUTE DateTimes)
 *
 *  \param dt
 *  \return void
 */
void datetime_set_negative(DateTime *dt)
{
    dt->positive = 0;
}

/*!
 * \brief
 *
 *  \param dt
 *  \return void
 */
void datetime_invert_sign(DateTime *dt)
{
    dt->positive = !dt->positive;
}
