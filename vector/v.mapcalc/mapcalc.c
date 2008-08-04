#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "list.h"
#include "mapcalc.h"
#include "number.h"
#include "map.h"
#include "plugin.h"
#include "vector.h"
#include "any.h"

extern int yyparse(void);	/* v.mapcalc.y */

void freesym(const void *elt);
SYMBOL *delsym(SYMBOL * head);
int cmpsymname(const void *data, const void *elt);
int cmpsymsym(const void *A, const void *B);
void symcpy(const void *Dst, const void *Src);
SYMBOL *getsym(const char *name);
SYMBOL *putsym(const char *name);
SYMBOL *argapp(SYMBOL * head, SYMBOL * arg);
int main(int argc, char **argv);


SYMBOL *symtab = NULL;
int parseerror = 0;

void freesym(const void *elt)
{
    SYMBOL *sym;

    sym = (SYMBOL *) elt;

    if (sym->name)
	G_free(sym->name);
    if (sym->proto)
	G_free(sym->proto);

    if (sym->itype == st_pnt) {
	VECTOR *vec, *next;

	for (vec = (VECTOR *) sym->v.p; vec;) {
	    next = vec->next;
	    if (--vec->refcnt < 1)
		G_free(vec);
	    vec = next;
	}
    }
    else if (sym->itype == st_map) {
	MAP *map, *next;

	for (map = (MAP *) sym->v.p; map;) {
	    next = map->next;
	    if (--map->refcnt < 1)
		G_free(map);	/* more needs to be done! */
	    map = next;
	}
    }
    else if (sym->itype == st_any) {
	ANY *any, *next;

	for (any = (ANY *) sym->v.p; any;) {
	    next = any->next;
	    if (--any->refcnt < 1)
		G_free(any);
	    any = next;
	}
    }
    else if (sym->itype == st_str && sym->v.p)
	G_free(sym->v.p);
}

SYMBOL *delsym(SYMBOL * head)
{
    return (SYMBOL *) listdelall((LIST *) head, freesym);
}

int cmpsymname(const void *data, const void *elt)
{
    SYMBOL *sym;
    const char *name;

    sym = (SYMBOL *) elt;
    name = (const char *)data;

    return strcmp(name, sym->name);
}

int cmpsymsym(const void *A, const void *B)
{
    SYMBOL *a, *b;

    a = (SYMBOL *) A;
    b = (SYMBOL *) B;

    return strcmp(b->name, a->name);
}

void symcpy(const void *Dst, const void *Src)
{
    SYMBOL *dst, *src;

    dst = (SYMBOL *) Dst;
    src = (SYMBOL *) Src;

    dst->type = src->type;
    dst->itype = src->itype;
    dst->rettype = src->rettype;
    if (src->name)
	dst->name = strdup(src->name);
    if (src->proto)
	dst->proto = strdup(src->proto);
    if (src->itype == st_str && src->v.p)
	dst->v.p = strdup(src->v.p);
    else {
	if (src->itype == st_pnt)
	    ((VECTOR *) src->v.p)->refcnt++;
	else if (src->itype == st_map)
	    ((MAP *) src->v.p)->refcnt++;
	else if (src->itype == st_any)
	    ((ANY *) src->v.p)->refcnt++;
	memcpy(&dst->v, &src->v, sizeof(src->v));
    }
}


SYMBOL *getsym(const char *name)
{
    return (SYMBOL *) listbsearchdatum((LIST *) symtab, name, cmpsymname);
}

SYMBOL *putsym(const char *name)
{
    SYMBOL *sym;

    sym = getsym(name);
    if (sym) {
	symtab = (SYMBOL *) listunlink((LIST *) symtab, (LIST *) sym);
	freesym(sym);
    }
    else
	sym = (SYMBOL *) listitem(sizeof(SYMBOL));

    sym->name = strdup(name);
    symtab = (SYMBOL *) listadd((LIST *) symtab, (LIST *) sym, cmpsymsym);

    return sym;
}

SYMBOL *argapp(SYMBOL * head, SYMBOL * arg)
{
    return (SYMBOL *) listapp((LIST *) head, (LIST *) arg);
}

int main(int argc, char **argv)
{
    init_num();
    init_map();
    init_plug();
    init_vec();
    init_any();

    yyparse();

    exit(0);
}
