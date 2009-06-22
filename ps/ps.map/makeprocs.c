/* Function: make_procs
 **
 ** This function creates some commonly used PostScript procedures.
 **
 ** Author: Paul W. Carlson     March 1992
 */

#include <grass/gis.h>
#include <grass/glocale.h>
#include "ps_info.h"

int make_procs(void)
{
    char filename[1024];
    FILE *fp;
    int level;

    /* begin procs */
    fprintf(PS.fp, "\n%%%%BeginProlog\n");

    /* level 2 is default PostScript level */
    level = (PS.level != 1) ? 2 : 1;
    fprintf(PS.fp, "/level %d def\n", level);

    sprintf(filename, "%s/etc/paint/prolog.ps", G_gisbase());

    fp = fopen(filename, "r");
    if (!fp)
	G_fatal_error(_("Unable to open prolog <%s>"), filename);

    for (;;) {
	char buff[80];

	if (!fgets(buff, sizeof(buff), fp))
	    break;
	fputs(buff, PS.fp);
    }

    fclose(fp);

    /* all procs should be defined above this line */
    fprintf(PS.fp, "%%%%EndProlog\n\n");

    return 0;
}
