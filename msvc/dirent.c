#include <stdlib.h>
#include <io.h>
#include <errno.h>
#include <dirent.h>

typedef ptrdiff_t handle_type; /* C99's intptr_t not sufficiently portable */

struct DIR {
    handle_type handle; /* -1 for failed rewind */
    struct _finddata_t info;
    struct dirent result; /* d_name null iff first time */
    char *name;           /* null-terminated char string */
};

DIR *opendir(const char *name)
{
    DIR *dir = 0;

    if (name && name[0]) {
        size_t base_length = strlen(name);
        const char *all = /* search pattern must end with suitable wildcard */
            strchr("/\\", name[base_length - 1]) ? "*" : "/*";
        dir = (DIR *)malloc(sizeof *dir);
        if (dir != 0 &&
            (dir->name = (char *)malloc(base_length + strlen(all) + 1)) != 0) {
            strcpy(dir->name, name);
            strcat(dir->name, all);

            if ((dir->handle =
                     (handle_type)_findfirst(dir->name, &dir->info)) != -1) {
                dir->result.d_name = 0;
            }
            else /* rollback */
            {
                free(dir->name);
                free(dir);
                dir = 0;
            }
        }
        else /* rollback */
        {
            free(dir);
            dir = 0;
            errno = ENOMEM;
        }
    }
    else {
        errno = EINVAL;
    }

    return dir;
}

int closedir(DIR *dir)
{
    int result = -1;

    if (dir) {
        if (dir->handle != -1) {
            result = _findclose(dir->handle);
        }

        free(dir->name);
        free(dir);
    }

    if (result == -1) /* map all errors to EBADF */
    {
        errno = EBADF;
    }

    return result;
}

struct dirent *readdir(DIR *dir)
{
    struct dirent *result = 0;

    if (dir && dir->handle != -1) {
        if (!dir->result.d_name || _findnext(dir->handle, &dir->info) != -1) {
            result = &dir->result;
            result->d_name = dir->info.name;
        }
    }
    else {
        errno = EBADF;
    }

    return result;
}

void rewinddir(DIR *dir)
{
    if (dir && dir->handle != -1) {
        _findclose(dir->handle);
        dir->handle = (handle_type)_findfirst(dir->name, &dir->info);
        dir->result.d_name = 0;
    }
    else {
        errno = EBADF;
    }
}
