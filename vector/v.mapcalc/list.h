#ifndef LIST_H
#define LIST_H

/*
 * Missing functions:
 * - version of listforeach() which allows passing arguments and
 *   which will return an argument
 */

#ifndef LIST
typedef struct List
{
    struct List *next;
} LIST;
#endif

#ifndef LISTFUNC_DEFINED
#define LISTFUNC_DEFINED
typedef int (*cmpfunc) (const void *sample, const void *each);
typedef void (*freefunc) (const void *elt);
typedef void (*cpyfunc) (const void *dst, const void *src);
typedef void (*actfunc) (const void *elt);
#endif

extern LIST *listitem(size_t size);
extern LIST *listadd(LIST * head, LIST * elt, cmpfunc cmp);
extern LIST *listaddnth(LIST * head, LIST * elt, int nth);
extern LIST *listprep(LIST * head, LIST * elt);
extern LIST *listapp(LIST * head, LIST * elt);
extern LIST *listunlink(LIST * head, LIST * elt);
extern LIST *listunlinknth(LIST * head, int nth);
extern LIST *listdel(LIST * head, LIST * elt, freefunc func);
extern LIST *listdelnth(LIST * head, int nth, freefunc func);
extern int listcnt(LIST * head);
extern LIST *listdup(LIST * head, cpyfunc cpy, size_t size);
extern LIST *listsplit(LIST * head, LIST * elt);
extern LIST *listsplitnth(LIST * head, int nth);
extern LIST *listjoin(LIST * head, LIST * tail);
extern LIST *listsort(LIST * head, cmpfunc cmp);
extern LIST *listrev(LIST * head);
extern LIST *listshuffle(LIST * head);
extern LIST *listdelall(LIST * head, freefunc func);
extern LIST **list2array(LIST * head);
extern LIST *array2list(LIST ** array);
extern void listforeach(LIST * head, actfunc action);
extern int listidx(LIST * head, LIST * elt);
extern LIST *listlast(LIST * head);
extern LIST *listnth(LIST * head, int nth);
extern LIST *listfind(LIST * head, LIST * elt, cmpfunc cmp);
extern LIST *listfinddatum(LIST * head, void *datum, cmpfunc cmp);
extern LIST *listbsearch(LIST * head, LIST * elt, cmpfunc cmp);
extern LIST *listbsearchdatum(LIST * head, const void *data, cmpfunc cmp);

#endif
