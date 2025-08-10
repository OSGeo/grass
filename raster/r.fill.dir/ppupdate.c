#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "tinf.h"

struct links {
    int next;
    int next_alt;
    void *pp;
    void *pp_alt;
    int trace;
};

void backtrace(int start, int nbasins, struct links list[])
{
    int i;

    for (i = 1; i <= nbasins; i += 1) {
        if (list[i].next == start && list[i].trace == 0) {
            list[i].trace = start;
            if (get_max(list[start].pp, list[i].pp) == list[start].pp)
                memcpy(list[i].pp, list[start].pp, bpe());
            backtrace(i, nbasins, list);
        }
    }
}

void ppupdate(int fe, int fb, int nl, int nbasins, struct band3 *elev,
              struct band3 *basins)
{
    int i, j, ii, n;
    CELL *here;
    CELL that_basin;
    void *barrier_height;
    void *this_elev = NULL;
    void *that_elev = NULL;

    struct links *list;

    list = G_malloc((nbasins + 1) * sizeof(struct links));

    for (i = 1; i <= nbasins; i += 1) {
        list[i].next = -1;
        list[i].pp = G_malloc(bpe());
        set_max(list[i].pp);

        list[i].next_alt = -1;
        list[i].pp_alt = G_malloc(bpe());
        set_max(list[i].pp_alt);

        list[i].trace = 0;
    }

    lseek(fe, 0, SEEK_SET);
    lseek(fb, 0, SEEK_SET);

    advance_band3(fb, basins);
    advance_band3(fb, basins);

    advance_band3(fe, elev);
    advance_band3(fe, elev);

    for (i = 1; i < nl - 1; i += 1) {
        advance_band3(fb, basins);
        advance_band3(fe, elev);

        for (j = 1; j < basins->ns - 1; j += 1) {

            /* check to see if the cell is non-null and in a basin */
            here = (CELL *)basins->b[1] + j;
            if (Rast_is_c_null_value(here) || *here < 0)
                continue;

            ii = *here;
            this_elev = elev->b[1] + j * bpe();

            /* check each adjoining cell; see if we're on a boundary. */
            for (n = 0; n < 8; n += 1) {

                switch (n) {
                case 0:
                    that_basin = *((CELL *)basins->b[0] + j + 1);
                    that_elev = elev->b[0] + (j + 1) * bpe();
                    break;
                case 1:
                    that_basin = *((CELL *)basins->b[1] + j + 1);
                    that_elev = elev->b[1] + (j + 1) * bpe();
                    break;
                case 2:
                    that_basin = *((CELL *)basins->b[2] + j + 1);
                    that_elev = elev->b[2] + (j + 1) * bpe();
                    break;
                case 3:
                    that_basin = *((CELL *)basins->b[2] + j);
                    that_elev = elev->b[2] + j * bpe();
                    break;
                case 4:
                    that_basin = *((CELL *)basins->b[2] + j - 1);
                    that_elev = elev->b[2] + (j - 1) * bpe();
                    break;
                case 5:
                    that_basin = *((CELL *)basins->b[1] + j - 1);
                    that_elev = elev->b[1] + (j - 1) * bpe();
                    break;
                case 6:
                    that_basin = *((CELL *)basins->b[0] + j - 1);
                    that_elev = elev->b[0] + (j - 1) * bpe();
                    break;
                case 7:
                    that_basin = *((CELL *)basins->b[0] + j);
                    that_elev = elev->b[0] + j * bpe();

                } /* end switch */

                /* see if we're on a boundary */
                if (that_basin != ii) {

                    /* what is that_basin if that_elev is null ? */
                    if (is_null(that_elev)) {
                        barrier_height = this_elev;
                    }
                    else {
                        barrier_height = get_max(that_elev, this_elev);
                    }
                    if (get_min(barrier_height, list[ii].pp) ==
                        barrier_height) {
                        /* save the old list entry in case we need it to fix a
                         * loop */
                        if (list[ii].next != that_basin) {
                            memcpy(list[ii].pp_alt, list[ii].pp, bpe());
                            list[ii].next_alt = list[ii].next;
                        }
                        /* create the new list entry */
                        memcpy(list[ii].pp, barrier_height, bpe());
                        list[ii].next = that_basin;
                    }
                    else if (get_min(barrier_height, list[ii].pp_alt) ==
                             barrier_height) {
                        if (list[ii].next == that_basin)
                            continue;
                        memcpy(list[ii].pp_alt, barrier_height, bpe());
                        list[ii].next_alt = that_basin;
                    }
                } /* end if */

            } /* end neighbor cells */

        } /* end cell */

    } /* end row */

    if (!this_elev || !that_elev)
        G_fatal_error(_("Unexpected NULL pointer in %s"), __func__);

    /* Look for pairs of basins that drain to each other */
    for (i = 1; i <= nbasins; i += 1) {
        if (list[i].next <= 0)
            continue;

        n = list[i].next;
        if (list[n].next == i) {
            /* we have a pair */
            /* find out how large the elevation difference would be for a change
             * in each basin */
            memcpy(that_elev, list[n].pp_alt, bpe());
            diff(that_elev, list[n].pp);

            memcpy(this_elev, list[i].pp_alt, bpe());
            diff(this_elev, list[i].pp);

            /* switch pour points in the basin where it makes the smallest
             * change */
            if (get_min(this_elev, that_elev) == this_elev) {
                list[i].next = list[i].next_alt;
                list[i].next_alt = n;

                this_elev = list[i].pp;
                list[i].pp = list[i].pp_alt;
                list[i].pp_alt = this_elev;
            }
            else {
                ii = list[n].next;
                list[n].next = list[n].next_alt;
                list[n].next_alt = ii;

                this_elev = list[n].pp;
                list[n].pp = list[n].pp_alt;
                list[n].pp_alt = this_elev;
            } /* end fix */

        } /* end problem */

    } /* end loop */

    /* backtrace drainages from the bottom and adjust pour points */
    for (i = 1; i <= nbasins; i += 1) {
        if (list[i].next == -1) {
            list[i].trace = i;
            backtrace(i, nbasins, list);
        }
    }

    /* fill all basins up to the elevation of their lowest bounding elevation */
    lseek(fe, 0, SEEK_SET);
    lseek(fb, 0, SEEK_SET);
    for (i = 0; i < nl; i += 1) {
        if (read(fe, elev->b[1], elev->sz) < 0)
            G_fatal_error(_("File reading error in %s() %d:%s"), __func__,
                          errno, strerror(errno));
        if (read(fb, basins->b[1], basins->sz) < 0)
            G_fatal_error(_("File reading error in %s() %d:%s"), __func__,
                          errno, strerror(errno));
        for (j = 0; j < basins->ns; j += 1) {
            ii = *((CELL *)basins->b[1] + j);
            if (ii <= 0)
                continue;
            this_elev = elev->b[1] + j * bpe();
            memcpy(this_elev, get_max(this_elev, list[ii].pp), bpe());
        }
        lseek(fe, -elev->sz, SEEK_CUR);
        if (write(fe, elev->b[1], elev->sz) < 0)
            G_fatal_error(_("File writing error in %s() %d:%s"), __func__,
                          errno, strerror(errno));
    }

    G_free(list);
}
