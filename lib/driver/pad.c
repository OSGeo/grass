
/* required for NULL */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "pad.h"

static PAD *padlist;

static int free_item(ITEM * item)
{
    LIST *list, *next;

    if (item->name != NULL)
	G_free(item->name);
    for (list = item->list; list != NULL; list = next) {
	next = list->next;
	if (list->value)
	    G_free(list->value);
	G_free(list);
    }
    G_free(item);

    return 0;
}

static ITEM *new_item(PAD * pad, const char *name)
{
    ITEM *item;

    item = (ITEM *) G_malloc((size_t) sizeof(ITEM));
    if (item == NULL)
	return (ITEM *) NULL;

    item->name = G_store(name);
    if (item->name == NULL) {
	G_free(item);
	return (ITEM *) NULL;
    }
    item->list = NULL;
    item->next = pad->items;
    if (item->next != NULL)
	item->next->prev = item;
    item->prev = NULL;
    pad->items = item;

    return item;
}

static int remove_value(ITEM * item, const char *value)
{
    LIST **p = &item->list;
    LIST *l = *p;

    for (l = *p; l; l = *p) {
	if (value && l->value && !strcmp(value, l->value)) {
	    *p = l->next;
	    if (l->value)
		G_free(l->value);
	    G_free(l);
	}
	else
	    p = &l->next;
    }

    return 0;
}

int append_item(PAD * pad, const char *name, const char *value, int replace)
{
    ITEM *item;
    LIST *cur, *prev;
    LIST *list;

    if (pad == NULL)
	return 0;

    /* allocate a list struct and put value into it */
    list = (LIST *) G_malloc((size_t) sizeof(LIST));
    if (list == NULL)
	return 0;
    list->next = NULL;
    list->value = G_store(value);
    if (list->value == NULL) {
	G_free(list);
	return 0;
    }
    /* find the named item for the current pad */
    item = find_item(pad, name);
    if (item == NULL)
	item = new_item(pad, name);
    if (item == NULL)
	return 0;

    /* remove any existing occurences of the value */
    if (replace)
	remove_value(item, value);

    /* add the LIST at the end of the item LIST */
    prev = NULL;
    for (cur = item->list; cur != NULL; cur = cur->next)
	prev = cur;

    if (prev == NULL)
	item->list = list;
    else
	prev->next = list;

    return 1;
}

int delete_item(PAD * pad, const char *name)
{
    ITEM *item;

    item = find_item(pad, name);
    if (item == NULL)
	return 0;

    if (item->prev == NULL)
	pad->items = item->next;
    else
	item->prev->next = item->next;

    if (item->next != NULL)
	item->next->prev = item->prev;

    /* free the item */
    free_item(item);

    return 1;
}

ITEM *find_item(PAD * pad, const char *name)
{
    ITEM *item;

    if (pad != NULL)
	for (item = pad->items; item != NULL; item = item->next)
	    if (strcmp(name, item->name) == 0)
		return item;
    return (ITEM *) NULL;
}

PAD *pad_list(void)
{
    return padlist;
}

static int delink_pad(PAD * pad)
{
    if (pad == NULL)
	return 1;

    if (pad->prev == NULL)
	padlist = pad->next;
    else
	pad->prev->next = pad->next;

    if (pad->next != NULL)
	pad->next->prev = pad->prev;

    return 0;
}

int create_pad(const char *name)
{
    PAD *pad;

    pad = (PAD *) G_malloc((size_t) sizeof(PAD));
    if (pad == NULL)
	return 0;
    pad->name = G_store(name);
    if (pad->name == NULL) {
	G_free(pad);
	return 0;
    }
    pad->items = NULL;
    pad->next = padlist;
    if (pad->next != NULL)
	pad->next->prev = pad;
    pad->prev = NULL;
    padlist = pad;
    return 1;
}

int delete_pad(PAD * pad)
{
    ITEM *item, *next;

    if (pad == NULL)
	return 0;

    delink_pad(pad);

    /* free the items */
    for (item = pad->items; item != NULL; item = next) {
	next = item->next;
	free_item(item);
    }
    G_free(pad);

    return 1;
}

PAD *find_pad(const char *name)
{
    PAD *pad;

    for (pad = padlist; pad != NULL; pad = pad->next)
	if (strcmp(name, pad->name) == 0)
	    return pad;
    return (PAD *) NULL;
}

int invent_pad(char *name)
{
    static int i = 0;

    do
	sprintf(name, "%d", ++i);
    while (find_pad(name) != NULL);

    return 0;
}
