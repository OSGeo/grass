#include <grass/gis.h>
#include <grass/glocale.h>
#include "watershed.h"


int intro(void)
{
    G_message(_("%s provides a text-based user-interface to the %s program."),
	      G_program_name(), NON_NAME);
    G_message(_("%s also allows the user to prepare a report of map layers for each"),
	      G_program_name());
    G_message(_("watershed basin determined in %s.\n"), NON_NAME);

    G_message(_("%s will help the user determine which options to use for the"),
	      G_program_name());
    G_message(_("%s program.  %s will then ask for map layers that will be"),
	      NON_NAME, G_program_name());
    G_message(_("divided by basin. %s will then run %s and create the report."),
	      G_program_name(), NON_NAME);

    return (0);
}
