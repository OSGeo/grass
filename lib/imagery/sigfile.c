/*!
   \file lib/imagery/sigfile.c
   
   \brief Imagery Library - Signature file functions (statistics for i.maxlik).
 
   (C) 2001-2008, 2013 by the GRASS Development Team
   
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.
   
   \author USA CERL
*/

#include <string.h>
#include <grass/imagery.h>

/*!
   \brief Create signature file

   \param name signature filename

   \return pointer to FILE*
   \return NULL on error
*/
FILE *I_fopen_signature_file_new(const char *name)
{
    char element[GPATH_MAX];
    FILE *fd;

    /* create sig directory */
    G__make_mapset_element_misc("signatures", "sig");

    fd = G_fopen_new_misc("signatures", name, "sig");
    
    return fd;
}

/*!
   \brief Open existing signature file

    Use fully qualified names for signatures from other mapsets

   \param name signature filename

   \return pointer to FILE*
   \return NULL on error
*/
FILE *I_fopen_signature_file_old(const char *name)
{
    char sig_name[GNAME_MAX], sig_mapset[GMAPSET_MAX];
    FILE *fd;

    if (G_unqualified_name(name, NULL, sig_name, sig_mapset) == 0)
        strcpy(sig_mapset, G_mapset());

    fd = G_fopen_old_misc("signatures", sig_name, "sig", sig_mapset);
    
    return fd;
}
