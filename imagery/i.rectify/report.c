#include <grass/glocale.h>
#include "global.h"

int report(char *name, char *mapset, char *result,
	   long rectify, long compress, int ok)
{
    int minutes, hours;
    long seconds;
    long ncells;

    G_message("%s", ok ? _("complete") : _("failed"));

    if (!ok)
	return 1;

    seconds = rectify;
    minutes = seconds / 60;
    hours = minutes / 60;
    minutes -= hours * 60;
    ncells = target_window.rows * target_window.cols;
    G_verbose_message(_("%d rows, %d cols (%ld cells) completed in"),
			target_window.rows, target_window.cols, ncells);
    if (hours)
	G_verbose_message("%d:%02d:%02ld", hours, minutes, seconds % 60);
    else
	G_verbose_message("%d:%02ld", minutes, seconds % 60);
    if (seconds)
	G_verbose_message(_("%.1f cells per minute"),
			  (60.0 * ncells) / ((double)seconds));
		      
    seconds = compress;

    if (seconds <= 0) {
	G_message("-----------------------------------------------");
	return 1;
    }

    minutes = seconds / 60;
    hours = minutes / 60;
    minutes -= hours * 60;
    G_verbose_message(_("data compression required an additional"));
    if (hours)
	G_verbose_message("%d:%02d:%02ld", hours, minutes, seconds % 60);
    else
	G_verbose_message("%d:%02ld\n", minutes, seconds % 60);

    G_message("-----------------------------------------------");

    return 0;
}
