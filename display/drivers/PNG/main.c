
/****************************************************************************
 *
 * MODULE:       PNG driver
 * AUTHOR(S):    Johansen Per Henrik <per.henrik.johansen norgit.no> 
 *                      (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Andreas Lange <andreas.lange rhein-main.de>,
 *               Huidae Cho <grass4u gmail.com>,
 *               Cedric Shock <cedricgrass shockfamily.net>
 *               Hamish Bowman <hamish_nospam yahoo.com>
 * PURPOSE:      driver to create PNG and PPM images
 * COPYRIGHT:    (C) 2001-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include "pngdriver.h"

int main(int argc, char **argv)
{
    LIB_init(PNG_Driver(), argc, argv);

    return LIB_main(argc, argv);
}
