#include <grass/gis.h>
char *explain_perms(int group, int other, int will)
{
    static char buf[128];
    char *who;
    char *verb;
    char *read;

    verb = "have";
    read = "read ";
    read = "";			/* remove this to have "read" appear */
    if (group && other) {
	who = "Everyone";
	verb = "has";
    }
    else if (group) {
	who = "Only users in your group";
    }
    else if (other) {
	who = "Only users outside your group";
    }
    else {
	who = "Only you";
	read = "";
    }
    if (will)
	verb = "have";

    sprintf(buf, "%s %s %s %saccess to mapset %s",
	    who, will ? "will" : "now", verb, read, G_mapset());
    return buf;
}
