
/*************************************************************
* I_list_cameras (full)
*************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/imagery.h>
#include <grass/ortholib.h>

static char *tempfile = NULL;

int I_list_cameras(int full)
{
    char *element;
    char buf[1024];
    char title[50];
    FILE *ls, *temp;
    int any;

    if (tempfile == NULL)
	tempfile = G_tempfile();

    element = "camera";
    G__make_mapset_element(element);

    temp = fopen(tempfile, "w");
    if (temp == NULL)
	G_fatal_error("can't open any temp files");
    fprintf(temp, "Available cameras\n");
    fprintf(temp, "---------------------------------\n");

    any = 0;
    strcpy(buf, "cd ");
    G_file_name(buf + strlen(buf), element, "", G_mapset());
    strcat(buf, ";ls");
    if (!full)
	strcat(buf, " -C");
    if (ls = popen(buf, "r")) {
	while (G_getl(buf, sizeof buf, ls)) {
	    any = 1;
	    fprintf(temp, "%s", buf);
	    if (full) {
		I_get_cam_title(buf, title, sizeof title);
		if (*title)
		    fprintf(temp, " (%s)", title);
		fprintf(temp, "\n");
	    }
	    else
		fprintf(temp, "\n");
	}
	pclose(ls);
    }
    if (!any)
	fprintf(temp, "no camera files available\n");
    fprintf(temp, "---------------------------------\n");
    fclose(temp);
    sprintf(buf, "$GRASS_PAGER %s", tempfile);
    G_system(buf);
    unlink(tempfile);
    fprintf(stderr, "hit RETURN to continue -->");
    G_gets(buf);

    return 0;
}
