/*!
 * \file lib/raster/set_window.c
 *
 * \brief Raster Library - Set window (map region)
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "../gis/G.h"
#include "R.h"

static void update_window_mappings(void);
static void check_write_window(void);

void Rast__init_window(void)
{
    if (G_is_initialized(&R__.window_set))
        return;

    G__init_window();

    R__.rd_window = G__.window;
    R__.wr_window = G__.window;
    R__.split_window = 0;

    G_initialize_done(&R__.window_set);
}

/*!
 * \brief Establishes 'window' as the current working window.
 *
 * \param window window to become operative window
 */
void Rast_set_window(struct Cell_head *window)
{
    Rast__init();

    if (R__.split_window)
        G_warning(_("Rast_set_window() called while window split"));

    check_write_window();

    G_adjust_Cell_head(window, 0, 0);

    R__.wr_window = *window;
    R__.rd_window = *window;
    R__.split_window = 0;

    update_window_mappings();
}

/*!
   \brief Unset current window
 */
void Rast_unset_window(void)
{
    G_debug(4, "Rast_unset_window()");

    R__.window_set = 0;
}

/*!
 * \brief Establishes 'window' as the current working window for output.
 *
 * \param window window to become operative window
 */
void Rast_set_output_window(struct Cell_head *window)
{
    Rast__init();

    check_write_window();

    G_adjust_Cell_head(window, 0, 0);

    R__.wr_window = *window;
    R__.split_window = 1;

    G_set_window(window);
}

/*!
 * \brief Establishes 'window' as the current working window for input.
 *
 * Any opened cell files has its file-to-window mapping reworked.
 *
 * \param window window to become operative window
 */

void Rast_set_input_window(struct Cell_head *window)
{
    Rast__init();

    G_adjust_Cell_head(window, 0, 0);

    R__.rd_window = *window;
    R__.split_window = 1;

    update_window_mappings();
}

static void update_window_mappings(void)
{
    int i;
    int maskfd;

    /* adjust window, check for valid window */
    /* adjust the real one, not a copy
       G_copy (&twindow, window, sizeof(struct Cell_head));
       window = &twindow;
     */

    /* except for mask raster, cell files open for read must have same
     * projection and zone as new window
     */
    maskfd = R__.auto_mask > 0 ? R__.mask_fd : -1;
    for (i = 0; i < R__.fileinfo_count; i++) {
        struct fileinfo *fcb = &R__.fileinfo[i];

        if (fcb->open_mode == OPEN_OLD) {
            if (fcb->cellhd.zone == R__.rd_window.zone &&
                fcb->cellhd.proj == R__.rd_window.proj)
                continue;
            if (i != maskfd)
                G_fatal_error(_("Rast_set_read_window(): projection/zone "
                                "differs from that of "
                                "currently open raster maps"));
        }
    }

    /* close the mask */
    if (R__.auto_mask > 0) {
        Rast_close(maskfd);
        /* G_free (R__.mask_buf); */
        R__.mask_fd = -1;
        R__.auto_mask = -1; /* turn off masking */
    }

    /* now for each possible open cell file, recreate the window mapping */
    /*
     * also the memory for reading and writing must be reallocated for all
     * opened cell files
     */
    for (i = 0; i < R__.fileinfo_count; i++) {
        struct fileinfo *fcb = &R__.fileinfo[i];

        if (fcb->open_mode != OPEN_OLD &&
            fcb->open_mode != OPEN_NEW_UNCOMPRESSED &&
            fcb->open_mode != OPEN_NEW_COMPRESSED)
            continue;

        if (fcb->open_mode == OPEN_OLD)
            G_fatal_error(_("Input window changed while maps are open for "
                            "read. Map name <%s>"),
                          fcb->name);
    }

    /* turn masking (back) on if necessary */
    Rast__check_for_auto_masking();
}

static void check_write_window(void)
{
    int i;

    for (i = 0; i < R__.fileinfo_count; i++) {
        struct fileinfo *fcb = &R__.fileinfo[i];

        if (fcb->open_mode == OPEN_NEW_UNCOMPRESSED ||
            fcb->open_mode == OPEN_NEW_COMPRESSED)
            G_fatal_error(_("Output window changed while maps are open for "
                            "write. Map name <%s>"),
                          fcb->name);
    }
}
