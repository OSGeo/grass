#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#if 0
#include <dmalloc.h>
#endif
#include <grass/gis.h>
#include <grass/glocale.h>
#include "list.h"

LIST *listitem(size_t size);
LIST *listadd(LIST * head, LIST * elt, cmpfunc cmp);
LIST *listaddnth(LIST * head, LIST * elt, int nth);
LIST *listprep(LIST * head, LIST * elt);
LIST *listapp(LIST * head, LIST * elt);
LIST *listunlink(LIST * head, LIST * elt);
LIST *listunlinknth(LIST * head, int nth);
LIST *listdel(LIST * head, LIST * elt, freefunc func);
LIST *listdelnth(LIST * head, int nth, freefunc func);
int listcnt(LIST * head);
LIST *listdup(LIST * head, cpyfunc cpy, size_t size);
LIST *listsplit(LIST * head, LIST * elt);
LIST *listsplitnth(LIST * head, int nth);
LIST *listjoin(LIST * head, LIST * tail);
LIST *listsort(LIST * head, cmpfunc cmp);
LIST *listrev(LIST * head);
LIST *listshuffle(LIST * head);
LIST *listdelall(LIST * head, freefunc func);
LIST **list2array(LIST * head);
LIST *array2list(LIST ** array);
void listforeach(LIST * head, actfunc action);
int listidx(LIST * head, LIST * elt);
LIST *listlast(LIST * head);
LIST *listnth(LIST * head, int nth);
LIST *listfind(LIST * head, LIST * elt, cmpfunc cmp);
LIST *listfinddatum(LIST * head, void *datum, cmpfunc cmp);
LIST *listbsearch(LIST * head, LIST * elt, cmpfunc cmp);
LIST *listbsearchdatum(LIST * head, const void *data, cmpfunc cmp);

static LIST *_listbsearch(LIST * min, int max, LIST * elt, cmpfunc cmp);
static LIST *_listbsearchdatum(LIST * min, int max, const void *datum,
			       cmpfunc cmp);

/*
 * listitem()                   allocate memory for a list item
 */

LIST *listitem(size_t size)
{
    LIST *item;

    item = G_calloc(1, size);
    if (!item) {
	G_fatal_error(_("Out of memory"));
	exit(1);
    }

    return item;
}

/*
 * listadd()                    insert item before first greater
 * (cmp == NULL) -> listapp()
 * This is VERY slow. Rather than a lineal search, we should
 * try to implement a listbapprox() function.
 */

LIST *listadd(LIST * head, LIST * elt, cmpfunc cmp)
{
    LIST *item, *prev = NULL;

    if (elt)
	elt->next = NULL;

    if (!elt)
	return head;
    if (!head)
	return elt;
    if (!cmp)
	return listapp(head, elt);

    for (item = head; item; item = item->next) {
	/*
	 * cmp (sample, each):
	 * Answers if each is smaller/equal/greater than sample
	 */
	if ((*cmp) (elt, item) > 0)
	    break;
	prev = item;
    }

    if (!prev) {
	elt->next = head;
	head = elt;
    }
    else {
	elt->next = prev->next;
	prev->next = elt;
    }

    return head;
}

/*
 * listaddnth()                 make new item the nth of the list
 * (nth <= 0) -> listprep(), (nth > listcnt()) -> listapp()
 */

LIST *listaddnth(LIST * head, LIST * elt, int nth)
{
    LIST *item, *prev = NULL;
    int i;

    if (elt)
	elt->next = NULL;

    if (!head)
	return elt;
    if (!elt)
	return head;

    if (nth < 1) {
	elt->next = head;
	return elt;
    }

    for (i = 0, item = head; item && i < nth; item = item->next, i++)
	prev = item;

    elt->next = prev->next;
    prev->next = elt;

    return head;
}

/*
 * listprep()                           prepend item on list
 */

inline LIST *listprep(LIST * head, LIST * elt)
{
    if (elt && elt->next)
	elt->next = NULL;

    if (!head)
	return elt;
    if (!elt)
	return head;

    elt->next = head;

    return elt;
}

/*
 * listapp()                            append item to list
 */

LIST *listapp(LIST * head, LIST * elt)
{
    LIST *item;

    if (elt)
	elt->next = NULL;

    if (!head)
	return elt;
    if (!elt)
	return head;

    for (item = head; item && item->next; item = item->next) ;

    item->next = elt;

    return head;
}

/*
 * listunlink()                         unlink item from list
 */

LIST *listunlink(LIST * head, LIST * elt)
{
    LIST *item;

    if (!head)
	return NULL;
    if (!elt)
	return head;

    if (head == elt) {
	head = elt->next;
	elt->next = NULL;
	return head;
    }

    for (item = head; item && item->next != elt; item = item->next) ;
    if (item->next == elt) {
	item->next = elt->next;
	elt->next = NULL;
    }
    return head;
}

/*
 * listunlinknth()                      unlink nth element from list
 * listunlinknth (head, 0) == (car (list))
 */

LIST *listunlinknth(LIST * head, int nth)
{
    LIST *item, *elt;
    int i;

    if (!head)
	return NULL;
    if (nth < 0)
	return head;

    if (nth == 0) {
	item = head->next;
	head->next = NULL;
	return item;
    }

    for (i = 0, item = head; item && item->next && i < nth - 1;
	 item = item->next, i++) ;
    if (item->next) {
	elt = item->next;
	item->next = elt->next;
	elt->next = NULL;
    }
    return head;
}

/*
 * listdel()                            unlink and free element from list
 */

LIST *listdel(LIST * head, LIST * elt, freefunc func)
{
    if (!elt)
	return head;

    if (head)
	head = listunlink(head, elt);

    if (func)
	(*func) (elt);
    G_free(elt);

    return head;
}

/*
 * listdelnth()                         unlink and free nth element from list
 */

LIST *listdelnth(LIST * head, int nth, freefunc func)
{
    LIST *item, *elt;
    int i;

    if (!head || nth < 0)
	return NULL;

    if (nth == 0) {
	elt = head;
	head = head->next;
	listdel(NULL, elt, func);
	return head;
    }

    for (i = 0, item = head; item && item->next && i < nth - 1;
	 item = item->next, i++) ;

    if (item && item->next) {
	elt = item->next;
	item->next = elt->next;
	listdel(NULL, elt, func);
    }

    return head;
}

/*
 * listcnt()                            cound elements in list
 */

inline int listcnt(LIST * head)
{
    LIST *item;
    int n = 0;

    for (item = head; item; item = item->next)
	n++;

    return n;
}

/*
 * listdup()                            duplicate list
 * if cpy is NULL, create same number of empty items
 */

LIST *listdup(LIST * head, cpyfunc cpy, size_t size)
{
    LIST *newhead = NULL, *last = NULL, *item, *elt;

    for (item = head; item; item = item->next) {
	elt = listitem(size);
	if (cpy)
	    (*cpy) (elt, item);
	if (!newhead)
	    newhead = last = elt;
	else {
	    last->next = elt;
	    last = elt;
	}
    }

    return newhead;
}

/*
 * listsplit()                          make elt the head of a taillist
 */

LIST *listsplit(LIST * head, LIST * elt)
{
    LIST *item;

    if (!head || !elt)
	return elt;

    if (head == elt)
	return NULL;

    for (item = head; item && item->next != elt; item = item->next) ;
    if (item->next == elt)
	item->next = NULL;

    return elt;
}

/*
 * listsplitnth()                       make nth element the head of a taillist
 */

LIST *listsplitnth(LIST * head, int nth)
{
    LIST *item, *tail = NULL;
    int i;

    if (!head || nth < 1)
	return NULL;

    for (i = 0, item = head; item && i < nth - 1; item = item->next, i++) ;
    if (item && item->next) {
	tail = item->next;
	item->next = NULL;
    }

    return tail;
}

/*
 * listjoin()                           joint two lists
 */

LIST *listjoin(LIST * head, LIST * tail)
{
    LIST *item;

    if (!head)
	return tail;
    if (!tail)
	return head;

    for (item = head; item && item->next; item = item->next) ;
    if (item)
	item->next = tail;

    return head;
}

/*
 * listsort()                           Quick sort on list
 */

LIST *listsort(LIST * head, cmpfunc cmp)
{
    LIST *high = NULL, *low = NULL, *item, *next;

    if (!head || !head->next)
	return head;

    for (item = head; item;) {
	next = item->next;
	if ((*cmp) (item, head) < 0) {
	    item->next = low;
	    low = item;
	}
	else {
	    item->next = high;
	    high = item;
	}
	item = next;
    }

    high = listsort(high, cmp);
    low = listsort(low, cmp);

    head = listjoin(high, low);

    return head;
}

/*
 * listrev()                            reverse order the list
 */

LIST *listrev(LIST * head)
{
    LIST *newhead = NULL, *item, *next;

    for (item = head; item;) {
	next = item->next;
	newhead = listprep(newhead, item);
	item = next;
    }

    return newhead;
}

/*
 * listshuffle()
 */
LIST *listshuffle(LIST * head)
{
    LIST **array, *newhead = NULL;
    int n, i = 0, val;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    srandom(tv.tv_usec);

    n = listcnt(head);
    array = list2array(head);

    while (i < n) {
	val = random() % n;

	if (array[val]) {
	    newhead = listprep(newhead, array[val]);
	    array[val] = NULL;
	    i++;
	}
    }

    G_free(array);

    return newhead;
}

/*
 * listdelall()                         free the whole list
 */

LIST *listdelall(LIST * head, freefunc func)
{
    LIST *item, *next;

    for (item = head; item;) {
	next = item->next;
	if (func)
	    (*func) (item);
	G_free(item);
	item = next;
    }

    return NULL;
}

/*
 * list2array()                         build an array with members of list
 * array is allocated and needs to be G_free ()ed, list not changed.
 */

LIST **list2array(LIST * head)
{
    LIST **array, *item;
    int n, i;

    n = listcnt(head);
    if (!n)
	return NULL;

    array = (LIST **) G_calloc(n + 1, sizeof(LIST *));
    if (!array) {
	G_fatal_error(_("Out of memory"));
	exit(1);
    }

    for (i = 0, item = head; item; item = item->next, i++)
	array[i] = item;

    return array;
}

/*
 * array2list()                         link the elements of an array in order
 */

LIST *array2list(LIST ** array)
{
    LIST *head = NULL, *item = NULL;
    int i;

    if (array)
	head = item = array[0];

    for (i = 1; array && array[i]; i++) {
	item->next = array[i];
	item = item->next;
    }
    item->next = NULL;

    return head;
}

/*
 * listforeach()                        execute action on each item
 */

inline void listforeach(LIST * head, actfunc action)
{
    LIST *item;

    if (!head || !action)
	return;

    for (item = head; item; item = item->next)
	(*action) (item);
}

/*
 * listidx()                            find offset of item from head
 * head is offset (index) zero.
 */

int listidx(LIST * head, LIST * elt)
{
    LIST *item;
    int i;

    if (!elt)
	return -1;

    for (i = 0, item = head; item; item = item->next, i++)
	if (item == elt)
	    break;

    if (!item)
	return -1;

    return i;
}

/*
 * listlast()                           find last item in list.
 */

inline LIST *listlast(LIST * head)
{
    LIST *item;

    for (item = head; item && item->next; item = item->next) ;

    return item;
}

/*
 * listnth()                            find nth item in list.
 */

LIST *listnth(LIST * head, int nth)
{
    LIST *item;
    int i;

    if (!head || nth < 0)
	return NULL;

    for (i = 0, item = head; item; item = item->next, i++)
	if (i == nth)
	    break;

    return item;
}

/*
 * listfind()                   linear search giving structure as sample
 */

LIST *listfind(LIST * head, LIST * elt, cmpfunc cmp)
{
    LIST *item;

    for (item = head; item; item = item->next)
	if (!(*cmp) (elt, item))
	    break;

    return item;
}

/*
 * listfinddatum()              linear search giving sample by pointer
 */

LIST *listfinddatum(LIST * head, void *datum, cmpfunc cmp)
{
    LIST *item;

    for (item = head; item; item = item->next)
	if (!(*cmp) (datum, item))
	    break;

    return item;
}

static LIST *_listbsearch(LIST * min, int max, LIST * elt, cmpfunc cmp)
{
    LIST *item;
    int i, n, result;

    if (!min)
	return NULL;

    n = max / 2;

    for (i = 0, item = min; item && i < n; item = item->next, i++) ;

    result = (*cmp) (item, elt);
    if (result == 0)
	return item;
    if (n == 0) {
	if (max == 1 && item->next && !(*cmp) (item->next, elt))
	    return item->next;
	return NULL;
    }

    if (result < 0)
	item = _listbsearch(min, n, elt, cmp);
    else
	item = _listbsearch(item->next, max - n - 1, elt, cmp);

    return item;
}

/*
 * listbsearch()                        binary search with struct as sample
 */

LIST *listbsearch(LIST * head, LIST * elt, cmpfunc cmp)
{
    LIST *item;
    int n, max, result;

    if (!head || !elt || !cmp)
	return NULL;

    max = listcnt(head);

    n = max / 2;
    item = listnth(head, n);
    result = (*cmp) (item, elt);

    if (result == 0)
	return item;

    if (result < 0)
	item = _listbsearch(head, n, elt, cmp);
    else
	item = _listbsearch(item->next, max - n - 1, elt, cmp);

    return item;
}

static LIST *_listbsearchdatum(LIST * min, int max, const void *datum,
			       cmpfunc cmp)
{
    LIST *elt;
    int i, n, result;

    if (!min)
	return NULL;

    n = max / 2;

    for (i = 0, elt = min; elt && i < n; elt = elt->next, i++) ;

    result = (*cmp) (datum, elt);
    if (result == 0)
	return elt;
    if (n == 0) {
	if (max == 1 && elt->next && !(*cmp) (datum, elt->next))
	    return elt->next;
	return NULL;
    }

    if (result < 0)
	elt = _listbsearchdatum(min, n, datum, cmp);
    else
	elt = _listbsearchdatum(elt->next, max - n - 1, datum, cmp);

    return elt;
}

/*
 * listbsearchdatum()                           binary search for sample by ptr
 */

LIST *listbsearchdatum(LIST * head, const void *datum, cmpfunc cmp)
{
    LIST *elt;
    int n, max, result;

    if (!head || !datum || !cmp)
	return NULL;

    max = listcnt(head);

    n = max / 2;
    elt = listnth(head, n);
    result = (*cmp) (datum, elt);

    if (result == 0)
	return elt;

    if (result < 0)
	elt = _listbsearchdatum(head, n, datum, cmp);
    else
	elt = _listbsearchdatum(elt->next, max - n - 1, datum, cmp);

    return elt;
}
