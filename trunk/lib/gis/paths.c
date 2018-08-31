#include <grass/config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef __MINGW32__
#include <pwd.h>
#else
#include <windows.h>
#include "aclapi.h"
#endif

#include <grass/gis.h>
#include <grass/glocale.h>

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
 * \param stat pointer to structure filled with file information
 *
 * \return Return value from system lstat function
 **/

int G_stat(const char *file_name, struct stat *buf)
{
    return stat(file_name, buf);
}

/**
 * \brief Get file status
 *
 * Returns information about the specified file.
 *
 * \param file_name file name, in the case of a symbolic link, the 
 *                  link itself is stat-ed, not the file that it refers to
 * \param stat pointer to structure filled with file information
 *
 * \return Return value from system lstat function
 **/

int G_lstat(const char *file_name, struct stat *buf)
{
#ifdef __MINGW32__
    return stat(file_name, buf);
#else
    return lstat(file_name, buf);
#endif
}

/**
 * \brief Get owner id of path
 *
 * Returns information about the specified file.
 *
 * \param path path to check
 *
 * \return Return owner id
 **/

int G_owner(const char *path)
{

#ifndef __MINGW32__
    struct stat info;

    G_stat(path, &info);

    return (int)info.st_uid;
#else

    /* this code is taken from the official example to 
     * find the owner of a file object from
     * http://msdn.microsoft.com/en-us/library/windows/desktop/aa446629%28v=vs.85%29.aspx */

    DWORD dwRtnCode = 0;
    PSID pSidOwner = NULL;
    BOOL bRtnBool = TRUE;
    LPTSTR AcctName = NULL;
    LPTSTR DomainName = NULL;
    DWORD dwAcctName = 1, dwDomainName = 1;
    SID_NAME_USE eUse = SidTypeUnknown;
    HANDLE hFile;
    PSECURITY_DESCRIPTOR pSD = NULL;

    /* Get the handle of the file object. */
    hFile = CreateFile(
                      TEXT(path),		/* lpFileName */
		      GENERIC_READ,		/* dwDesiredAccess */
		      FILE_SHARE_READ,		/* dwShareMode */
		      NULL,			/* lpSecurityAttributes */
		      OPEN_EXISTING,		/* dwCreationDisposition */
		      FILE_ATTRIBUTE_NORMAL,	/* dwFlagsAndAttributes */
		      NULL			/* hTemplateFile */
		      );
    
    if (hFile == INVALID_HANDLE_VALUE) {
	G_fatal_error(_("Unable to open file <%s> for reading"), path);
    }
    
    /* Get the owner SID of the file. */
    dwRtnCode = GetSecurityInfo(
		      hFile,				/* handle */
		      SE_FILE_OBJECT,			/* ObjectType */
		      OWNER_SECURITY_INFORMATION,	/* SecurityInfo */
		      &pSidOwner,			/* ppsidOwner */
		      NULL,				/* ppsidGroup */
		      NULL,				/* ppDacl */
		      NULL,				/* ppSacl */
		      &pSD				/* ppSecurityDescriptor */
		      );
    
    if (dwRtnCode != ERROR_SUCCESS) {
	G_fatal_error(_("Unable to fetch security info for <%s>"), path);
    }
    CloseHandle(hFile);
    
    return (int)pSidOwner;
#endif
}
