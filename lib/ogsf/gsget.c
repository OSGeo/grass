/*
* $Id$
*/

#include <grass/gstypes.h>

int get_mapatt(typbuff * buff, int offset, float *att)
{
    if (buff->nm) {
	if (BM_get
	    (buff->nm, (offset % buff->nm->cols),
	     (offset / buff->nm->cols))) {
	    return (0);
	}
    }

    *att = (buff->ib ? (float) buff->ib[offset] :
	    buff->sb ? (float) buff->sb[offset] :
	    buff->cb ? (float) buff->cb[offset] :
	    buff->fb ? (float) buff->fb[offset] : buff->k);

    if (buff->tfunc) {
	*att = (buff->tfunc) (*att, offset);
    }

    return (1);
}
