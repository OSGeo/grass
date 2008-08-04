
/**
 * \file tempfile.c
 *
 * \brief GIS Library - Temporary file functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <grass/gis.h>


/**
 * \brief Returns a temporary file name.
 *
 * This routine returns a pointer to a string containing a unique 
 * temporary file name that can be used as a temporary file within the 
 * module. Successive calls to <i>G_tempfile()</i> will generate new 
 * names. Only the file name is generated. The file itself is not 
 * created. To create the file, the module must use standard UNIX 
 * functions which create and open files, e.g., <i>creat()</i> or 
 * <i>fopen()</i>.<br>
 *
 * Successive calls will generate different names the names are of the 
 * form pid.n where pid is the programs process id number and n is a 
 * unique identifier.<br>
 *
 * <b>Note:</b> It is recommended to <i>unlink()</i> (remove) the 
 * temp file on exit/error. Only if GRASS is left with 'exit', the GIS 
 * mapset manangement will clean up the temp directory (ETC/clean_temp).
 *
 * \return pointer to a character string containing the name. The name 
 * is copied to allocated memory and may be released by the unix free() 
 * routine.
 */

char *G_tempfile(void)
{
    return G__tempfile(getpid());
}


/**
 * \brief Create tempfile from process id.
 *
 * See <i>G_tempfile()</i>.
 *
 * \param[in] pid
 * \return Pointer to string path
 */

char *G__tempfile(int pid)
{
    char path[GPATH_MAX];
    char name[GNAME_MAX];
    char element[100];
    static int uniq = 0;
    struct stat st;

    if (pid <= 0)
	pid = getpid();
    G__temp_element(element);
    do {
	sprintf(name, "%d.%d", pid, uniq++);
	G__file_name(path, element, name, G_mapset());
    }
    while (stat(path, &st) == 0);

    return G_store(path);
}


/**
 * \brief Populates <b>element</b> with a path string.
 *
 * \param[in,out] element
 * \return always returns 0
 */

int G__temp_element(char *element)
{
    const char *machine;

    strcpy(element, ".tmp");
    machine = G__machine_name();
    if (machine != NULL && *machine != 0) {
	strcat(element, "/");
	strcat(element, machine);
    }
    G__make_mapset_element(element);

    return 0;
}
