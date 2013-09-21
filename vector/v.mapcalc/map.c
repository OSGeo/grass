
/****************************************************************************
 *
 * MODULE:       v.mapcalc
 * AUTHOR(S):    Christoph Simon (original contributor)
 *               
 * PURPOSE:      
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "list.h"
#include "mapcalc.h"
#include "map.h"

typedef MAP *(*m_func) (void);
typedef MAP *(*m_func_m) (void *m0);
typedef MAP *(*m_func_mm) (void *m0, void *m1);

typedef struct Mapfunc
{
    char *name;
    void *func;
    char *proto;
} MAPFUNC;

static MAP *testmap(MAP * m);
static MAP *test2map(MAP * m, MAP * n);
static MAP *map_op_func_plus(MAP * m, MAP * n);
static void find_maps(void);
void init_map(void);
void showmap(SYMBOL * map);
void setmap(SYMBOL * var, SYMBOL * map);
SYMBOL *mkmapvar(SYMBOL * var, SYMBOL * map);
SYMBOL *mapfunc(SYMBOL * func, SYMBOL * arglist);
SYMBOL *mapop(int op, SYMBOL * map1, SYMBOL * map2);

static MAPFUNC mf[] = {
    {"testmap", testmap, "m"},
    {"test2map", test2map, "mm"},
    {"map_op_func_+", map_op_func_plus, "mm"},
    {NULL, NULL, NULL}
};

/*************************************************************************
 * This function represents a built-in map function.
 * Should probably go to another file
 */
static MAP *testmap(MAP * m)
{
    char namebuf[128];

    /*
     * The map name always exists, as it represents data on disk, but
     * might be a temporary name, when it should not be displayed
     * (should it?).
     */
    G_message(_("Performing 1 arg map function on map %s"), m->name);
    sprintf(namebuf, "t-%s", m->name);
    m = (MAP *) listitem(sizeof(MAP));
    m->name = strdup(namebuf);
    return m;
}

static MAP *test2map(MAP * m, MAP * n)
{
    char namebuf[128];

    G_message(_("Performing 2 arg map function on maps %s and %s"), m->name,
	      n->name);
    sprintf(namebuf, "%s.%s", m->name, n->name);
    m = (MAP *) listitem(sizeof(MAP));
    m->name = strdup(namebuf);
    return m;
}

static MAP *map_op_func_plus(MAP * m, MAP * n)
{
    char namebuf[128];

    G_message(_("Performing map %s + %s"), m->name, n->name);
    sprintf(namebuf, "%s.%s", m->name, n->name);
    m = (MAP *) listitem(sizeof(MAP));
    m->name = strdup(namebuf);
    return m;
}

/*
 * end of move to other file
 ***********************************************************************/

static void find_maps(void)
{
    char *gisdbase, *location, *mapset;
    char basepath[4096], subdirpath[4096], path[4096];
    DIR *dir, *subdir;
    struct dirent *ent, *subent;

    gisdbase = getenv("GISDBASE");
    if (!gisdbase)
	gisdbase = ".";

    location = getenv("LOCATION_NAME");
    if (!location)
	location = ".";

    mapset = getenv("MAPSET");
    if (!mapset)
	mapset = ".";

    /*
     * Now, if I'm not in grass, I can simulate the existence of a vector
     * map creating a directory vector with one subdirectory for each `map'
     * having a file `head'
     */

    sprintf(basepath, "%s/%s/%s/vector", gisdbase, location, mapset);
    dir = opendir(basepath);
    if (!dir)
	return;

    while ((ent = readdir(dir)) != NULL) {
	struct stat buf;

	if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
	    continue;

	strcpy(path, basepath);
	strcat(path, "/");
	strcat(path, ent->d_name);

	if (stat(path, &buf))
	    continue;
	if (S_ISDIR(buf.st_mode)) {
	    strcpy(subdirpath, path);
	    subdir = opendir(subdirpath);
	    if (!subdir)
		continue;
	    while ((subent = readdir(subdir)) != NULL) {
		if (!strcmp(subent->d_name, ".") ||
		    !strcmp(subent->d_name, ".."))
		    continue;
		if (!strcmp(subent->d_name, "head")) {
		    MAP *map;
		    SYMBOL *sym;

		    map = (MAP *) listitem(sizeof(MAP));
		    map->name = strdup(ent->d_name);
		    map->refcnt++;
		    sym = putsym(map->name);
		    sym->type = sym->itype = st_map;
		    sym->v.p = map;
		}
	    }
	    closedir(subdir);
	}
    }
    closedir(dir);
}

void init_map(void)
{
    SYMBOL *sym;
    int i;

    for (i = 0; mf[i].name; i++) {
	sym = putsym(mf[i].name);
	sym->type = sym->itype = st_mfunc;
	sym->v.p = mf[i].func;
	sym->proto = mf[i].proto;
	sym->rettype = st_map;
    }

    find_maps();
}

void printmap(SYMBOL * sym)
{
    MAP *map;

    map = (MAP *) sym->v.p;
    if (map->name)
	fprintf(stdout, "\t%s\n", map->name);
}

void showmap(SYMBOL * sym)
{
    MAP *map;

    map = (MAP *) sym->v.p;
    printmap(sym);
    if (--map->refcnt > 0)
	sym->v.p = NULL;
    freesym(sym);
}

MAP *freemap(MAP * map)
{
    if (map) {
	if (map->refcnt > 0)
	    return map;
	if (map->name)
	    G_free(map->name);
	if (map->mapinfo)
	    G_free(map->mapinfo);	/* call grass to handle map */
	G_free(map);
    }
    return NULL;
}

void setmap(SYMBOL * var, SYMBOL * map)
{
    SYMBOL *sym;

    if (var->name) {
	sym = getsym(var->name);
	if (sym) {
	    sym->v.p = freemap(sym->v.p);
	    sym->v.p = map->v.p;
	}
    }

    freemap(var->v.p);
    var->v.p = NULL;
    freesym(var);

    printmap(map);
    map->v.p = NULL;
    freesym(map);
}

SYMBOL *mkmapvar(SYMBOL * var, SYMBOL * map)
{
    var->type = var->itype = st_map;
    var->name = var->v.p;
    var->v.p = map->v.p;
    map->v.p = NULL;
    freesym(map);

    symtab = (SYMBOL *) listadd((LIST *) symtab, (LIST *) var, cmpsymsym);

    printmap(var);

    return var;
}

SYMBOL *mapfunc(SYMBOL * func, SYMBOL * arglist)
{
    SYMBOL *sym;
    MAP *res = NULL;
    int argc = -1;

    if (!func || !func->v.p || func->type != st_mfunc) {
	parseerror = 1;
	G_warning(_("Can't call bad map-function"));
    }
    else
	argc = listcnt((LIST *) arglist);

    if (argc == 0 && (!func->proto || !*func->proto))
	res = (*(m_func) func->v.p) ();
    else if (argc == 1 && !strcmp(func->proto, "m"))
	res = (*(m_func_m) func->v.p) (arglist->v.p);
    else if (argc == 2 && !strcmp(func->proto, "mm"))
	res = (*(m_func_mm) func->v.p) (arglist->v.p, arglist->next->v.p);
    else {
	G_warning(_("Bad arguments to mapfunc %s (argc = %d)"), func->name,
		  argc);
	parseerror = 1;
    }

    listdelall((LIST *) func, freesym);
    listdelall((LIST *) arglist, freesym);

    sym = (SYMBOL *) listitem(sizeof(SYMBOL));
    sym->type = st_map;
    sym->v.p = res;

    return sym;
}

SYMBOL *mapop(int op, SYMBOL * map1, SYMBOL * map2)
{
    SYMBOL *func, *arglist, *res = NULL;
    char buf[32];

    sprintf(buf, "map_op_func_%c", op);

    func = getsym(buf);
    if (!func) {
	G_warning(_("No function defined to perform map %c map"), op);
	parseerror = 1;
    }
    else {
	res = (SYMBOL *) listitem(sizeof(SYMBOL));
	symcpy(res, func);
	res->next = NULL;
	func = res;
	arglist = (SYMBOL *) listapp(NULL, (LIST *) map1);
	arglist = (SYMBOL *) listapp((LIST *) arglist, (LIST *) map2);

	res = mapfunc(func, arglist);
    }

    /* free map1/map2 ?    only if they have no name? */

    return res;
}
