#include <grass/Vect.h>
#include <grass/glocale.h>
#include "conv.h"

/* conversion of old file format elment type codes to new 
 *
 *  returns: new type
 *           0 - dead element
 *          -1 - error
 */
char dig_old_to_new_type(char type)
{
    switch (type) {
    case FILE_LINE:
	type = GV_LINE;
	break;
    case FILE_AREA:
	type = GV_BOUNDARY;
	break;
    case FILE_DOT:
	type = GV_POINT;
	break;
    case FILE_DEAD_LINE:
    case FILE_DEAD_AREA:
    case FILE_DEAD_DOT:
	type = 0;
	break;
    default:
	G_warning(_("OLD_T_NEW Got a bad type code [%x]"), type);
	type = -1;
	break;
    }
    return (type);
}

/* conversion of new element types to old file format elment type codes */
char dig_new_to_old_type(char type)
{
    switch (type) {
    case GV_LINE:
	type = FILE_LINE;
	break;
    case GV_BOUNDARY:
	type = FILE_AREA;
	break;
    case GV_POINT:
	type = FILE_DOT;
	break;
    default:
	G_warning(_("NEW_T_OLD Got a bad type code [%x]"), type);
	type = 0;
	break;
    }
    return (type);
}
