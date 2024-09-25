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

    element = (char *)G_malloc(80 * sizeof(char));

    if (group == NULL || *group == 0) {
        G_free(element);
        return 0;
    }
    sprintf(element, "group/%s", group);
    file_exists = G_find_file(element, "INIT_EXP", G_mapset()) != NULL;
    G_free(element);
    return file_exists;
}
