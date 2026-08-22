/*!
 * \file lib/gis/endian.c
 *
 * \brief GIS Library - Functions to determine architecture endian.
 *
 * This endian test was taken from ./src.contrib/GMSL/NVIZ2.2/TOGL/apps/image.c.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
*
 * \author Markus Neteler
 */

/*!
 * \brief Tests for little ENDIAN.
 *
 * Test if machine is little or big endian.
 *
 * \return 1 little endian
 * \return 0 big endian
 */
int G_is_little_endian(void)
{
    union {
        int testWord;
        char testByte[sizeof(int)];
    } endianTest;

    endianTest.testWord = 1;

    if (endianTest.testByte[0] == 1)
        return 1; /* true: little endian */

    return 0; /* false: big endian */
}
