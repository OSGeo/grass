#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

struct proj_unit *get_proj_unit(const char *arg)
{
    char buf[4096];
    struct proj_unit *unit;
    FILE *fp;

    sprintf(buf, "%s/etc/proj/units.table", G_gisbase());

    fp = fopen(buf, "r");
    if (!fp)
	return NULL;

    for (unit = NULL; !unit;) {
	char plural[16], singular[16];
	double factor;
	struct proj_unit *unit;

	if (!G_getl2(buf, sizeof(buf), fp))
	    break;

	if (sscanf(buf, "%[^:]:%[^:]:%lf", plural, singular, &factor) != 3)
	    continue;

	if (G_strcasecmp(arg, plural) != 0)
	    continue;

	unit = G_malloc(sizeof(struct proj_unit));

	unit->units = G_store(plural);
	unit->unit = G_store(singular);
	unit->fact = factor;
    }

    fclose(fp);

    return unit;
}

struct proj_desc *get_proj_desc(const char *arg)
{
    char buf[4096];
    struct proj_desc *res;
    FILE *fp;

    sprintf(buf, "%s/etc/proj/desc.table", G_gisbase());

    fp = fopen(buf, "r");
    if (!fp)
	return NULL;

    for (res = NULL; !res;) {
	char name[16], type[16], key[16], desc[100];

	if (!G_getl2(buf, sizeof(buf), fp))
	    break;

	if (sscanf(buf, "%[^:]:%[^:]:%[^:]:%[^\n]", name, type, key, desc) !=
	    4)
	    continue;

	if (G_strcasecmp(arg, name) != 0)
	    continue;

	res = G_malloc(sizeof(struct proj_desc));

	res->name = G_store(name);
	res->type = G_store(type);
	res->key = G_store(key);
	res->desc = G_store(desc);
    }

    fclose(fp);

    return res;
}

struct proj_parm *get_proj_parms(const char *arg)
{
    char buf[4096];
    struct proj_parm *parm_table = NULL;
    int parm_num = 0;
    int parm_max = 0;
    char *data;
    int done;
    FILE *fp;

    sprintf(buf, "%s/etc/proj/parms.table", G_gisbase());

    fp = fopen(buf, "r");
    if (!fp)
	return NULL;

    for (data = NULL; !data;) {
	char *p;

	if (!G_getl2(buf, sizeof(buf), fp))
	    break;

	for (p = buf; *p && *p != ':'; p++) ;

	if (*p != ':')
	    break;

	*p++ = '\0';

	if (G_strcasecmp(buf, arg) != 0)
	    continue;

	for (; *p && *p != ':'; p++) ;

	if (*p != ':')
	    break;

	*p++ = '\0';

	data = p;
    }

    fclose(fp);

    if (!data)
	return NULL;

    for (done = 0; !done;) {
	char name[16], ask[8], dfl[32];
	struct proj_parm *parm;
	char *p;

	for (p = data; *p && *p != ';'; p++) ;

	if (*p == ';')
	    *p++ = '\0';
	else
	    done = 1;

	if (sscanf(data, "%[^=]=%[^,],%s", name, ask, dfl) != 3) {
	    data = p;
	    continue;
	}

	data = p;

	if (parm_num + 1 >= parm_max) {
	    parm_max += 16;
	    parm_table =
		G_realloc(parm_table, parm_max * sizeof(struct proj_parm));
	}

	parm = &parm_table[parm_num++];

	parm->name = G_store(name);

	if (strcmp(ask, "ask") == 0)
	    parm->ask = 1;
	else if (strcmp(ask, "noask") == 0)
	    parm->ask = 0;
	else {
	    parm->ask = 1;
	    G_warning(_("Unrecognized 'ask' value in parms.table: %s"),
		      ask);
	}

	if (strcmp(dfl, "nodfl") == 0)
	    parm->def_exists = 0;
	else if (sscanf(dfl, "%lf", &parm->deflt) == 1)
	    parm->def_exists = 1;
	else {
	    parm->def_exists = 0;
	    G_warning(_("Unrecognized default value in parms.table: %s"),
		      dfl);
	}
    }

    parm_table[parm_num].name = NULL;

    return parm_table;
}
