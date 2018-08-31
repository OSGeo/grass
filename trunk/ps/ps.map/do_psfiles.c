/* Function: do_psfiles
 **
 ** Author: Paul W. Carlson     May 1992
 */

#include <grass/gis.h>
#include <grass/glocale.h>
#include "ps_info.h"

int do_psfiles(void)
{
    int i;
    char buf[256];
    FILE *fp;

    for (i = 0; i < PS.num_psfiles; i++) {
	if ((fp = fopen(PS.psfiles[i], "r")) == NULL)
	    continue;

	G_message(_("Reading PostScript include file <%s> ..."),
		  PS.psfiles[i]);

	fprintf(PS.fp, "\n");
	while (fgets(buf, 256, fp) != NULL)
	    fprintf(PS.fp, "%s", buf);
	fprintf(PS.fp, "\n");
	fclose(fp);
    }

    return 0;
}
