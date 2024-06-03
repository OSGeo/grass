/*!
   \file lib/vector/Vlib/rewind.c

   \brief Vector library - rewind data

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009, 2011-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author Level 3 by Martin Landa <landa.martin gmail.com>
 */

#include <grass/vector.h>
#include <grass/glocale.h>

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static int rew_dummy(struct Map_info *Map)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
static int rew_dummy(struct Map_info *Map UNUSED)
=======
static int rew_dummy(struct Map_info *Map)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
static int rew_dummy(struct Map_info *Map)
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    return -1;
}

#if !defined HAVE_OGR || !defined HAVE_POSTGRES
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static int format(struct Map_info *Map)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
static int format(struct Map_info *Map UNUSED)
=======
static int format(struct Map_info *Map)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
static int format(struct Map_info *Map)
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}
#endif

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static int (*Rewind_array[][4])(struct Map_info *) = {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static int (*Rewind_array[][4])(struct Map_info *) = {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static int (*Rewind_array[][4])(struct Map_info *) = {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
static int (*Rewind_array[][4])(struct Map_info *) = {
=======
>>>>>>> osgeo-main
static int (*Rewind_array[][4])() = {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
static int (*Rewind_array[][4])() = {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
static int (*Rewind_array[][4])(struct Map_info *) = {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
static int (*Rewind_array[][4])() = {
=======
static int (*Rewind_array[][4])(struct Map_info *) = {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    {rew_dummy, V1_rewind_nat, V2_rewind_nat, rew_dummy}
#ifdef HAVE_OGR
    ,
    {rew_dummy, V1_rewind_ogr, V2_rewind_ogr, rew_dummy},
    {rew_dummy, V1_rewind_ogr, V2_rewind_ogr, rew_dummy}
#else
    ,
    {rew_dummy, format, format, rew_dummy},
    {rew_dummy, format, format, rew_dummy}
#endif
#ifdef HAVE_POSTGRES
    ,
    {rew_dummy, V1_rewind_pg, V2_rewind_pg, V2_rewind_pg}
#else
    ,
    {rew_dummy, format, format, rew_dummy}
#endif
};

/*!
   \brief Rewind vector map to cause reads to start at beginning

   \param Map pointer to Map_info structure

   \return 0 on success
   \return -1 on error
 */
int Vect_rewind(struct Map_info *Map)
{
    if (!VECT_OPEN(Map))
        return -1;

    G_debug(1, "Vect_Rewind(): name = %s level = %d", Map->name, Map->level);

    return (*Rewind_array[Map->format][Map->level])(Map);
}
