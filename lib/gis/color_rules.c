/*!
 \file lib/gis/color_tables.c
 
 \brief GIS Library - Color tables

 Taken from r.colors module.

 (C) 2001-2011 by the GRASS Development Team
*/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

static int scan_rules(char ***);
static int cmp(const void *, const void *);

/*!
  \brief Get list of color rules for Option->options
  
  \return allocated string buffer with options
*/
char *G_color_rules_list(void)
{
    char *list, **rules;
    const char *name;
    int size, len, nrules;
    int i, n;

    list = NULL;
    size = len = 0;

    nrules = scan_rules(&rules);
    
    for (i = 0; i < nrules; i++) {
        name = rules[i];
        n = strlen(name);

        if (size < len + n + 2) {
            size = len + n + 200;
            list = G_realloc(list, size);
        }

        if (len > 0)
            list[len++] = ',';

        memcpy(&list[len], name, n + 1);
        len += n;
    }

    G_free(rules);
    
    return list;
}

/*!
  \brief Get color rules description for Option->descriptions

  \return allocated buffer with descriptions
*/
char *G_color_rules_descriptions(void)
{
    char path[GPATH_MAX];
    struct Key_Value *kv;
    int result_len, result_max, nrules;
    char *result, **rules;
    const char *name, *desc;
    int i, len;

    result_len = 0;
    result_max = 2000;
    result = G_malloc(result_max);
    
    G_snprintf(path, GPATH_MAX, "%s/etc/colors.desc", G_gisbase());
    kv = G_read_key_value_file(path);
    if (!kv)
        return NULL;

    nrules = scan_rules(&rules);
    
    for (i = 0; i < nrules; i++) {
        name = rules[i];
        desc = G_find_key_value(name, kv);
        
        if (!desc)
	    desc = _("no description");
	
        /* desc = _(desc); */
	
        len = strlen(name) + strlen(desc) + 2;
        if (result_len + len >= result_max) {
            result_max = result_len + len + 1000;
            result = G_realloc(result, result_max);
        }

        sprintf(result + result_len, "%s;%s;", name, desc);
        result_len += len;
    }

    G_free_key_value(kv);
    G_free(rules);
    
    return result;
}

int scan_rules(char ***rules)
{
    int nrules;
    char path[GPATH_MAX];

    G_snprintf(path, GPATH_MAX, "%s/etc/colors", G_gisbase());

    *rules = G__ls(path, &nrules);

    *rules = G_realloc(*rules, (nrules + 3) * sizeof (const char *));

    (*rules)[nrules++] = G_store("random");
    (*rules)[nrules++] = G_store("grey.eq");
    (*rules)[nrules++] = G_store("grey.log");

    qsort(*rules, nrules, sizeof (char *), cmp);

    return nrules;
}

int cmp(const void *aa, const void *bb)
{
    char *const *a = (char *const *) aa;
    char *const *b = (char *const *) bb;

    return strcmp(*a, *b);
}
