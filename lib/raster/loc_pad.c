#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/graphics.h>

#include "transport.h"
#include "pad.h"

/* PAD FUNCTIONS
   The monitor has a very simple database management capabil­
   ity  which supports the windowing.  There are scratch pads
   to be written on. Each scratch pad can contain items,  and
   each  item can have a list of values.  These are NOT to be
   used by the programmer.  They are used indirectly  through
   the displaylib library calls.
 */

static PAD *curpad;		/* current selected pad */

int LOC_pad_create(const char *pad)
{
    if (*pad == 0)		/* this is scratch pad */
	return OK;
    else if (find_pad(pad) != NULL)
	return DUPLICATE;	/* duplicate pad */
    else if (create_pad(pad))
	return OK;
    else
	return NO_MEMORY;
}

int LOC_pad_current(char *name)
{
    if (curpad == NULL) {
	*name = '\0';
	return NO_CUR_PAD;
    }
    else {
	strcpy(name, curpad->name);
	return OK;
    }
}

int LOC_pad_delete(void)
{
    if (curpad == NULL)
	return NO_CUR_PAD;
    else if (*curpad->name == 0)
	return ILLEGAL;
    else {
	delete_pad(curpad);
	curpad = NULL;
	return OK;
    }
}

int LOC_pad_invent(char *pad)
{
    invent_pad(pad);

    return 0;
}

int LOC_pad_list(char ***list, int *count)
{
    PAD *p;
    char **l;
    int n;

    for (p = pad_list(), n = 0; p; p = p->next)
	if (*p->name)
	    n++;

    *count = n;
    *list = l = G_malloc(n * sizeof(char *));

    for (p = pad_list(); p; p = p->next)
	if (*p->name)
	    *l++ = G_store(p->name);

    return 0;
}

int LOC_pad_select(const char *pad)
{
    curpad = find_pad(pad);

    if (curpad == NULL)
	return NO_PAD;

    return OK;
}

int LOC_pad_append_item(const char *item, const char *value, int replace)
{
    if (curpad == NULL)
	return NO_CUR_PAD;

    if (append_item(curpad, item, value, replace))
	return OK;

    return NO_MEMORY;
}

int LOC_pad_delete_item(const char *name)
{
    if (curpad == NULL)
	return NO_CUR_PAD;

    delete_item(curpad, name);
    return OK;
}

int LOC_pad_get_item(const char *name, char ***list, int *count)
{
    ITEM *item;
    LIST *p;
    char **l;
    int n;

    if (curpad == NULL)
	return NO_CUR_PAD;

    item = find_item(curpad, name);
    if (item == NULL)
	return NO_ITEM;

    for (p = item->list, n = 0; p; p = p->next)
	if (*p->value)
	    n++;

    *count = n;
    *list = l = G_malloc(n * sizeof(char *));

    for (p = item->list, n = 0; p; p = p->next)
	if (*p->value)
	    *l++ = G_store(p->value);

    return OK;
}

int LOC_pad_list_items(char ***list, int *count)
{
    ITEM *p;
    char **l;
    int n;

    if (curpad == NULL)
	return NO_CUR_PAD;

    for (p = curpad->items, n = 0; p; p = p->next)
	if (*p->name)
	    n++;
    *count = n;
    *list = l = G_malloc(n * sizeof(char *));

    for (p = curpad->items, n = 0; p; p = p->next)
	if (*p->name)
	    *l++ = G_store(p->name);

    return OK;
}

int LOC_pad_set_item(const char *name, const char *value)
{
    if (curpad == NULL)
	return NO_CUR_PAD;

    delete_item(curpad, name);

    if (append_item(curpad, name, value, 0))
	return OK;

    return NO_MEMORY;
}
