#include <grass/gis.h>
#include <grass/glocale.h>

char *explain_perms(int group, int other, int will)
{
    static char buf[256];
    char *msg = _("Cannot explain access permissions for mapset <%s>");

    if (group && other) {
        msg = will ? _("Everyone will have access to mapset <%s>")
                   : _("Everyone now has access to mapset <%s>");
    }
    else if (group) {
        msg = will ? _("Only users in your group will have access to "
                       "mapset <%s>")
                   : _("Only users in your group now have access to mapset "
                       "<%s>");
    }
    else if (other) {
        msg = will ? _("Only users outside your group will have access to "
                       "mapset <%s>")
                   : _("Only users outside your group now have access to "
                       "mapset <%s>");
    }
    else {
        msg = will ? _("Only you will have access to mapset <%s>")
                   : _("Only you now have access to mapset <%s>");
    }

    snprintf(buf, sizeof(buf), msg, G_mapset());
    return buf;
}
