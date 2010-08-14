#include <grass/config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grass/gis.h>

/**
 * \brief Creates a new directory
 *
 * Creates a new directory with permissions 0777 (on Unix) or
 * default permissions(?) on Windows.
 *
 * \param path String containing path of directory to be created
 *
 * \return Return value from system mkdir() function
 **/

int G_mkdir(const char *path)
{
#ifdef __MINGW32__
    return mkdir(path);
#else
    return mkdir(path, 0777);
#endif
}

/**
 * \brief Checks if a specified character is a valid directory
 *        separator character on the host system
 *
 * \param c Character to check
 *
 * \return 1 if c is a directory separator character, 0 if not
 **/

int G_is_dirsep(char c)
{
    if (c == GRASS_DIRSEP || c == HOST_DIRSEP)
	return 1;
    else
	return 0;
}

/**
 * \brief Checks if a specified path looks like an absolute
 *        path on the host system
 *
 * \param path String containing path to check
 *
 * \return 1 if path looks like an absolute path, 0 if not
 **/

int G_is_absolute_path(const char *path)
{
    if (G_is_dirsep(path[0])
#ifdef __MINGW32__
	|| (isalpha(path[0]) && (path[1] == ':') && G_is_dirsep(path[2]))
#endif
	)
	return 1;
    else
	return 0;
}

/**
 * \brief Converts directory separator characters in a string to the
 *        native host separator character (/ on Unix, \ on Windows)
 *
 * \param path String to be converted
 *
 * \return Pointer to the string
 **/

char *G_convert_dirseps_to_host(char *path)
{
    char *i;

    for (i = path; *i; i++) {
	if (*i == GRASS_DIRSEP)
	    *i = HOST_DIRSEP;
    }

    return path;
}

/**
 * \brief Converts directory separator characters in a string from the
 *        native host character to the GRASS separator character (/)
 *
 *
 * \param path String to be converted
 *
 * \return Pointer to the string
 **/

char *G_convert_dirseps_from_host(char *path)
{
    char *i;

    for (i = path; *i; i++) {
	if (*i == HOST_DIRSEP)
	    *i = GRASS_DIRSEP;
    }

    return path;
}

/**
 * \brief Get file status
 *
 * Returns information about the specified file.
 *
 * \param file_name file name
 * \param stat
 *
 * \return Return value from system lstat function
 **/

int G_stat(const char *file_name, STRUCT_STAT *buf)
{
    return stat(file_name, buf);
}

/**
 * \brief Get file status
 *
 * Returns information about the specified file.
 *
 * \param file_name file name
 * \param stat in the case of a symbolic link, the link itself is
 *             stat-ed, not the file that it refers to
 *
 * \return Return value from system lstat function
 **/

int G_lstat(const char *file_name, STRUCT_STAT *buf)
{
#ifdef __MINGW32__
    return stat(file_name, buf);
#else
    return lstat(file_name, buf);
#endif
}
