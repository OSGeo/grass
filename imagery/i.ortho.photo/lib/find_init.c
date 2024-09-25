/**************************************************************
 * I_find_init (group)
 *
 * Find the a camera initial file in the current group (if it exists)
 **************************************************************/
#include <grass/gis.h>

int I_find_initial(char *group)
{
    char *element;
    int file_exists;

    if (group == NULL || *group == 0) {
        return 0;
    }

    element = (char *)G_malloc(80 * sizeof(char));

    
    sprintf(element, "group/%s", group);
    file_exists = G_find_file(element, "INIT_EXP", G_mapset()) != NULL;
    G_free(element);
    return file_exists;
}
