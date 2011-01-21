#include <grass/glocale.h>
#include "global.h"

int report(long rectify, int ok)
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
	G_verbose_message(_("%d:%02d:%02ld hours"), hours, minutes, seconds % 60);
    else
	G_verbose_message(_("%d:%02ld minutes"), minutes, seconds % 60);
    if (seconds)
	G_verbose_message(_("%.1f cells per minute"),
			  (60.0 * ncells) / ((double)seconds));
		      
    G_message("-----------------------------------------------");

    return 1;
}
