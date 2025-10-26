/*!
   \file lib/vector/vedit/delete.c

   \brief Vedit library - delete vector features

   (C) 2007-2008, 2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <stdlib.h>

#include <grass/vedit.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/*!
   \brief Delete selected features

   \param Map pointer to Map_info
   \param List list of features to be deleted

   \return number of deleted features
   \return -1 on error
 */
int Vedit_delete_lines(struct Map_info *Map, struct ilist *List)
{
    int i, line;
    int nlines_removed;

    nlines_removed = 0;

    for (i = 0; i < List->n_values; i++) {
        line = List->value[i];

        if (!Vect_line_alive(Map, line)) {
            G_warning(_("Attempt to delete dead feature (%d)"), line);
            continue;
        }

        if (Vect_delete_line(Map, line) < 0) {
            return -1;
        }

        G_debug(3, "Vedit_delete_lines(): line=%d", line);
        nlines_removed++;
    }

    return nlines_removed;
}

/*!
   \brief Delete area (centroid and set of boundaries) by centroid

   \param Map pointer to Map_info struct
   \param centroid

   \return 0 no area deleted
   \return 1 area deleted
 */
int Vedit_delete_area_centroid(struct Map_info *Map, int centroid)
{
    int area;

    G_debug(1, "Vedit_delete_area_centroid(): centroid = %d", centroid);

    area = Vect_get_centroid_area(Map, centroid);
    if (area == 0) {
        G_warning(_("No area found for centroid %d"), centroid);
        return 0;
    }
    if (area < 0) {
        G_warning(_("Duplicate centroid %d, unable to delete area"), centroid);
        return 0;
    }

    return Vedit_delete_area(Map, area);
}

/*!
   \brief Delete area (centroid + set of boundaries) by id

   \param Map pointer to Map_info struct
   \param area id

   \return 0 no area deleted
   \return 1 area deleted
 */
int Vedit_delete_area(struct Map_info *Map, int area)
{
    int i, line, centroid, left, right;
    struct ilist *list;

    G_debug(3, "Vedit_delete_area(): area=%d", area);
    centroid = Vect_get_area_centroid(Map, area);
    if (centroid != 0) {
        Vect_delete_line(Map, centroid);
    }
    else {
        G_warning(_("Area %d without centroid"), area);
        return 0;
    }
    list = Vect_new_list();
    Vect_get_area_boundaries(Map, area, list);
    if (list->n_values > 0) {
        for (i = 0; i < list->n_values; i++) {
            line = abs(list->value[i]);
            Vect_get_line_areas(Map, line, &left, &right);
            if (left > 0 && right > 0)
                /* do not delete common boundaries */
                continue;

            Vect_delete_line(Map, line);
        }
    }
    else {
        G_warning(_("Area %d has no boundaries"), area);
        Vect_destroy_list(list);
        return 0;
    }

    Vect_destroy_list(list);

    return 1;
}

/*!
   \brief Delete vector areas of given category

   \param Map pointer to Map_info struct
   \param field layer number
   \param cat category number

   \return number of deleted areas
 */
int Vedit_delete_areas_cat(struct Map_info *Map, int field, int cat)
{
    int area, nareas, nremoved;

    G_debug(1, "Vedit_delete_areas(): field = %d cat = %d", field, cat);
    nareas = Vect_get_num_areas(Map);
    nremoved = 0;
    for (area = 1; area <= nareas; area++) {
        if (!Vect_area_alive(Map, area))
            continue;
        if (Vect_get_area_cat(Map, area, field) != cat)
            continue;

        if (Vedit_delete_area(Map, area))
            nremoved++;
    }

    return nremoved;
}
