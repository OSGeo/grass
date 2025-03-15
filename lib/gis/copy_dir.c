/*!
 * \file lib/gis/copy_dir.c
 *
 * \brief GIS Library - function to recursively copy a directory
 *
 * Extracted from general/manage/lib/do_copy.c
 *
 * (C) 2008-2015 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Huidae Cho
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <grass/gis.h>

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

/*!
 * \brief Copy recursively source directory to destination directory
 *
 * RULE:
 * 1. If destination does not exist, copy source to destination as expected.
 * 2. If destination already exists and it's a file, destination will be
 *    deleted first and apply RULE 1.
 * 3. If destination already exists which is a directory and source is a file,
 *    try to copy source to destination directory.
 * 4. If destination already exists which is a directory and source is also a
 *    directory, try to copy all contents in source to destination directory.
 *
 * This rule is designed according to general/manage/lib/copy.sh.
 *
 * POSSIBLE CASES:
 * \verbatim
 * if src is a file:
 *      if dst does not exist:
 *              copy src to dst                         RULE 1
 *      if dst is a file:
 *              delete dst and copy src to dst          RULE 2
 *      if dst is a directory:
 *              try recursive_copy(src, dst/src)        RULE 3
 * if src is a directory:
 *      if dst does not exist:
 *              copy src to dst                         RULE 1
 *      if dst is a file:
 *              delete dst and copy src to dst          RULE 2
 *      if dst is a directory:
 *              try                                     RULE 4
 *              for i in `ls src`
 *              do
 *                      recursive_copy(src/$i, dst/$i)
 *              done
 * \endverbatim
 *
 * \param src source directory
 * \param dst destination directory
 *
 * \return 0 if successful, otherwise 1
 */

int G_recursive_copy(const char *src, const char *dst)
{
    DIR *dirp;
    struct stat sb;

    if (G_lstat(src, &sb) < 0)
        return 1;

    /* src is a file */
    if (!S_ISDIR(sb.st_mode)) {
        char buf[4096];
        int fd, fd2;
        ssize_t len, len2;

        if (G_lstat(dst, &sb) == 0 && S_ISDIR(sb.st_mode)) {
            char path[GPATH_MAX];
            const char *p = strrchr(src, '/');

            /* src => dst/src */
            sprintf(path, "%s/%s", dst, (p ? p + 1 : src));
            return G_recursive_copy(src, path);
        }

        /* src => dst */
        if ((fd = open(src, O_RDONLY)) < 0)
            return 1;

        if ((fd2 = open(dst, O_CREAT | O_TRUNC | O_WRONLY, sb.st_mode & 0777)) <
            0) {
            close(fd);
            return 1;
        }

        while ((len = read(fd, buf, sizeof(buf))) > 0) {
            while ((len > 0) && (len2 = write(fd2, buf, (size_t)len)) >= 0)
                len -= len2;
        }

        close(fd);
        close(fd2);

        return 0;
    }

    /* src is a directory */
    if (G_lstat(dst, &sb) < 0) {
        if (G_mkdir(dst))
            return 1;
    }
    else
        /* if dst already exists and it's a file, try to remove it */
        if (!S_ISDIR(sb.st_mode)) {
            if (remove(dst) < 0 || G_mkdir(dst) < 0)
                return 1;
        }

    dirp = opendir(src);
    if (!dirp)
        return 1;

    for (;;) {
        char path[GPATH_MAX], path2[GPATH_MAX];
        struct dirent *dp = readdir(dirp);

        if (!dp)
            break;

        /* do not copy hidden files */
        if (dp->d_name[0] == '.')
            continue;

        sprintf(path, "%s/%s", src, dp->d_name);
        sprintf(path2, "%s/%s", dst, dp->d_name);

        if (G_recursive_copy(path, path2) != 0) {
            closedir(dirp);
            return 1;
        }
    }

    closedir(dirp);

    return 0;
}
