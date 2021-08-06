/*!
   \file lib/imagery/sigfile.c

   \brief Imagery Library - Signature file functions (statistics for i.maxlik).

   (C) 2001-2008, 2013, 2021 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author USA CERL
 */

#include <string.h>
#include <grass/imagery.h>

/*!
   \brief Create signature file

<<<<<<< HEAD
   Returns a pointer to FILE for writing signature file.
   Use fclose on the pointer to close after use.

=======
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
   \param name signature filename

   \return pointer to FILE
   \return NULL on error
 */
FILE *I_fopen_signature_file_new(const char *name)
{
<<<<<<< HEAD
    char dir[GNAME_MAX];
    FILE *fd;

    /* create sig directory */
    I_make_signatures_dir(I_SIGFILE_TYPE_SIG);
    I_get_signatures_dir(dir, I_SIGFILE_TYPE_SIG);
    fd = G_fopen_new_misc(dir, "sig", name);

=======
    char element[GNAME_MAX];
    FILE *fd;

    /* create sig directory */
    I__make_signatures_element(I_SIGFILE_TYPE_SIG);

    I__get_signatures_element(element, I_SIGFILE_TYPE_SIG);
    fd = G_fopen_new(element, name);

>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
    return fd;
}

/*!
   \brief Open existing signature file

<<<<<<< HEAD
   Use fully qualified names for signatures from other mapsets.

   Returns a pointer to FILE with signature. Use fclose on the pointer
   after use.
=======
   Use fully qualified names for signatures from other mapsets
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))

   \param name signature filename

   \return pointer to FILE
   \return NULL on error
 */
FILE *I_fopen_signature_file_old(const char *name)
{
    char sig_name[GNAME_MAX], sig_mapset[GMAPSET_MAX];
<<<<<<< HEAD
    char dir[GNAME_MAX];
=======
    char element[GNAME_MAX];
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
    FILE *fd;

    if (G_unqualified_name(name, NULL, sig_name, sig_mapset) == 0)
        strcpy(sig_mapset, G_mapset());

<<<<<<< HEAD
    I_get_signatures_dir(dir, I_SIGFILE_TYPE_SIG);
    fd = G_fopen_old_misc(dir, "sig", sig_name, sig_mapset);
=======
    I__get_signatures_element(element, I_SIGFILE_TYPE_SIG);
    fd = G_fopen_old(element, sig_name, sig_mapset);
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))

    return fd;
}
