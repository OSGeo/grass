/*  Routines to manage the graphics window contents list
 *
 * D_set_erase_color(color)
 *     sets the color name of the current erase color for the window
 *
 * D_get_erase_color(color)
 *     returns the current erase color name for window
 *
 */

#include <string.h>
#include <stdio.h>
#include <grass/display.h>
#include <grass/raster.h>


/*!
 * \brief clears information about current frame
 *
 * Removes all information about the current frame. This includes the map region and the
 * frame content lists.
 *
 *  \param ~
 *  \return int
 */

void D_clear_window(void)
{
}

static char *erase_color;

void D_set_erase_color(const char *colorname)
{
    if (erase_color)
	G_free(erase_color);

    erase_color = G_store(colorname);
}


void D_get_erase_color(char *colorname)
{
    if (!erase_color)
	erase_color = G_store(DEFAULT_BG_COLOR);

    strcpy(colorname, erase_color);
}
