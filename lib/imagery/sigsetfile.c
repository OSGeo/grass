/*!
   \file lib/imagery/sigsetfile.c

   \brief Imagery Library - Signature file functions (statistics for i.smap)

   (C) 2001-2011, 2013, 2021 by the GRASS Development Team

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
    char dir[GNAME_MAX];
    FILE *fd;

    /* create sig directory */
    I_make_signatures_dir(I_SIGFILE_TYPE_SIGSET);

    I_get_signatures_dir(dir, I_SIGFILE_TYPE_SIGSET);
    fd = G_fopen_new_misc(dir, "sig", name);

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
    char dir[GNAME_MAX];
    FILE *fd;

    if (G_unqualified_name(name, NULL, sig_name, sig_mapset) == 0)
        strcpy(sig_mapset, G_mapset());

    I_get_signatures_dir(dir, I_SIGFILE_TYPE_SIGSET);
    fd = G_fopen_old_misc(dir, "sig", sig_name, sig_mapset);

    return fd;
}
