
/****************************************************************************
 *
 * MODULE:       PostScript driver
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com>
 *                      (original contributor)
 *               Markus Neteler <neteler itc.it> 
 * PURPOSE:      driver to create PostScript files from display commands
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include "psdriver.h"

int main(int argc, char **argv)
{
    LIB_init(PS_Driver(), argc, argv);

    return LIB_main(argc, argv);
}
