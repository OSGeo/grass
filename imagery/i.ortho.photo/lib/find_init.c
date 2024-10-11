/**************************************************************
 * I_find_init (group)
 *
 * Find the a camera initial file in the current group (if it exists)
 **************************************************************/
#include <grass/gis.h>
#include <grass/glocale.h>

int I_find_initial(char *group)
{
    char element[GNAME_MAX + 6];

    if (group == NULL || *group == 0)
        return 0;

    if (snprintf(element, GNAME_MAX, "group/%s", group) >= GNAME_MAX) {
        G_warning(_("Group name <%s> is too long"), group);
        return 0;
    }

    return G_find_file(element, "INIT_EXP", G_mapset()) != NULL;
}
