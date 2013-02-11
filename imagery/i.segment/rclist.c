#include <grass/gis.h>
#include <grass/glocale.h>
#include "iseg.h"

void rclist_init(struct rclist *list)
{
    list->head = list->tail = NULL;
    
    return;
}

void rclist_add(struct rclist *list, int row, int col)
{
    struct rc *new = G_malloc(sizeof(struct rc));

    if (!new)
	G_fatal_error(_("rclist out of memory"));

    new->next = NULL;
    new->row = row;
    new->col = col;
    
    if (list->head) {
	list->head->next = new;
	list->head = list->head->next;
    }
    else {
	list->head = list->tail = new;
    }
    
    return;
}

/* return 1 if an element was dropped
 * return 0 if list is empty
 */
int rclist_drop(struct rclist *list, struct rc *rc)
{
    if (list->tail) {
	struct rc *next = list->tail->next;

	rc->row = list->tail->row;
	rc->col = list->tail->col;
	G_free(list->tail);
	list->tail = next;
	if (!list->tail)
	    list->head = NULL;

	return 1;
    }

    return 0;
}

void rclist_destroy(struct rclist *list)
{
    struct rc *next = list->tail;
    
    while (next) {
	next = next->next;
	G_free(list->tail);
	list->tail = next;
    }
    list->head = NULL;
    
    return;
}

