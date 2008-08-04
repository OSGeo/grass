
/**
 * \file overwrite.c
 *
 * \brief GIS Library - Check for overwrite.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team, Martin Landa <landa.martin gmail.com>
 *
 * \date 2007-2008
 */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>

/**
 * \brief Check for overwrite mode
 *
 * Check variables OVERWRITE, GRASS_OVERWRITE and '--o' flag.
 *
 * The parser G_parser() checks if the map already exists in current mapset,
 * we can switch out the check and do it
 * in the module after the parser.
 *
 * \param[in] argc number of arguments
 * \param[in] argv array of arguments
 *
 * \return 1 if overwrite
 * \return 0 if not overwrite
 */

int G_check_overwrite(int argc, char **argv)
{
    char *overstr;
    int overwrite;

    overwrite = 0;
    if ((overstr = G__getenv("OVERWRITE"))) {
	overwrite = atoi(overstr);
    }

    /* check if inherited GRASS_OVERWRITE is 1 */
    if (!overwrite && (overstr = getenv("GRASS_OVERWRITE"))) {
	overwrite = atoi(overstr);
    }

    /* check for --o or --overwrite option */
    if (!overwrite) {
	int i;

	for (i = 0; i < argc; i++) {
	    if (strcmp(argv[i], "--o") == 0 ||
		strcmp(argv[i], "--overwrite") == 0) {
		overwrite = 1;
		break;
	    }
	}
    }

    G__setenv("OVERWRITE", "1");

    return overwrite;
}
