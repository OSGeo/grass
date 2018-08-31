/*!
 * \file lib/gis/debug.c
 *
 * \brief GIS Library - Debug functions.
 *
 * (C) 2001-2012 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static int initialized;
static int grass_debug_level;

/**
 * \brief Initiate debugging.
 */
void G_init_debug(void)
{
    const char *lstr;

    if (G_is_initialized(&initialized))
	return;

    lstr = G_getenv_nofatal("DEBUG");

    if (lstr != NULL)
	grass_debug_level = atoi(lstr);
    else
	grass_debug_level = 0;

    G_initialize_done(&initialized);
}

/**
 * \brief Print debugging message.
 *
 * Print debugging message if environment variable GRASS_DEBUG_LEVEL
 * is set to level equal or greater  
 *
 * Levels: (recommended levels)<br>
 *  - 1 - message is printed once or twice per module<br>
 *  - 2 - less interesting once-per-module messages,<br>
 *  - 2 - library functions likely to be used once in a module<br>
 *  - 3 - library functions likely to be called a few times in a module (<=10),<br>
 *  - 3 - database opening and closing logistics<br>
 *  - 4 - each row (raster) or line (vector) or database/column (DB),<br>
 *  - 4 - each column/cat (DB)<br>
 *  - 5 - each cell (raster) or point (vector) or cat/attribute (DB)
 *
 * \param[in] level level
 * \param[in] msg message
 * \return 0 on error
 * \return 1 on success
 */
int G_debug(int level, const char *msg, ...)
{
    char *filen;
    va_list ap;
    FILE *fd;

    G_init_debug();

    if (grass_debug_level >= level) {
	va_start(ap, msg);

	filen = getenv("GRASS_DEBUG_FILE");
	if (filen != NULL) {
	    fd = fopen(filen, "a");
	    if (!fd) {
		G_warning(_("Cannot open debug file '%s'"), filen);
		return 0;
	    }
	}
	else {
	    fd = stderr;
	}

	fprintf(fd, "D%d/%d: ", level, grass_debug_level);
	vfprintf(fd, msg, ap);
	fprintf(fd, "\n");
	fflush(fd);

	if (filen != NULL)
	    fclose(fd);

	va_end(ap);
    }

    return 1;
}

