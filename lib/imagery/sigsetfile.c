/*!
  \file lib/imagery/sigsetfile.c
 
  \brief Imagery Library - Signature file functions (statistics for i.smap)
 
  (C) 2001-2011, 2013 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author USA CERL
*/

#include <string.h>

#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

/*!
  \brief Create new sigset file

  \param name name of sigset file

  \return pointer to FILE
  \return NULL on error
*/
FILE *I_fopen_sigset_file_new(const char *name)
{
    FILE *fd;

    /* create sig directory */
    G__make_mapset_element_misc("signatures", "sigset");

    fd = G_fopen_new_misc("signatures", name, "sigset");
    
    return fd;
}

/*!
  \brief Open existing sigset signature file

  \param name name of signature file (may be fully qualified)

  \return pointer to FILE*
  \return NULL on error
*/
FILE *I_fopen_sigset_file_old(const char *name)
{
    char sig_name[GNAME_MAX], sig_mapset[GMAPSET_MAX];
    FILE *fd;

    if (G_unqualified_name(name, NULL, sig_name, sig_mapset) == 0)
        strcpy(sig_mapset, G_mapset());

    fd = G_fopen_old_misc("signatures", sig_name, "sigset", sig_mapset);
    
    return fd;
}
