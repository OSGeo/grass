#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grass/gis.h>
#include "local_proto.h"

int make_location(const char *gisdbase, const char *location_name)
{
    struct Cell_head window;
    char buf[GPATH_MAX];
    int i;
    char myname[75];
    char *mapset;
    char *name, c;
    FILE *fp;

    G_clear_screen();
    fprintf(stderr,
	    "To create a new LOCATION, you will need the following information:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "1. The coordinate system for the database\n");
    /*for (i = 1; name = G__projection_name(i); i++) */
    fprintf(stderr, "        %s (for imagery and other unreferenced data)\n",
	    G__projection_name(PROJECTION_XY));
    fprintf(stderr, "        %s\n", G__projection_name(PROJECTION_LL));
    fprintf(stderr, "        %s\n", G__projection_name(PROJECTION_UTM));
    fprintf(stderr, "        %s\n", G__projection_name(PROJECTION_OTHER));
    fprintf(stderr, "2. The zone for the %s database\n",
	    G__projection_name(PROJECTION_UTM));
    fprintf(stderr,
	    "   and all the necessary parameters for projections other than\n");
    fprintf(stderr, "   %s, %s, and %s\n", G__projection_name(PROJECTION_LL),
	    G__projection_name(PROJECTION_XY),
	    G__projection_name(PROJECTION_UTM));
    fprintf(stderr,
	    "3. The coordinates of the area to become the default region\n");
    fprintf(stderr, "   and the grid resolution of this region\n");
    fprintf(stderr,
	    "4. A short, one-line description or title for the location\n");
    fprintf(stderr, "\n");

    if (!G_yes("Do you have all this information? ", 1))
	return 0;

    G_zero(&window, sizeof(window));
    while (1) {
	G_clear_screen();
	fprintf(stderr,
		"Please specify the coordinate system for location <%s>\n\n",
		location_name);
	fprintf(stderr, "A   %s\n", G__projection_name(PROJECTION_XY));
	fprintf(stderr, "B   %s\n", G__projection_name(PROJECTION_LL));
	fprintf(stderr, "C   %s\n", G__projection_name(PROJECTION_UTM));
	fprintf(stderr, "D   %s\n", G__projection_name(PROJECTION_OTHER));
	fprintf(stderr, "RETURN to cancel\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "> ");
	if (!G_gets(buf))
	    continue;
	G_strip(buf);
	if (*buf == 0)
	    return 0;
	if (sscanf(buf, "%c", &c) != 1)
	    continue;
	switch (c) {
	case 'A':
	    i = PROJECTION_XY;
	    break;
	case 'a':
	    i = PROJECTION_XY;
	    break;
	case 'B':
	    i = PROJECTION_LL;
	    break;
	case 'b':
	    i = PROJECTION_LL;
	    break;
	case 'C':
	    i = PROJECTION_UTM;
	    break;
	case 'c':
	    i = PROJECTION_UTM;
	    break;
	case 'D':
	    i = PROJECTION_OTHER;
	    break;
	case 'd':
	    i = PROJECTION_OTHER;
	    break;
	default:
	    continue;
	}
	name = G__projection_name(i);
	if (name == NULL)
	    continue;
	fprintf(stderr, "\n");
	sprintf(buf, "\n%s coordinate system? ", name);
	if (G_yes(buf, 1))
	    break;
    }
    window.proj = i;
    /*
       while (window.proj == PROJECTION_UTM)
       {
       fprintf (stderr, " ("Please specify the %s zone for location <%s>\n",
       G__projection_name(window.proj), location_name);
       fprintf (stderr, " ("or RETURN to cancel\n");
       fprintf (stderr, " ("\n");
       fprintf (stderr, " ("> ");
       if (!G_gets(buf))
       continue;
       G_strip (buf);
       if (*buf == 0) return 0;
       if (sscanf (buf, "%d", &i) != 1)
       continue;
       if (i > 0 && i <= 60)
       {
       fprintf(stderr, "zone %d is illegal"\n);
       fprintf (stderr, " ("\n");
       sprintf (buf, "zone %d? ", i);
       if (G_yes (buf, 1))
       {
       window.zone = i;
       break;
       }
       }
     */

    while (1) {
	G_clear_screen();
	fprintf(stderr,
		"Please enter a one line description for location <%s>\n\n",
		location_name);
	fprintf(stderr, "> ");
	if (!G_gets(buf))
	    continue;
	G_squeeze(buf);
	buf[sizeof(myname)] = 0;
	G_squeeze(buf);
	fprintf(stderr,
		"=====================================================\n");
	fprintf(stderr, "%s\n", buf);
	fprintf(stderr,
		"=====================================================\n");
	if (G_yes("ok? ", *buf != 0))
	    break;
    }
    strcpy(myname, buf);
    /*
       if(G_edit_cellhd(&window, -1) < 0)
       return 0;
     */

    mapset = "PERMANENT";
    G__setenv("MAPSET", mapset);
    G__setenv("LOCATION_NAME", location_name);

    sprintf(buf, "%s/%s", gisdbase, location_name);
    if (G_mkdir(buf) < 0)
	return 0;
    sprintf(buf, "%s/%s/%s", gisdbase, location_name, mapset);
    if (G_mkdir(buf) < 0)
	return 0;
    /* set the dummy window */
    window.north = 1.;
    window.south = 0.;
    window.top = 1.;
    window.bottom = 0.;
    window.rows = 1;
    window.rows3 = 1;
    window.cols = 1;
    window.cols3 = 1;
    window.depths = 1;
    window.ew_res = 1.;
    window.ew_res3 = 1.;
    window.ns_res = 1.;
    window.ns_res3 = 1.;
    window.tb_res = 1.;
    window.east = 1.;
    window.west = 0.;
    window.zone = 0.;
    /* make a dummy default window for location */
    /* later after calling g.setrpj we will let user create a real default window */
    G__put_window(&window, "", "DEFAULT_WIND");
    G__put_window(&window, "", "WIND");

    sprintf(buf, "%s/%s/%s/MYNAME", gisdbase, location_name, mapset);
    fp = fopen(buf, "w");
    fputs(myname, fp);
    fclose(fp);

    return 1;
}
