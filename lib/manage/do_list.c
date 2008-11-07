#include "list.h"
int do_list(int n, char *mapset)
{
    G_list_element(list[n].element[0], list[n].desc[0], mapset, (int (*)())0);

    return 0;
}
