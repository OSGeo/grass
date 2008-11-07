#include "list.h"
int add_element(char *elem, char *desc)
{
    int n;
    int nelem;

    if (*desc == 0)
	desc = elem;

    n = nlist - 1;
    nelem = list[n].nelem++;
    list[n].element =
	(char **)G_realloc(list[n].element, (nelem + 1) * sizeof(char *));
    list[n].element[nelem] = G_store(elem);
    list[n].desc =
	(char **)G_realloc(list[n].desc, (nelem + 1) * sizeof(char *));
    list[n].desc[nelem] = G_store(desc);

    return 0;
}
