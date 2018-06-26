/*!
 \file lib/gis/color_rules.c
 
 \brief GIS Library - Color tables management subroutines

 Taken from r.colors module.

 (C) 2001-2011 by the GRASS Development Team
*/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

struct colorinfo
{
    char *name;
    char *desc;
    char *type;
};

static struct colorinfo *get_colorinfo(int *);
static void free_colorinfo(struct colorinfo *, int);

static int cmp_clrname(const void *a, const void *b)
{
    struct colorinfo *ca = (struct colorinfo *) a;
    struct colorinfo *cb = (struct colorinfo *) b;

    return (strcmp(ca->name, cb->name));
}

/*!
  \brief Get list of color rules for Option->options
  
  \return allocated string buffer with options
*/
char *G_color_rules_options(void)
{
    char *list;
    const char *name;
    int size, len, nrules;
    int i, n;
    struct colorinfo *colorinfo;

    list = NULL;
    size = len = 0;

    colorinfo = get_colorinfo(&nrules);
    
    for (i = 0; i < nrules; i++) {
        name = colorinfo[i].name;
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

    free_colorinfo(colorinfo, nrules);
    
    return list;
}

/*!
  \brief Get color rules description for Option->descriptions

  \return allocated buffer with descriptions
*/
char *G_color_rules_descriptions(void)
{
    int result_len, result_max;
    char *result;
    const char *name, *desc;
    int i, len, nrules;
    struct colorinfo *colorinfo;

    result_len = 0;
    result_max = 2000;
    result = G_malloc(result_max);

    colorinfo = get_colorinfo(&nrules);
    
    for (i = 0; i < nrules; i++) {
        name = colorinfo[i].name;
        desc = colorinfo[i].desc;
        
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

    free_colorinfo(colorinfo, nrules);
    
    return result;
}

/*!
  \brief Get color rules description for Option->descriptions

  The type of color rule including range is appended to the description 

  \return allocated buffer with name, description, and type
*/
char *G_color_rules_description_type(void)
{
    int i, len, nrules;
    struct colorinfo *colorinfo;
    const char *name, *desc, *type;
    int result_len, result_max;
    char *result;
    
    colorinfo = get_colorinfo(&nrules);

    result_len = 0;
    result_max = 2000;
    result = G_malloc(result_max);

    for (i = 0; i < nrules; i++) {
        name = colorinfo[i].name;
        desc = colorinfo[i].desc;
        type = colorinfo[i].type;

	if (desc) {
	    len = strlen(name) + strlen(desc) + strlen(type) + 5;
	    if (result_len + len >= result_max) {
		result_max = result_len + len + 1000;
		result = G_realloc(result, result_max);
	    }

	    sprintf(result + result_len, "%s;%s [%s];", name, desc, type);
	    result_len += len;
	}
	else {
	    len = strlen(name) + strlen(type) + 5;
	    if (result_len + len >= result_max) {
		result_max = result_len + len + 1000;
		result = G_realloc(result, result_max);
	    }

	    sprintf(result + result_len, "%s; [%s];", name, type);
	    result_len += len;
	}
    }

    free_colorinfo(colorinfo, nrules);
    
    return result;
}

/*!
  \brief Print color rules

  \param out file where to print
*/
void G_list_color_rules(FILE *out)
{
    int i, nrules;
    struct colorinfo *colorinfo;

    colorinfo = get_colorinfo(&nrules);

    for (i = 0; i < nrules; i++)
	fprintf(out, "%s\n", colorinfo[i].name);

    free_colorinfo(colorinfo, nrules);
}

/*!
  \brief Print color rules with description and type

  The type of color rule including range is appended to the description. 
  If a color rule name is given, color info is printed only for this 
  rule.

  \param name optional color rule name, or NULL
  \param out file where to print
*/
void G_list_color_rules_description_type(FILE *out, char *name)
{
    int i, nrules;
    struct colorinfo *colorinfo, csearch, *cfound;
    
    colorinfo = get_colorinfo(&nrules);

    cfound = NULL;
    if (name) {
	csearch.name = name;
	cfound = bsearch(&csearch, colorinfo, nrules,
			 sizeof(struct colorinfo), cmp_clrname);

	if (cfound) {
	    if (cfound->desc) {
		fprintf(out, "%s: %s [%s]\n", cfound->name,
			cfound->desc, cfound->type);
	    }
	    else {
		fprintf(out, "%s: [%s]\n", cfound->name,
			cfound->type);
	    }
	}
    }

    if (cfound == NULL) {
	for (i = 0; i < nrules; i++) {
	    if (colorinfo[i].desc) {
		fprintf(out, "%s: %s [%s]\n", colorinfo[i].name,
			colorinfo[i].desc, colorinfo[i].type);
	    }
	    else {
		fprintf(out, "%s: [%s]\n", colorinfo[i].name,
			colorinfo[i].type);
	    }
	}
    }

    free_colorinfo(colorinfo, nrules);
}

/*!
  \brief Check if color rule is defined

  \param name color rule name

  \return 1 found
  \return 0 not found
*/
int G_find_color_rule(const char *name)
{
    int result, nrules;
    struct colorinfo *colorinfo, csearch;
    
    colorinfo = get_colorinfo(&nrules);

    csearch.name = (char *)name;
    result = (bsearch(&csearch, colorinfo, nrules,
		      sizeof(struct colorinfo), cmp_clrname) != NULL);

    free_colorinfo(colorinfo, nrules);

    return result;
}

struct colorinfo *get_colorinfo(int *nrules)
{
    int i;
    char path[GPATH_MAX];
    FILE *fp;
    struct colorinfo *colorinfo;
    char **cnames;

    /* load color rules */
    G_snprintf(path, GPATH_MAX, "%s/etc/colors", G_gisbase());

    *nrules = 0;
    cnames = G_ls2(path, nrules);
    (*nrules) += 3;
    colorinfo = G_malloc(*nrules * sizeof(struct colorinfo));
    for (i = 0; i < *nrules - 3; i++) {
	char buf[1024];
	double rmin, rmax;
	int first;
	int cisperc;

	colorinfo[i].name = G_store(cnames[i]);
	colorinfo[i].desc = NULL;
	
	/* open color rule file */
	G_snprintf(path, GPATH_MAX, "%s/etc/colors/%s", G_gisbase(),
		   colorinfo[i].name);
	fp = fopen(path, "r");
	if (!fp)
	    G_fatal_error(_("Unable to open color rule"));
	
	/* scan all lines */
	first = 1;
	rmin = rmax = 0;
	cisperc = 0;
	while (G_getl2(buf, sizeof(buf), fp)) {
	    char value[80], color[80];
	    double x;
	    char c;

	    G_strip(buf);

	    if (*buf == '\0')
		continue;
	    if (*buf == '#')
		continue;

	    if (sscanf(buf, "%s %[^\n]", value, color) != 2)
		continue;

	    if (G_strcasecmp(value, "default") == 0) {
		continue;
	    }

	    if (G_strcasecmp(value, "nv") == 0) {
		continue;
	    }

	    if (sscanf(value, "%lf%c", &x, &c) == 2 && c == '%') {
		cisperc = 1;
		break;
	    }
	    if (sscanf(value, "%lf", &x) == 1) {
		if (first) {
		    first = 0;
		    rmin = rmax = x;
		}
		else {
		    if (rmin > x)
			rmin = x;
		    if (rmax < x)
			rmax = x;
		}
	    }
	}
	fclose(fp);

	if (cisperc)
	    colorinfo[i].type = G_store(_("range: map values"));
	else {
	    G_snprintf(buf, sizeof(buf) - 1, _("range: %g to %g"), rmin, rmax);
	    colorinfo[i].type = G_store(buf);
	}
    }
    G_free(cnames);

    /* colors without rules but description */
    colorinfo[*nrules - 3].name = G_store("random");
    colorinfo[*nrules - 3].desc = NULL;
    colorinfo[*nrules - 3].type = G_store(_("range: map values"));

    colorinfo[*nrules - 2].name = G_store("grey.eq");
    colorinfo[*nrules - 2].desc = NULL;
    colorinfo[*nrules - 2].type = G_store(_("range: map values"));

    colorinfo[*nrules - 1].name = G_store("grey.log");
    colorinfo[*nrules - 1].desc = NULL;
    colorinfo[*nrules - 1].type = G_store(_("range: map values"));

    qsort(colorinfo, *nrules, sizeof(struct colorinfo), cmp_clrname);

    /* load color descriptions */
    G_snprintf(path, GPATH_MAX, "%s/etc/colors.desc", G_gisbase());
    fp = fopen(path, "r");
    if (!fp)
	G_fatal_error(_("Unable to open color descriptions"));

    for (;;) {
	char buf[1024];
	char tok_buf[1024];
	char *cname, *cdesc;
	int ntokens;
	char **tokens;
	struct colorinfo csearch, *cfound;

	if (!G_getl2(buf, sizeof(buf), fp))
	    break;
	strcpy(tok_buf, buf);
	tokens = G_tokenize(tok_buf, ":");
	ntokens = G_number_of_tokens(tokens);
	if (ntokens != 2)
	    continue;

	cname = G_chop(tokens[0]);
	cdesc = G_chop(tokens[1]);
	
	csearch.name = cname;
	cfound = bsearch(&csearch, colorinfo, *nrules,
			 sizeof(struct colorinfo), cmp_clrname);

	if (cfound) {
	    cfound->desc = G_store(cdesc);
	}
	G_free_tokens(tokens);
    }
    fclose(fp);

    return colorinfo;
}

void free_colorinfo(struct colorinfo *colorinfo, int nrules)
{
    int i;

    for (i = 0; i < nrules; i++) {
	if (colorinfo[i].name)
	    G_free(colorinfo[i].name);
	if (colorinfo[i].desc)
	    G_free(colorinfo[i].desc);
	if (colorinfo[i].type)
	    G_free(colorinfo[i].type);
    }
    if (nrules > 0)
	G_free(colorinfo);
}
