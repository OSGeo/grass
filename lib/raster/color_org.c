#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>

#define LOOKUP_COLORS 2048

static void organize_lookup(struct Colors *, int);
static int organize_fp_lookup(struct Colors *, int);
static int double_comp(const void *, const void *);

void Rast__organize_colors(struct Colors *colors)
{
    /* don't do anything if called recursively */
    if (!colors->organizing) {
	colors->organizing = 1;

	organize_lookup(colors, 0);
	organize_lookup(colors, 1);

	organize_fp_lookup(colors, 0);
	organize_fp_lookup(colors, 1);

	colors->organizing = 0;
    }
}

static int organize_fp_lookup(struct Colors *colors, int mod)
{
    int i;
    DCELL val;
    struct _Color_Info_ *cp;
    struct _Color_Rule_ *rule;

    if (mod)
	cp = &colors->modular;
    else
	cp = &colors->fixed;

    /* if one of the lookup tables exist, don't do anything */
    if (cp->lookup.active || cp->fp_lookup.active)
	return 1;
    if (cp->n_rules == 0)
	return 1;

    cp->fp_lookup.vals = (DCELL *)
	G_calloc(cp->n_rules * 2, sizeof(DCELL));
    /* 2 endpoints for each rule */
    cp->fp_lookup.rules = (struct _Color_Rule_ **)
	G_calloc(cp->n_rules * 2, sizeof(struct _Color_Rule_ *));

    /* get the list of DCELL values from set of all lows and highs
       of all rules */
    /* NOTE: if low==high in a rule, the value appears twice in a list
       but if low==high of the previous, rule the value appears only once */

    i = 0;
    /* go through the list of rules from end to beginning, 
       because rules are sored in reverse order of reading,
       and we want to read the in correct order, to ignore
       the same values in the end of rule and beginning of next rule */

    /* first go to the last rules */
    for (rule = cp->rules; rule->next; rule = rule->next) ;
    /* now traverse from the last to the first rule */
    for (; rule; rule = rule->prev) {
	/* check if the min is the same as previous maximum */
	if (i == 0 || rule->low.value != cp->fp_lookup.vals[i - 1])
	    cp->fp_lookup.vals[i++] = rule->low.value;
	cp->fp_lookup.vals[i++] = rule->high.value;
    }
    cp->fp_lookup.nalloc = i;

    /* now sort the values */
    qsort((char *)cp->fp_lookup.vals, cp->fp_lookup.nalloc,
	  sizeof(DCELL), &double_comp);

    /* now find the rule to apply between each 2 values in a list */
    for (i = 0; i < cp->fp_lookup.nalloc - 1; i++) {
	val = (cp->fp_lookup.vals[i] + cp->fp_lookup.vals[i + 1]) / 2.;
	/* fprintf (stderr, "%lf %lf ", cp->fp_lookup.vals[i], cp->fp_lookup.vals[i+1]); */

	for (rule = cp->rules; rule; rule = rule->next)
	    if (rule->low.value <= val && val <= rule->high.value)
		break;
	/* if(rule) fprintf (stderr, "%d %lf %lf %d\n", i, rule->low.value, rule->high.value, rule);
	   else fprintf (stderr, "null\n");
	 */
	cp->fp_lookup.rules[i] = rule;
    }
    cp->fp_lookup.active = 1;

    return 0;
}

static void organize_lookup(struct Colors *colors, int mod)
{
    int i, n;
    CELL x;
    CELL cat[LOOKUP_COLORS];
    struct _Color_Info_ *cp;

    /* don't do anything if the color structure is float */
    if (colors->is_float)
	return;

    if (mod)
	cp = &colors->modular;
    else
	cp = &colors->fixed;

    if (cp->lookup.active)
	return;

    n = (CELL) cp->max - (CELL) cp->min + 1;
    if (n >= LOOKUP_COLORS || n <= 0)
	return;

    x = (CELL) cp->min;
    for (i = 0; i < n; i++)
	cat[i] = x++;;

    cp->lookup.nalloc = n;
    cp->lookup.red = (unsigned char *)G_malloc(n);
    cp->lookup.grn = (unsigned char *)G_malloc(n);
    cp->lookup.blu = (unsigned char *)G_malloc(n);
    cp->lookup.set = (unsigned char *)G_malloc(n);

    G_zero(cp->lookup.set, n * sizeof(unsigned char));
    Rast__lookup_colors((void *)cat,
			cp->lookup.red, cp->lookup.grn, cp->lookup.blu,
			cp->lookup.set, n, colors, mod, 1, CELL_TYPE);

    cp->lookup.active = 1;
}

static int double_comp(const void *xx, const void *yy)
{
    const DCELL *x = xx, *y = yy;

    if (*x < *y)
	return -1;
    else if (*x == *y)
	return 0;
    else
	return 1;
}
