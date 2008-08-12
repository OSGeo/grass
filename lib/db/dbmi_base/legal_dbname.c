#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/* TODO: are we as restrictive here as for vector names? */

/*!
   \fn int db_legal_tablename (char *s)
   \brief  Check if output is legal table name
   \return 1 OK
   \return -1 if name does not start with letter A..Za..z
   or if name does not continue with A..Za..z0..9_@
   Rule:  [A-Za-z][A-Za-z0-9_@]*
   \param  name table name to be checked
 */

int db_legal_tablename(const char *s)
{
    char buf[GNAME_MAX];

    sprintf(buf, "%s", s);

    if (*s == '.' || *s == 0) {
	fprintf(stderr,
		_("Illegal table map name <%s>. May not contain '.' or 'NULL'.\n"),
		buf);
	return DB_FAILED;
    }

    /* file name must start with letter */
    if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z'))) {
	fprintf(stderr,
		_("Illegal table map name <%s>. Must start with a letter.\n"),
		buf);
	return DB_FAILED;
    }

    for (s++; *s; s++)
	if (!
	    ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') ||
	     (*s >= '0' && *s <= '9') || *s == '_' || *s == '@')) {
	    fprintf(stderr,
		    _("Illegal table map name <%s>. Character <%c> not allowed.\n"),
		    buf, *s);
	    return DB_FAILED;
	}

    return DB_OK;
}
