/*!
   \file lib/vector/Vlib/init_head.c

   \brief Vector library - init header of vector maps
   
   Higher level functions for reading/writing/manipulating vectors.

   Initialize Head structure. To make sure that we are not writing out
   garbage to a file.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author Various updates by Martin Landa <landa.martin gmail.com>, 2009
*/

#include <string.h>
#include <grass/vector.h>

/*!
   \brief Initialize Map_info head structure (dig_head)

   \param[in,out] Map pointer to Map_info structure
 */
void Vect__init_head(struct Map_info *Map)
{
    char buf[64];

    G_zero(&(Map->head), sizeof(struct dig_head *));

    /* organization */
    Vect_set_organization(Map, "");
    
    /* date */
    Vect_set_date(Map, "");

    /* user name */
    sprintf(buf, "%s", G_whoami());
    Vect_set_person(Map, buf);

    /* map name */
    Vect_set_map_name(Map, "");

    /* source date */
    sprintf(buf, "%s", G_date());
    Vect_set_map_date(Map, buf);

    /* comments */
    Vect_set_comment(Map, "");

    /* scale, threshold */
    Vect_set_scale(Map, 1);
    Vect_set_thresh(Map, 0.0);

    /* proj, zone */
    Vect_set_proj(Map, -1);
    Vect_set_zone(Map, -1);

    /* support variables */
    Map->plus.Spidx_built = FALSE;
    Map->plus.release_support = FALSE;
    Map->plus.update_cidx = FALSE;
}

/*!
   \brief Copy header data from one to another map

   \param from target vector map 
   \param[out] to destination vector map

   \return 0
 */
int Vect_copy_head_data(const struct Map_info *from, struct Map_info *to)
{
    Vect_set_organization(to, Vect_get_organization(from));
    Vect_set_date(to, Vect_get_date(from));
    Vect_set_person(to, Vect_get_person(from));
    Vect_set_map_name(to, Vect_get_map_name(from));
    Vect_set_map_date(to, Vect_get_map_date(from));
    Vect_set_comment(to, Vect_get_comment(from));

    Vect_set_scale(to, Vect_get_scale(from));
    Vect_set_zone(to, Vect_get_zone(from));
    Vect_set_thresh(to, Vect_get_thresh(from));

    return 0;
}
