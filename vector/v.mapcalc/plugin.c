#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include "list.h"
#include "mapcalc.h"

typedef char *(*func_t) (void);

static int register_function(char *fname, void *func, char *proto);
void init_plug(void);

static int register_function(char *fname, void *func, char *proto)
{
    STYP type;
    SYMBOL *sym;

    sym = getsym(fname);
    if (sym)
	symtab = (SYMBOL *) listdel((LIST *) symtab, (LIST *) sym, freesym);

    /*
     * This is quite incomplete
     */

    switch (proto[0]) {
    case 'd':
	type = st_nfunc;
	break;
    case 'm':
	type = st_mfunc;
	break;
    case 'p':
	type = st_pfunc;
	break;
    default:
	return 0;
    }

    sym = putsym(fname);
    sym->v.p = func;
    sym->type = sym->itype = sym->rettype = type;
    sym->proto = strdup(proto + 2);	/* skip "m=" */

    return 1;
}

void init_plug(void)
{
    SYMBOL *sym;
    char path[4096], pathname[4096], *ptr, *fname, *proto;
    struct dirent *ent;
    void *handle, *func;
    func_t fh, ph;
    DIR *dir;

    /*
     * Search algorithm:
     * 1. Find "pluginpath" in symbol table. If not found...
     * 2. Find "$GISBASE/plugins". If not defined
     * 3. Use "./plugins"
     */

    sym = getsym("pluginpath");
    if (sym && sym->v.p)
	strcpy(path, sym->v.p);
    else {
	ptr = getenv("GISBASE");
	if (ptr)
	    strcpy(path, ptr);
	else
	    strcpy(path, getcwd(path, 4096));
	if (path[strlen(path) - 1] != '/')
	    strcat(path, "/");
	strcat(path, "plugins");
    }

    dir = opendir(path);
    if (!dir)
	return;

    while ((ent = readdir(dir)) != NULL) {
	if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
	    continue;
	strcpy(pathname, path);
	strcat(pathname, "/");
	strcat(pathname, ent->d_name);
	if (access(pathname, R_OK | X_OK))
	    continue;
	handle = dlopen(pathname, RTLD_LAZY);
	if (!handle)
	    continue;
	fh = dlsym(handle, "fname");
	if (dlerror()) {
	    dlclose(handle);
	    continue;
	}
	fname = (*fh) ();
	if (!fname) {
	    dlclose(handle);
	    continue;
	}
	ph = dlsym(handle, "proto");
	if (dlerror()) {
	    dlclose(handle);
	    continue;
	}
	proto = (*ph) ();
	if (!proto) {
	    dlclose(handle);
	    continue;
	}
	func = dlsym(handle, fname);
	if (!func) {
	    dlclose(handle);
	    continue;
	}
	if (!register_function(fname, func, proto))
	    dlclose(handle);
    }
    closedir(dir);
}
