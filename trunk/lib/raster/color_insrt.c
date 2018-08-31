/* This routine is public only because source is in different files.
 * It should NEVER be called directly.
 * It is used by Rast_add_c_color_rule() and G__read_old_colors().
 * These routines know when it is appropriate to call this routine.
 */
#include <grass/gis.h>
#include <grass/raster.h>

#define umalloc(n) (unsigned char *) G_malloc((size_t)n)
#define urealloc(s,n) (unsigned char *) G_realloc(s,(size_t)n)

#define LIMIT(x) if (x < 0) x = 0; else if (x > 255) x = 255;

int Rast__insert_color_into_lookup(CELL cat,
				   int red, int grn, int blu,
				   struct _Color_Info_ *cp)
{
    long nalloc;
    long i;
    long newlen, curlen, gap;

    LIMIT(red);
    LIMIT(grn);
    LIMIT(blu);

    /* first color? */
    if (!cp->lookup.active) {
	cp->lookup.active = 1;
	cp->lookup.nalloc = 256;
	cp->lookup.red = umalloc(cp->lookup.nalloc);
	cp->lookup.grn = umalloc(cp->lookup.nalloc);
	cp->lookup.blu = umalloc(cp->lookup.nalloc);
	cp->lookup.set = umalloc(cp->lookup.nalloc);
	cp->max = cp->min = cat;
    }

    /* extend the color table? */
    else if (cat > cp->max) {
	curlen = cp->max - cp->min + 1;
	newlen = cat - cp->min + 1;
	nalloc = newlen;
	if (nalloc != (int)nalloc)	/* check for int overflow */
	    return -1;

	if (nalloc > cp->lookup.nalloc) {
	    while (cp->lookup.nalloc < nalloc)
		cp->lookup.nalloc += 256;
	    nalloc = cp->lookup.nalloc;

	    cp->lookup.red = urealloc((char *)cp->lookup.red, nalloc);
	    cp->lookup.grn = urealloc((char *)cp->lookup.grn, nalloc);
	    cp->lookup.blu = urealloc((char *)cp->lookup.blu, nalloc);
	    cp->lookup.set = urealloc((char *)cp->lookup.set, nalloc);
	}

	/* fill in gap with white */
	for (i = curlen; i < newlen; i++) {
	    cp->lookup.red[i] = 255;
	    cp->lookup.grn[i] = 255;
	    cp->lookup.blu[i] = 255;
	    cp->lookup.set[i] = 0;
	}
	cp->max = cat;
    }
    else if (cat < cp->min) {
	curlen = cp->max - cp->min + 1;
	newlen = cp->max - cat + 1;
	gap = newlen - curlen;
	nalloc = newlen;
	if (nalloc != (int)nalloc)	/* check for int overflow */
	    return -1;

	if (nalloc > cp->lookup.nalloc) {
	    while (cp->lookup.nalloc < nalloc)
		cp->lookup.nalloc += 256;
	    nalloc = cp->lookup.nalloc;

	    cp->lookup.red = urealloc((char *)cp->lookup.red, nalloc);
	    cp->lookup.grn = urealloc((char *)cp->lookup.grn, nalloc);
	    cp->lookup.blu = urealloc((char *)cp->lookup.blu, nalloc);
	    cp->lookup.set = urealloc((char *)cp->lookup.set, nalloc);
	}

	/* shift the table to make room in front */
	for (i = 1; i <= curlen; i++) {
	    cp->lookup.red[newlen - i] = cp->lookup.red[curlen - i];
	    cp->lookup.grn[newlen - i] = cp->lookup.grn[curlen - i];
	    cp->lookup.blu[newlen - i] = cp->lookup.blu[curlen - i];
	    cp->lookup.set[newlen - i] = cp->lookup.set[curlen - i];
	}

	/* fill in gap with white */
	for (i = 1; i < gap; i++) {
	    cp->lookup.red[i] = 255;
	    cp->lookup.grn[i] = 255;
	    cp->lookup.blu[i] = 255;
	    cp->lookup.set[i] = 0;
	}
	cp->min = cat;
    }

    /* set the color! */
    i = cat - cp->min;
    cp->lookup.red[i] = red;
    cp->lookup.grn[i] = grn;
    cp->lookup.blu[i] = blu;
    cp->lookup.set[i] = 1;

    return 1;
}
