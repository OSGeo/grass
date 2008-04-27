/**
 * \file endian.c
 *
 * \brief Functions to determine architecture endian.
 *
 * This endian test was taken from ./src.contrib/GMSL/NVIZ2.2/TOGL/apps/image.c.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Markus Neteler
 *
 * \date 2001-2006
 */


/**
 * \fn int G_is_little_endian (void)
 *
 * \brief Tests for little ENDIAN.
 *
 * Test if machine is little or big endian.
 *
 * \return 1 little endian
 * \return 0 big endian
 */

int G_is_little_endian (void)    
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
