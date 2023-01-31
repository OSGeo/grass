/*!
   \file lib/manage/do_list.c

   \brief Manage Library - List elements

   (C) 2001-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/glocale.h>

#include "manage_local_proto.h"

/*!
   \brief List elements

   \param n element index in the array (negative value for all elements)
   \param mapset name of mapset ("" for search path)
 */
void M_do_list(int n, const char *mapset)
{
    int i;

    if (n >= nlist) {
        G_fatal_error(_("%s: invalid index %d"), "M_do_list()", n);
    }

    if (n < 0) {
        for (i = 0; i < nlist; i++) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            G_list_element(list[i].element[0], list[i].desc[0], mapset, NULL);
        }
    }
    else {
        G_list_element(list[n].element[0], list[n].desc[0], mapset, NULL);
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            G_list_element(list[i].element[0], list[i].desc[0], mapset,
                           (int (*)())0);
        }
    }
    else {
        G_list_element(list[n].element[0], list[n].desc[0], mapset,
                       (int (*)())0);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            G_list_element(list[i].element[0], list[i].desc[0], mapset,
                           (int (*)())0);
        }
    }
    else {
        G_list_element(list[n].element[0], list[n].desc[0], mapset,
                       (int (*)())0);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
            G_list_element(list[i].element[0], list[i].desc[0], mapset, NULL);
        }
    }
    else {
        G_list_element(list[n].element[0], list[n].desc[0], mapset, NULL);
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }
}
