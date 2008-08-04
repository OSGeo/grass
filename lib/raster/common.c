
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/graphics.h>

/*!
 * \brief flush graphics
 *
 * Send all pending graphics commands to the graphics driver.
 * This is done automatically when graphics input requests are made.
 * Generally this is only needed for interactive graphics.
 *
 *  \param void
 *  \return int
 */

void R_flush(void)
{
    R_stabilize();
}

void R_pad_perror(const char *msg, int code)
{
    const char *err;

    switch (code) {
    case OK:
	err = "";
	break;
    case NO_CUR_PAD:
	err = "no current pad";
	break;
    case NO_PAD:
	err = "pad not found";
	break;
    case NO_MEMORY:
	err = "out of memory";
	break;
    case NO_ITEM:
	err = "item not found";
	break;
    case ILLEGAL:
	err = "illegal request";
	break;
    case DUPLICATE:
	err = "duplicate name";
	break;
    default:
	err = "unknown error";
	break;
    }

    fprintf(stderr, "%s%s%s\n", msg, *msg ? " : " : "", err);
}

void R_pad_freelist(char **list, int count)
{
    int i;

    if (count <= 0)
	return;

    for (i = 0; i < count; i++)
	G_free(list[i]);

    G_free(list);
}
