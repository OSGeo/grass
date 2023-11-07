#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/manage.h>

void list_elements(void)
{
    G_message(_("List of available elements:"));
    M_read_list(FALSE, NULL);
    M_show_elements();
}

#if 0
int check_element(const char *element)
{
    return M_read_list(FALSE, NULL);
}
#endif
