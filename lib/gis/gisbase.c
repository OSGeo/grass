
/**********************************************************************
 *
 *   char *
 *   G_gisbase()
 *
 *   returns:    pointer to string containing the base directory of
 *               GRASS-GRID
 **********************************************************************/

#include <grass/gis.h>


/*!
 * \brief Get full path name of the top level module directory
 *
 * Returns the full path name of the top level directory for GRASS
 * programs. This directory will have subdirectories which will
 * contain modules and files required for the running of the
 * system. Some of these directories are:
 
 \verbatim
 bin    commands run by the user
 etc    modules and data files used by GRASS commands
 \endverbatim

 * The use of G_gisbase() to find these subdirectories enables GRASS
 * modules to be written independently of where the GRASS system is
 * actually installed on the machine. For example, to run the module
 * <i>sroff</i> in the GRASS <i>etc</i> directory:

 \code
 char command[200];

 sprintf(command, "%s/etc/sroff", G_gisbase());
 G_spawn(command, "sroff", NULL);
 \endcode
 *
 * \return pointer to a string
 */
const char *G_gisbase(void)
{
    return G_getenv("GISBASE");
}
