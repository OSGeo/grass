#include "list.h"
void do_list(int n, const char *mapset)
{
    G_list_element(list[n].element[0], list[n].desc[0], mapset, (int (*)())0);
}
