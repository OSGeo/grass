#include <stdlib.h>
#include <grass/gis.h>


/*!
 * \brief free color structure memory
 *
 * The dynamically allocated memory associated with the <b>colors</b>
 * structure is freed.
 * <b>Note.</b> This routine may be used after <i>G_read_colors</i> as
 * well as after <i>G_init_colors.</i>
 *
 *  \param colors
 *  \return int
 */

int G_free_colors(struct Colors *colors)
{
    G__color_reset(colors);
    G_init_colors(colors);

    return 0;
}

/*******************************************
 * G__color* routines only to be used by other routines in this
 * library
 *******************************************/

int G__color_free_rules(struct _Color_Info_ *cp)
{
    struct _Color_Rule_ *rule, *next;

    for (rule = cp->rules; rule; rule = next) {
	next = rule->next;
	G_free(rule);
    }
    cp->rules = NULL;

    return 0;
}

int G__color_free_lookup(struct _Color_Info_ *cp)
{
    if (cp->lookup.active) {
	G_free(cp->lookup.red);
	G_free(cp->lookup.blu);
	G_free(cp->lookup.grn);
	G_free(cp->lookup.set);
	cp->lookup.active = 0;
    }

    return 0;
}

int G__color_free_fp_lookup(struct _Color_Info_ *cp)
{
    if (cp->fp_lookup.active) {
	G_free(cp->fp_lookup.vals);
	G_free(cp->fp_lookup.rules);
	cp->fp_lookup.active = 0;
	cp->fp_lookup.nalloc = 0;
    }

    return 0;
}

int G__color_reset(struct Colors *colors)
{
    G__color_free_lookup(&colors->fixed);
    G__color_free_lookup(&colors->modular);
    G__color_free_rules(&colors->fixed);
    G__color_free_rules(&colors->modular);
    colors->version = 0;
    /* this routine should NOT init the colors */

    return 0;
}
