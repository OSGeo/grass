/* Header file: border.h
 *
 *  AUTHOR:      M. Hamish Bowman, New Zealand  February 2007
 *
 *  COPYRIGHT:   (c) 2007 Hamish Bowman, and the GRASS Development Team
 *             This program is free software under the GNU General Public
 *             License (>=v2). Read the file COPYING that comes with GRASS
 *             for details.
 */

struct border
{
    double r, g, b;
    double width;
};

extern struct border brd;
