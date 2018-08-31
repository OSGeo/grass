#include <grass/gis.h>
#include <grass/glocale.h>

char *explain_perms(int group, int other, int will)
{
    static char buf[256];
    char *who;
    char *verb;
    char *read;

    verb = _("have");
    read = _("read ");
    read = "";			/* remove this to have "read" appear */
    if (group && other) {
	who = _("Everyone");
	verb = _("has");
    }
    else if (group) {
	who = _("Only users in your group");
    }
    else if (other) {
	who = _("Only users outside your group");
    }
    else {
	who = _("Only you");
	read = "";
    }
    if (will)
	verb = _("have");

    sprintf(buf, _("%s %s %s %saccess to mapset %s"),
	    who, will ? _("will") : _("now"), verb, read, G_mapset());
    return buf;
}
