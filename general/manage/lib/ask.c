#include "list.h"

char *ask_old(int n, char *prompt, char *name)
{
    return G_ask_old(prompt, name, list[n].element[0], list[n].desc[0]);
}

char *ask_new(int n, char *prompt, char *name)
{
    return G_ask_new(prompt, name, list[n].element[0], list[n].desc[0]);
}

char *ask_in_mapset(int n, char *prompt, char *name)
{
    return G_ask_in_mapset(prompt, name, list[n].element[0], list[n].desc[0]);
}
