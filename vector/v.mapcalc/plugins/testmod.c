#include <stdio.h>
#include <string.h>
#include "../list.h"
#include "../mapcalc.h"
#include "../map.h"


/*
 * Required function:
 * Return the name of the main function which is useable in v.mapcalc.
 */
char *fname(void)
{
    return "dltest";
}

/*
 * Required function:
 * Return the prototype of the main function. Generally, it's the initial
 * letter of the C-type, plus "m" for (MAP *) and "p" for (POINT *).
 * Indirection doesn't fully work, but to return a pointer to one of the
 * above types, the letter "a" is used ("ai" means an integer pointer).
 *
 * Each prototype needs to be represented by one of the typedef and switch
 * cases in func.c of mapcalc.
 */
char *proto(void)
{
    return "m=mm";
}

/*
 * This is the main function which needs to have the name and prototype
 * as returned above.
 */
MAP *dltest(MAP * m, MAP * n)
{
    char namebuf[128];

    printf("Performing 2 arg dynamically loaded map function on maps "
	   "%s and %s\n", m->name, n->name);
    sprintf(namebuf, "%s.%s", m->name, n->name);
    m = (MAP *) listitem(sizeof(MAP));
    m->name = strdup(namebuf);
    return m;
}
