/**************************************************************
 * I_find_init (group)
 *
 * Find the a camera initial file in the current group (if it exists)
 **************************************************************/
#include <grass/gis.h>

int I_find_initial(char *group)
{
    char element[GNAME_MAX];
    int file_exists;

    if (group == NULL || *group == 0)
        return 0;

    if (snprintf(element, GNAME_MAX, "group/%s", group) >= GNAME_MAX) {
        G_warning("Group name truncated");
        return 0;
    }
    
    file_exists = G_find_file(element, "INIT_EXP", G_mapset()) != NULL;
    return file_exists;
}
