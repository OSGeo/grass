
/****************************************************************************
 *
 * MODULE:       Cairo driver
 * AUTHOR(S):    Lars Ahlzen <lars@ahlzen.com>
 * PURPOSE:      driver to create bitmap or vector files from display commands
 *               using the Cairo graphics library
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include "cairodriver.h"

int main(int argc, char **argv)
{
    LIB_init(Cairo_Driver(), argc, argv);

    return LIB_main(argc, argv);
}
