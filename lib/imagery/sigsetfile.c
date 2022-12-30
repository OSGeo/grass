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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    char dir[GNAME_MAX];
    FILE *fd;

    /* create sig directory */
    I_make_signatures_dir(I_SIGFILE_TYPE_SIGSET);

    I_get_signatures_dir(dir, I_SIGFILE_TYPE_SIGSET);
    fd = G_fopen_new_misc(dir, "sig", name);
<<<<<<< HEAD
=======
    char element[GNAME_MAX];
=======
    char dir[GNAME_MAX];
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
    FILE *fd;

    /* create sig directory */
    I_make_signatures_dir(I_SIGFILE_TYPE_SIGSET);

<<<<<<< HEAD
    I__get_signatures_element(element, I_SIGFILE_TYPE_SIGSET);
    fd = G_fopen_new(element, name);
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    I_get_signatures_dir(dir, I_SIGFILE_TYPE_SIGSET);
    fd = G_fopen_new_misc(dir, "sig", name);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

=======

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    char dir[GNAME_MAX];
=======
    char element[GNAME_MAX];
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    char dir[GNAME_MAX];
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    char dir[GNAME_MAX];
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    FILE *fd;

    if (G_unqualified_name(name, NULL, sig_name, sig_mapset) == 0)
        strcpy(sig_mapset, G_mapset());

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    I_get_signatures_dir(dir, I_SIGFILE_TYPE_SIGSET);
    fd = G_fopen_old_misc(dir, "sig", sig_name, sig_mapset);
=======
    I__get_signatures_element(element, I_SIGFILE_TYPE_SIGSET);
    fd = G_fopen_old(element, sig_name, sig_mapset);
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    I_get_signatures_dir(dir, I_SIGFILE_TYPE_SIGSET);
    fd = G_fopen_old_misc(dir, "sig", sig_name, sig_mapset);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    I_get_signatures_dir(dir, I_SIGFILE_TYPE_SIGSET);
    fd = G_fopen_old_misc(dir, "sig", sig_name, sig_mapset);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    return fd;
}
