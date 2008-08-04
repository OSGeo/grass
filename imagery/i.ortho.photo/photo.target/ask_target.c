/* ask_target.c */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vask.h>
#include "local_proto.h"

int ask_target(char *group, char *location, char *mapset)
{
    char buf[100];
    char t1[80], t2[80], t3[80];
    char cur_location[30], cur_mapset[30];

    strcpy(cur_location, location);
    strcpy(cur_mapset, mapset);

    sprintf(t1, "Please select the target LOCATION and MAPSET for group<%s>",
	    group);
    sprintf(t2, "CURRENT LOCATION: %s", G_location());
    sprintf(t3, "CURRENT MAPSET:   %s", G_mapset());

    V_clear();
    V_line(1, t1);
    V_line(4, t2);
    V_line(5, t3);
    V_line(9, "TARGET LOCATION:");
    V_line(10, "TARGET MAPSET:");
    V_line(12,
	   "(enter list for a list of locations or mapsets within a location)");
    V_ques(location, 's', 9, 18, 20);
    V_ques(mapset, 's', 10, 18, 20);

    for (;;) {
	if (strcmp(location, "list") == 0)
	    strcpy(location, cur_location);
	if (strcmp(mapset, "list") == 0)
	    strcpy(mapset, cur_mapset);

	V_intrpt_ok();
	if (!V_call())
	    exit(0);
	if (*location == 0 && *mapset == 0)
	    exit(0);

	if (*location == 0 || strcmp(location, "list") == 0)
	    list_locations();
	else if (no_location(location)) {
	    fprintf(stderr, "\n** <%s> - unknown location\n", location);
	    list_locations();
	}
	else {
	    G__setenv("LOCATION_NAME", location);
	    if (*mapset == 0 || strcmp(mapset, "list") == 0)
		list_mapsets();
	    else if (mapset_ok(mapset))
		break;
	    else
		list_mapsets();
	}
	fprintf(stderr, "Hit RETURN -->");
	G_gets(buf);
    }

    return 0;
}

int list_locations(void)
{
    char buf[1024];

    sprintf(buf, "ls -C %s\n", G_gisdbase());
    fprintf(stderr, "\nKnown locations:\n");
    system(buf);

    return 0;
}

int no_location(char *location)
{
    char buf[1024];

    sprintf(buf, "%s/%s", G_gisdbase(), location);
    return access(buf, 0) != 0;
}

int list_mapsets(void)
{
    char buf[1024];
    FILE *fd;
    int any, ok, any_ok;
    int len, tot_len;

    sprintf(buf, "ls %s/%s", G_gisdbase(), G_location());
    fprintf(stderr, "LOCATION %s\n", G_location());
    fprintf(stderr, "\nAvailable mapsets:\n");
    fd = popen(buf, "r");
    any = 0;
    any_ok = 0;
    tot_len = 0;
    if (fd) {
	while (fscanf(fd, "%s", buf) == 1) {
	    any = 1;
	    len = strlen(buf) + 1;
	    len /= 20;
	    len = (len + 1) * 20;
	    tot_len += len;
	    if (tot_len > 75) {
		fprintf(stderr, "\n");
		tot_len = len;
	    }
	    if (ok = (G__mapset_permissions(buf) == 1))
		any_ok = 1;
	    fprintf(stderr, "%s%-*s", ok ? "(+)" : "   ", len, buf);
	}
	pclose(fd);
	if (tot_len)
	    fprintf(stderr, "\n");
	if (any_ok)
	    fprintf(stderr,
		    "\nnote: you only have access to mapsets marked with (+)\n");
	else if (any)
	    fprintf(stderr,
		    "\nnote: you do not have access to any of these mapsets\n");
    }

    return 0;
}

int mapset_ok(char *mapset)
{
    switch (G__mapset_permissions(mapset)) {
    case 1:
	return 1;
    case 0:
	fprintf(stderr, "\n** <%s> - permission to mapset denied **\n",
		mapset);
	return 0;
    default:
	fprintf(stderr, "\n** <%s> - mapset not found **\n", mapset);
	return 0;
    }
}
